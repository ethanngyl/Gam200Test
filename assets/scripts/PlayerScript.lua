--[[
===============================================================================
 File:          PlayerScript.lua
 Authors:       ETHAN NG YONG LE
 Co-Authors:    Padilla Carl Jameson Z. 
 Date:          2026-02-03
 Contribution:  Ethan (70%), Carl (30%)
 ------------------------------------------------------------------------------

 PLAYER CONTROLLER (FSM & Grid)

 Brief:
    Handles all player logic on the grid.
    Uses an internal FSM to switch between Waiting, Moving, and Attacking.
    Automatically handles Action Points (AP) and animation syncing.

 Usage:
    1. Attach this script to the Player Entity.
    2. Ensure a "GridSystem" is present in the scene for tile calculation.
    3. Controls:
       - WASD / Arrows: Select direction
       - Space: Confirm Move / Attack

 States:
    [Waiting] -> Input -> [Moving] -> Arrive -> [Waiting]
    [Waiting] -> Input -> [Attacking] -> End -> [Waiting]


 Copyright (C) 2026 DigiPen Institute of Technology.
 Reproduction or disclosure of this file or its contents
 without the prior written consent of DigiPen Institute of
 Technology is prohibited.
===============================================================================
]]--

local FSM = {}
FSM.__index = FSM

function FSM:new(name)
    local instance = {
        name = name or "FSM",
        states = {},
        currentState = nil,
        currentStateName = "",
        data = {},
        debugEnabled = false
    }
    setmetatable(instance, FSM)
    return instance
end

function FSM:addState(name, stateTable)
    if not name or not stateTable then return end
    stateTable.enter = stateTable.enter or function() end
    stateTable.update = stateTable.update or function() end
    stateTable.exit = stateTable.exit or function() end
    stateTable.fsm = self
    stateTable.name = name
    self.states[name] = stateTable
end

function FSM:start(stateName)
    if not self.states[stateName] then
        print("[" .. self.name .. "] ERROR: State '" .. stateName .. "' not found")
        return
    end
    self.currentStateName = stateName
    self.currentState = self.states[stateName]
    if self.debugEnabled then
        print("[" .. self.name .. "] Started: " .. stateName)
    end
    self.currentState.enter(self.currentState)
end

function FSM:changeState(newStateName)
    if not self.states[newStateName] then return end
    if newStateName == self.currentStateName then return end
    
    local oldName = self.currentStateName
    if self.currentState then
        self.currentState.exit(self.currentState)
    end
    
    self.currentStateName = newStateName
    self.currentState = self.states[newStateName]
    
    if self.debugEnabled then
        print("[" .. self.name .. "] " .. oldName .. " -> " .. newStateName)
    end
    self.currentState.enter(self.currentState)
end

function FSM:update(dt)
    if self.currentState then
        self.currentState.update(self.currentState, dt)
    end
end

function FSM:getCurrentState()
    return self.currentStateName
end

function FSM:isInState(stateName)
    return self.currentStateName == stateName
end

function FSM:setData(key, value)
    self.data[key] = value
end

function FSM:getData(key)
    return self.data[key]
end

function FSM:setDebugEnabled(enabled)
    self.debugEnabled = enabled
end

-- Export FSM globally for other scripts to use
_G.FSM = FSM

-- ============================================================================
-- ANIMATION STATE ENUMS
-- ============================================================================

local AnimGroup = {
    Idle = 0,
    Walk = 1,
    Attack = 2,
    Injured = 3,
    Death = 4
}

local AnimDirection = {
    Front = 0,
    Back = 1,
    Side = 2,
    None = 3
}

-- ============================================================================
-- SCRIPT STATE
-- ============================================================================

local entityID = 0
local playerFSM = nil
local moveCooldown = 0.0
local moveCooldownTime = 0.2
local apCostPerMove = 1

-- Animation state
local currentAnimGroup = AnimGroup.Idle
local currentAnimDirection = AnimDirection.Front
local isFlippedX = false

-- Debug tracking
local hasLoggedActive = false
local lastTurnPrint = nil

-- Input state tracking
local lastActiveCheck = false
local blockedKeys = {}

-- P key state tracking
local lastPKeyDown = false


-- ============================================================================
-- SKILL SYSTEM (Data-Driven)
-- ============================================================================
--
-- To add a new skill:
--   1. Add its definition to SkillDefs below
--   2. Assign it to a player + key in PlayerSkills
--   3. Done. No new functions or state variables needed.
-- ============================================================================

-- Load skill pattern definitions (provides SkillPatterns.GetPattern)
dofile("assets/scripts/SkillPatterns.lua")

-- All available skill definitions
-- skillType: nil/"melee" = instant damage, "projectile" = spawns a projectile
local SkillDefs = {
    BasicAttack = {
        name     = "Basic Attack",
        pattern  = "adjacent",
        damage   = 1,
        apCost   = 1,
        range    = 1,
    },
    AreaBlast = {
        name     = "Area Blast",
        pattern  = "area3x3",
        damage   = 1,
        apCost   = 2,
        range    = 0,  -- 0 = centered on caster
    },
    Fireball = {
        name      = "Fireball",
        skillType = "projectile",
        pattern   = "line",        -- preview: line in facing direction
        damage    = 1,
        apCost    = 2,
        range     = 5,             -- preview range (tiles shown)
        projSpeed = 3.0,           -- world units per second
        pierce    = false,         -- stops on first enemy hit
    },
    PiercingShot = {
        name      = "Piercing Shot",
        skillType = "projectile",
        pattern   = "pierce",      -- preview: line in facing direction
        damage    = 2,
        apCost    = 3,
        range     = 7,             -- longer range preview
        projSpeed = 4.0,           -- faster projectile
        pierce    = true,          -- passes through all enemies
    },
}

-- Per-player skill assignments: playerIndex -> { key -> skillID }
-- Player index is determined by spawn order (1 = first spawned, etc.)
-- Keys 1-4 = show skill preview, Space = execute the previewed skill
local PlayerSkills = {
    [1] = { ["1"] = "BasicAttack", ["2"] = "AreaBlast" },
    [2] = { ["1"] = "Fireball", ["2"] = "PiercingShot" },
    [3] = { ["1"] = "BasicAttack" },
}

-- All keys that can be bound to skills (used for preview selection)
local skillSlotKeys = { "1", "2", "3", "4" }

-- Per-key held state tracking
local lastSkillKeyDown = {}
for _, k in ipairs(skillSlotKeys) do
    lastSkillKeyDown[k] = false
end
local lastSpaceKeyDown = false

-- Active skill preview (only one at a time)
-- nil when no preview is showing; { skillID, tiles = {{x,y},...} } when active
local activePreview = nil

-- Cached player index (1, 2, or 3)
local myPlayerIndex = nil

local function getPlayerIndex()
    if myPlayerIndex then return myPlayerIndex end
    local allPlayers = GetAllPlayers()
    if allPlayers then
        for i, pid in ipairs(allPlayers) do
            if pid == entityID then
                myPlayerIndex = i
                return i
            end
        end
    end
    return nil
end

local function getMySkills()
    local idx = getPlayerIndex()
    if idx then return PlayerSkills[idx] or {} end
    return {}
end

-- Movement key state tracking (for press-only movement)
local lastWKeyDown = false
local lastSKeyDown = false
local lastAKeyDown = false
local lastDKeyDown = false

-- ============================================================================
-- HELPER FUNCTIONS
-- ============================================================================

local function blockHeldKeys()
    blockedKeys = {}
    if IsKeyDown("W") then
        blockedKeys["W"] = true
        lastWKeyDown = true  -- Track as held so first release allows movement
        print("[PlayerScript]   W is held - blocking until released")
    else
        lastWKeyDown = false
    end
    if IsKeyDown("S") then
        blockedKeys["S"] = true
        lastSKeyDown = true
        print("[PlayerScript]   S is held - blocking until released")
    else
        lastSKeyDown = false
    end
    if IsKeyDown("A") then
        blockedKeys["A"] = true
        lastAKeyDown = true
        print("[PlayerScript]   A is held - blocking until released")
    else
        lastAKeyDown = false
    end
    if IsKeyDown("D") then
        blockedKeys["D"] = true
        lastDKeyDown = true
        print("[PlayerScript]   D is held - blocking until released")
    else
        lastDKeyDown = false
    end
    if IsKeyDown("P") then
        lastPKeyDown = true
        print("[PlayerScript]   P is held - will ignore until released")
    else
        lastPKeyDown = false
    end
    -- Track skill slot keys (1-4) and Space (execute)
    for _, key in ipairs(skillSlotKeys) do
        if IsKeyDown(key) then
            lastSkillKeyDown[key] = true
            print("[PlayerScript]   " .. key .. " is held - will ignore until released")
        else
            lastSkillKeyDown[key] = false
        end
    end
    if IsKeyDown("Space") then
        lastSpaceKeyDown = true
        print("[PlayerScript]   Space is held - will ignore until released")
    else
        lastSpaceKeyDown = false
    end
    local anyHeld = lastSpaceKeyDown
    if not anyHeld then
        for _, key in ipairs(skillSlotKeys) do
            if lastSkillKeyDown[key] then anyHeld = true; break end
        end
    end
    if next(blockedKeys) == nil and not lastPKeyDown and not anyHeld then
        print("[PlayerScript]   No keys held - input ready!")
    end
end

local function updateBlockedKeys()
    if blockedKeys["W"] and not IsKeyDown("W") then
        blockedKeys["W"] = nil
        print("[PlayerScript] W released - unblocked")
    end
    if blockedKeys["S"] and not IsKeyDown("S") then
        blockedKeys["S"] = nil
        print("[PlayerScript] S released - unblocked")
    end
    if blockedKeys["A"] and not IsKeyDown("A") then
        blockedKeys["A"] = nil
        print("[PlayerScript] A released - unblocked")
    end
    if blockedKeys["D"] and not IsKeyDown("D") then
        blockedKeys["D"] = nil
        print("[PlayerScript] D released - unblocked")
    end
end

local function updateAnimationDirection(moveDirX, moveDirY)
    local newDirection = currentAnimDirection
    local newFlipX = isFlippedX

    if moveDirY > 0 then
        newDirection = AnimDirection.Back
    elseif moveDirY < 0 then
        newDirection = AnimDirection.Front
    elseif moveDirX > 0 then
        newDirection = AnimDirection.Side
        newFlipX = true
    elseif moveDirX < 0 then
        newDirection = AnimDirection.Side
        newFlipX = false
    end

    if newDirection ~= currentAnimDirection then
        currentAnimDirection = newDirection
        SetAnimationDirection(entityID, currentAnimDirection)
    end
    if newFlipX ~= isFlippedX then
        isFlippedX = newFlipX
        SetAnimationFlipX(entityID, isFlippedX)
    end
end

local function endTurn()
    if currentAnimGroup ~= AnimGroup.Idle then
        currentAnimGroup = AnimGroup.Idle
        SetAnimationGroup(entityID, currentAnimGroup)
    end
    SetSpriteColor(entityID, 1.0, 1.0, 1.0, 1.0)  -- restore default color on turn end
    EndCharacterTurn()
    hasLoggedActive = false
    lastActiveCheck = false
    blockedKeys = {}
    -- Don't reset lastPKeyDown here - keep it true if P is held
    -- lastPKeyDown = false
    -- Reset skill key states
    for _, key in ipairs(skillSlotKeys) do
        lastSkillKeyDown[key] = false
    end
    lastSpaceKeyDown = false
    -- Reset movement key states
    lastWKeyDown = false
    lastSKeyDown = false
    lastAKeyDown = false
    lastDKeyDown = false
    ClearActivePreview()
end

-- ============================================================================
-- FSM STATES
-- ============================================================================

local function createPlayerStates(fsm)

    -- ========================================================================
    -- WAITING FOR INPUT STATE
    -- ========================================================================
    fsm:addState("WaitingForInput", {
        enter = function(self)
            -- Set to Idle animation when entering this state
            if currentAnimGroup ~= AnimGroup.Idle then
                currentAnimGroup = AnimGroup.Idle
                SetAnimationGroup(entityID, currentAnimGroup)
            end
        end,

        update = function(self, dt)
            -- Only process if active character
            if not IsActiveCharacter(entityID) then return end

            -- Update blocked keys
            updateBlockedKeys()

            -- Block input during Death animation
            local currentAnim = GetAnimationGroup(entityID)
            if currentAnim == AnimGroup.Death then
                print("[PlayerScript] Entity " .. entityID .. " in Death animation - blocking input")
                return
            end

            -- Guard: skip input while move cooldown is active
            -- (moveCooldown is decremented by the main Update path)
            if moveCooldown > 0 then
                return
            end

            -- Block input during UI animation
            if IsUIAnimating and IsUIAnimating() then
                return
            end

            -- Block input during turn transition
            if IsInTurnTransition and IsInTurnTransition() then
                return
            end

            -- Check P key for manual turn end
            local pKeyDown = IsKeyDown("P")
            if pKeyDown and not lastPKeyDown then
                print("[PlayerScript] FSM: P key pressed - manually ending turn for Entity " .. entityID)
                -- Don't reset lastPKeyDown here - keep it true while P is held
                lastPKeyDown = true
                endTurn()
                return
            end
            -- Only update lastPKeyDown if P was released
            if not pKeyDown then
                lastPKeyDown = false
            end

            -- Get current position
            local currentX, currentY = GetEntityGridPosition(entityID)
            if currentX == nil or currentY == nil then
                return
            end

            -- Keys 1-4: select skill and show preview instantly
            local mySkills = getMySkills()
            for _, key in ipairs(skillSlotKeys) do
                local skillID = mySkills[key]
                local keyDown = IsKeyDown(key)
                if skillID and keyDown and not lastSkillKeyDown[key] then
                    local skill = SkillDefs[skillID]
                    local currentAttackAP = GetEntityAttackAP(entityID)
                    if skill and currentAttackAP >= skill.apCost then
                        ShowSkillPreview(skillID)
                    else
                        PulseTile(currentX, currentY, 0.3, 1.0, 1.0, 0.3)
                    end
                end
                lastSkillKeyDown[key] = keyDown
            end

            -- Space: execute the currently previewed skill
            local spaceDown = IsKeyDown("Space")
            if spaceDown and not lastSpaceKeyDown and activePreview then
                ExecuteSkill(activePreview.skillID)
            end
            lastSpaceKeyDown = spaceDown

            -- Check movement input (PRESS-ONLY - not hold)
            local wDown = IsKeyDown("W") and not blockedKeys["W"]
            local sDown = IsKeyDown("S") and not blockedKeys["S"]
            local aDown = IsKeyDown("A") and not blockedKeys["A"]
            local dDown = IsKeyDown("D") and not blockedKeys["D"]

            -- Only trigger movement on key PRESS (transition from released to pressed)
            local wPressed = wDown and not lastWKeyDown
            local sPressed = sDown and not lastSKeyDown
            local aPressed = aDown and not lastAKeyDown
            local dPressed = dDown and not lastDKeyDown

            -- Update last key states
            lastWKeyDown = wDown
            lastSKeyDown = sDown
            lastAKeyDown = aDown
            lastDKeyDown = dDown

            if wPressed or sPressed or aPressed or dPressed then
                -- Store movement direction (only the pressed key)
                fsm:setData("moveW", wPressed)
                fsm:setData("moveS", sPressed)
                fsm:setData("moveA", aPressed)
                fsm:setData("moveD", dPressed)
                fsm:changeState("Moving")
                return
            end
        end,

        exit = function(self)
            -- Nothing special
        end
    })

    -- ========================================================================
    -- MOVING STATE
    -- ========================================================================
    fsm:addState("Moving", {
        targetX = 0,
        targetY = 0,
        moveDirX = 0,
        moveDirY = 0,

        enter = function(self)
            local currentX, currentY = GetEntityGridPosition(entityID)
            self.targetX = currentX
            self.targetY = currentY
            self.moveDirX = 0
            self.moveDirY = 0

            -- Determine direction
            if self.fsm:getData("moveW") then
                self.targetY = currentY + 1
                self.moveDirY = 1
            elseif self.fsm:getData("moveS") then
                self.targetY = currentY - 1
                self.moveDirY = -1
            elseif self.fsm:getData("moveA") then
                self.targetX = currentX - 1
                self.moveDirX = -1
            elseif self.fsm:getData("moveD") then
                self.targetX = currentX + 1
                self.moveDirX = 1
            end

            -- Clear FSM data
            self.fsm:setData("moveW", false)
            self.fsm:setData("moveS", false)
            self.fsm:setData("moveA", false)
            self.fsm:setData("moveD", false)

            print("[PlayerScript] Movement attempted! Target: (" .. self.targetX .. ", " .. self.targetY .. ")")

            -- Clear any active skill preview if moving
            if activePreview then
                print("[PlayerScript] Clearing skill preview due to movement")
                ClearActivePreview()
            end

            -- Execute movement
            self:executeMove()

            -- Return to WaitingForInput
            self.fsm:changeState("WaitingForInput")
        end,

        update = function(self, dt)
            -- Movement is instant, this state exits immediately
        end,

        exit = function(self)
            -- Nothing special
        end,

        executeMove = function(self)
            print("[PlayerScript] Step 1: Validating target position (" .. self.targetX .. ", " .. self.targetY .. ")...")

            local isValid = IsValidGridPosition(self.targetX, self.targetY)
            print("[PlayerScript]   IsValidGridPosition: " .. tostring(isValid))
            if not isValid then
                print("[PlayerScript] FAILED: Invalid grid position!")
                return
            end

            local isWalkable = IsWalkableTile(self.targetX, self.targetY)
            print("[PlayerScript]   IsWalkableTile: " .. tostring(isWalkable))
            if not isWalkable then
                print("[PlayerScript] FAILED: Tile not walkable!")
                PulseTile(self.targetX, self.targetY, 0.3, 1.0, 0.3, 0.3)
                return
            end

            local isOccupied = IsTileOccupied(self.targetX, self.targetY)
            print("[PlayerScript]   IsTileOccupied: " .. tostring(isOccupied))
            if isOccupied then
                print("[PlayerScript] FAILED: Tile occupied by another entity!")
                PulseTile(self.targetX, self.targetY, 0.3, 1.0, 0.5, 0.0)  -- Orange pulse for occupied
                return
            end

            print("[PlayerScript] Step 1: PASSED - target is valid, walkable, and unoccupied")

            print("[PlayerScript] Step 2: Checking AP...")
            local currentAP, maxAP = GetEntityAP(entityID)
            print("[PlayerScript]   Entity " .. entityID .. " AP: " .. tostring(currentAP) .. "/" .. tostring(maxAP) .. " (need " .. apCostPerMove .. ")")

            if currentAP < apCostPerMove then
                print("[PlayerScript] FAILED: Not enough AP!")
                PulseTile(self.targetX, self.targetY, 0.3, 1.0, 1.0, 0.3)
                -- Removed automatic turn end - player can still attack or press P to end turn
                return
            end

            print("[PlayerScript] Step 2: PASSED - sufficient AP")

            print("[PlayerScript] Step 3: Calling MoveEntityToTile(" .. entityID .. ", " .. self.targetX .. ", " .. self.targetY .. ")...")
            local success = MoveEntityToTile(entityID, self.targetX, self.targetY)
            print("[PlayerScript] Step 3: MoveEntityToTile returned: " .. tostring(success))

            if success then
                print("[PlayerScript] Step 4: Movement SUCCESS! Consuming AP...")

                -- Update animation direction
                updateAnimationDirection(self.moveDirX, self.moveDirY)

                -- Switch to Walk animation
                if currentAnimGroup ~= AnimGroup.Attack and
                   currentAnimGroup ~= AnimGroup.Injured and
                   currentAnimGroup ~= AnimGroup.Death then
                    currentAnimGroup = AnimGroup.Walk
                    SetAnimationGroup(entityID, currentAnimGroup)
                    SetAnimationPlaying(entityID, true)
                end

                -- Consume AP
                ConsumeEntityAP(entityID, apCostPerMove)

                local newAP, maxAP = GetEntityAP(entityID)
                print("[PlayerScript] After movement: Entity " .. entityID .. " AP: " .. tostring(newAP) .. "/" .. tostring(maxAP))

                -- Removed automatic turn end - player can still attack or press P to end turn
                if newAP == 0 then
                    print("[PlayerScript] Movement AP depleted - player can still attack or press P to end turn")
                end

                -- Visual feedback
                ShowTileBorder(self.targetX, self.targetY, 0.5)
                PulseTile(self.targetX, self.targetY, 0.3, 0.3, 1.0, 0.3)

                -- Check for chest
                if HasChestAtTile(self.targetX, self.targetY) then
                    CollectChest(self.targetX, self.targetY)
                    Log("[PlayerScript] Collected chest at (" .. self.targetX .. ", " .. self.targetY .. ")")
                end

                -- Check for goal
                if HasGoalAtTile(self.targetX, self.targetY) then
                    Log("[PlayerScript] Reached goal! Level complete!")
                end

                moveCooldown = moveCooldownTime

                Log("[PlayerScript] Moved to (" .. self.targetX .. ", " .. self.targetY .. ") - AP remaining: " .. newAP)
            else
                Log("[PlayerScript] Failed to move to (" .. self.targetX .. ", " .. self.targetY .. ")")
            end
        end
    })
end

-- Death state tracking
local hasAdvancedTurnOnDeath = false  -- Prevent infinite turn advance loop

-- ============================================================================
-- LIFECYCLE: OnInit
-- ============================================================================

function OnInit(id)
    print("============================================================")
    print("========== PlayerScript OnInit() CALLED for Entity " .. id .. " ==========")
    print("============================================================")

    entityID = id

    -- Apply scale to ALL players (unified scaling)
    SetScale(entityID, 0.25, 0.25)
    print("[PlayerScript] Scaled Player " .. entityID .. " to 0.25x0.25")


    print("[PlayerScript] Checking AP/HP for Entity " .. entityID .. "...")
    local ap, maxAP = GetEntityAP(entityID)
    local hp, maxHP = GetEntityHP(entityID)
    print("[PlayerScript] Entity " .. entityID .. " - AP: " .. tostring(ap) .. "/" .. tostring(maxAP) .. ", HP: " .. tostring(hp) .. "/" .. tostring(maxHP))

    -- Initialize animation state
    print("[PlayerScript] Initializing animation state...")
    currentAnimGroup = AnimGroup.Idle
    currentAnimDirection = AnimDirection.Front
    isFlippedX = false

    SetAnimationGroup(entityID, currentAnimGroup)
    SetAnimationDirection(entityID, currentAnimDirection)
    SetAnimationFlipX(entityID, isFlippedX)

    -- Create FSM
    playerFSM = FSM:new("Player_" .. entityID)
    playerFSM:setDebugEnabled(true)  -- Set to false to reduce console spam
    createPlayerStates(playerFSM)
    playerFSM:start("WaitingForInput")

    print("[PlayerScript] Entity " .. entityID .. " initialized with FSM")
    print("============================================================")
end

-- ============================================================================
-- LIFECYCLE: OnUpdate
-- ============================================================================

function OnUpdate(dt)
    -- ========================================================================
    -- TURN SCROLL CHECK: Skip input during "Your Turn" animation
    -- ========================================================================
    
    -- Use C++ bridge function to check if turn scroll is playing
    -- This bridges from entity Lua state to level Lua state's UIManager
    if IsTurnScrollPlaying and IsTurnScrollPlaying() then
        return
    end
    
    -- ========================================================================
    -- DEATH CHECK: Stop processing if entity is dead
    -- ========================================================================

    local currentHP, maxHP = GetEntityHP(entityID)
    if not currentHP or currentHP <= 0 then
        -- Entity is dead - check if it's the active player
        local isActive = IsActiveCharacter(entityID)
        if isActive and not hasAdvancedTurnOnDeath then
            -- Active player died - automatically advance turn to prevent softlock (ONCE only)
            print("[PlayerScript] !!! DEAD ACTIVE PLAYER " .. entityID .. " - AUTO-ADVANCING TURN (ONCE) !!!")
            hasAdvancedTurnOnDeath = true  -- Prevent infinite loop
            NextCharacterTurn()  -- Call global function to advance turn
            print("[PlayerScript] !!! Turn advanced - hasAdvancedTurnOnDeath = true !!!")
        end
        -- Stop all processing
        return
    end

    -- Reset death flag when alive (for resurrection or turn switch)
    hasAdvancedTurnOnDeath = false

    -- ========================================================================
    -- PARTY SYSTEM: INPUT ROUTING
    -- ========================================================================

    -- DEBUG: Check what turn it is
    local currentTurn = GetCurrentTurn()
    if not lastTurnPrint or lastTurnPrint ~= currentTurn then
        print("[PlayerScript] Entity " .. entityID .. " - Current turn phase: " .. tostring(currentTurn))
        lastTurnPrint = currentTurn
    end

    -- CRITICAL: Only process input if this is the active character
    local isActive = IsActiveCharacter(entityID)
    if not isActive then
        if lastActiveCheck then
            -- Just became inactive - restore default color
            SetSpriteColor(entityID, 1.0, 1.0, 1.0, 1.0)
            lastActiveCheck = false
            hasLoggedActive = false
            blockedKeys = {}
            lastPKeyDown = false
            for _, key in ipairs(skillSlotKeys) do
                lastSkillKeyDown[key] = false
            end
            lastSpaceKeyDown = false
            -- Reset movement key states
            lastWKeyDown = false
            lastSKeyDown = false
            lastAKeyDown = false
            lastDKeyDown = false
            ClearActivePreview()

            if currentAnimGroup ~= AnimGroup.Idle then
                currentAnimGroup = AnimGroup.Idle
                SetAnimationGroup(entityID, currentAnimGroup)
            end
        end
        return
    end

    -- DEBUG: Log when this character becomes active (once per turn)
    if not hasLoggedActive then
        print("============================================================")
        print("========== PlayerScript: Entity " .. entityID .. " is now ACTIVE ==========")
        print("============================================================")
        local currentAP, maxAP = GetEntityAP(entityID)
        local currentHP, maxHP = GetEntityHP(entityID)
        print("[PlayerScript] Entity " .. entityID .. " AP: " .. tostring(currentAP) .. "/" .. tostring(maxAP))
        print("[PlayerScript] This entity will now respond to WASD input")
        print("============================================================")
        hasLoggedActive = true
    end

    -- ========================================================================
    -- ANIMATION BLOCKING: Disable input during scroll and AP refill animations
    -- ========================================================================

    -- Block input during animations (scroll and AP refill)
    if UIManager and UIManager.IsAnyAnimationPlaying and UIManager.IsAnyAnimationPlaying() then
        print("[PlayerScript] Blocking input - animation playing")
        return
    end

    -- ========================================================================
    -- INPUT STATE TRACKING (prevents key carry-over from previous turn)
    -- ========================================================================

    -- Detect if we just became active this frame
    if isActive and not lastActiveCheck then
        -- Just became active - highlight red
        SetSpriteColor(entityID, 1.0, 0.35, 0.35, 1.0)
        print("[PlayerScript] Entity " .. entityID .. " just became active - checking held keys...")
        blockHeldKeys()
    end
    lastActiveCheck = isActive

    -- Update FSM
    if playerFSM then
        playerFSM:update(dt)
    end
    if blockedKeys["S"] and not IsKeyDown("S") then
        blockedKeys["S"] = nil
        print("[PlayerScript] S released - unblocked")
    end
    if blockedKeys["A"] and not IsKeyDown("A") then
        blockedKeys["A"] = nil
        print("[PlayerScript] A released - unblocked")
    end
    if blockedKeys["D"] and not IsKeyDown("D") then
        blockedKeys["D"] = nil
        print("[PlayerScript] D released - unblocked")
    end

    -- ========================================================================
    -- ANIMATION STATE MANAGEMENT
    -- ========================================================================

    -- Block movement during Death animation
    local currentAnim = GetAnimationGroup(entityID)
    if currentAnim == AnimGroup.Death then
        print("[PlayerScript] Entity " .. entityID .. " in Death animation - blocking movement")
        return
    end

    -- ========================================================================
    -- MOVEMENT INPUT
    -- ========================================================================

    -- Update cooldown timer
    if moveCooldown > 0 then
        moveCooldown = moveCooldown - dt
        -- Return to Idle if not moving
        if currentAnimGroup == AnimGroup.Walk then
            currentAnimGroup = AnimGroup.Idle
            SetAnimationGroup(entityID, currentAnimGroup)
        end
        return
    end

    -- ========================================================================
    -- BLOCK INPUT DURING AP REFILL ANIMATION
    -- ========================================================================

    -- Check if UI is animating (AP crystals refilling)
    -- Uses C++ bridge to access UIManager in LevelLoader's Lua state
    if IsUIAnimating and IsUIAnimating() then
        print("[PlayerScript] DEBUG: Blocked by IsUIAnimating")
        -- Don't allow movement during AP refill animation
        return
    end

    -- ========================================================================
    -- BLOCK INPUT DURING TURN TRANSITION COOLDOWN
    -- ========================================================================

    -- Check if we're in turn transition cooldown (prevents input carry-over)
    -- Uses C++ bridge to access PartyTurnManager in LevelLoader's Lua state
    if IsInTurnTransition and IsInTurnTransition() then
        print("[PlayerScript] DEBUG: Blocked by IsInTurnTransition")
        -- Don't allow movement during turn transition cooldown
        return
    end

    -- ========================================================================
    -- MANUAL TURN END (P KEY)
    -- ========================================================================

    -- Allow player to preemptively end their turn with P key
    -- Detect single press: P is down now but wasn't down last frame
    local pKeyDown = IsKeyDown("P")
    if pKeyDown and not lastPKeyDown then
        print("[PlayerScript] P key pressed - manually ending turn for Entity " .. entityID)

        -- Set animation back to Idle before ending turn
        if currentAnimGroup ~= AnimGroup.Idle then
            currentAnimGroup = AnimGroup.Idle
            SetAnimationGroup(entityID, currentAnimGroup)
        end

        SetSpriteColor(entityID, 1.0, 1.0, 1.0, 1.0)  -- restore default color on turn end
        EndCharacterTurn()
        hasLoggedActive = false  -- Reset for next character
        lastActiveCheck = false  -- Reset active tracking
        blockedKeys = {}  -- Clear blocked keys
        -- Don't reset lastPKeyDown here - let it stay true while P is held
        -- It will be reset when P is released or when character becomes inactive

        -- Return early - turn is over
        return
    end
    lastPKeyDown = pKeyDown  -- Update P key state for next frame

    -- NOTE: Skill key handling (1-4 + Space) removed from here.
    -- The FSM WaitingForInput state is the single source of truth for skill input.
    -- Having duplicate skill processing here caused lastSkillKeyDown to be updated
    -- twice per frame, leading to missed key presses during moveCooldown transitions.

end

-- NOTE: Old duplicate movement code removed - FSM handles all movement input
-- ============================================================================
-- GENERIC SKILL FUNCTIONS (data-driven)
-- ============================================================================

-- Show preview tiles for any skill
function ShowSkillPreview(skillID)
    ClearActivePreview()

    local skill = SkillDefs[skillID]
    if not skill then
        print("[PlayerScript] ERROR: Unknown skill '" .. tostring(skillID) .. "'")
        return
    end

    local currentX, currentY = GetEntityGridPosition(entityID)
    if not currentX or not currentY then return end

    print("[PlayerScript] Showing preview for " .. skill.name .. " at (" .. currentX .. ", " .. currentY .. ")")

    -- Projectile skills: no tile preview, just register as active for Space to execute
    if skill.skillType == "projectile" then
        activePreview = { skillID = skillID, tiles = {} }
        print("[PlayerScript] Preview active (projectile, no tiles): " .. skill.name)
        return
    end

    local pattern = SkillPatterns.GetPattern(skill.pattern, currentAnimDirection, skill.range, isFlippedX)
    local tiles = {}

    for _, offset in ipairs(pattern) do
        local tileX = currentX + offset.x
        local tileY = currentY + offset.y
        if IsValidGridPosition(tileX, tileY) then
            TintTile(tileX, tileY, 1.0, 0.3, 0.3, 0.7)
            table.insert(tiles, {x = tileX, y = tileY})
        end
    end

    if #tiles > 0 then
        activePreview = { skillID = skillID, tiles = tiles }
        print("[PlayerScript] Preview active: " .. skill.name .. " (" .. #tiles .. " tiles)")
    end
end

-- Clear the active skill preview (works for any skill)
function ClearActivePreview()
    if activePreview and activePreview.tiles then
        for _, tile in ipairs(activePreview.tiles) do
            if tile and tile.x and tile.y then
                TintTile(tile.x, tile.y, 1.0, 1.0, 1.0, 1.0)
            end
        end
    end
    activePreview = nil
end

-- Find all enemies within a skill's pattern
function FindEnemiesInPattern(skillID)
    local skill = SkillDefs[skillID]
    if not skill then return {} end

    local currentX, currentY = GetEntityGridPosition(entityID)
    if not currentX or not currentY then return {} end

    local enemies = GetAllEnemies()
    if not enemies or #enemies == 0 then return {} end

    local pattern = SkillPatterns.GetPattern(skill.pattern, currentAnimDirection, skill.range, isFlippedX)

    -- Build lookup of target tiles
    local targetTiles = {}
    for _, offset in ipairs(pattern) do
        targetTiles[(currentX + offset.x) .. "," .. (currentY + offset.y)] = true
    end

    local found = {}
    for _, eID in ipairs(enemies) do
        local ex, ey = GetEntityGridPosition(eID)
        if ex and ey and targetTiles[ex .. "," .. ey] then
            table.insert(found, {id = eID, x = ex, y = ey})
        end
    end

    return found
end

-- Get the world-space direction vector for the player's current facing
-- Uses movement conventions: W=+Y(Back), S=-Y(Front), D=+X(Side+flip), A=-X(Side)
local function getFacingWorldDirection()
    if currentAnimDirection == AnimDirection.Front then
        return 0, -1   -- Facing camera = down = -Y
    elseif currentAnimDirection == AnimDirection.Back then
        return 0, 1    -- Facing away = up = +Y
    elseif currentAnimDirection == AnimDirection.Side then
        if isFlippedX then
            return 1, 0   -- flipX=true = facing right = +X
        else
            return -1, 0  -- flipX=false = facing left = -X
        end
    end
    return 0, -1  -- Default: down
end

-- Execute any skill: melee (instant damage) or projectile (spawns projectile)
function ExecuteSkill(skillID)
    local skill = SkillDefs[skillID]
    if not skill then
        ClearActivePreview()
        return
    end

    print("[PlayerScript] ===== EXECUTING: " .. skill.name .. " =====")

    -- Check AP
    local currentAP, maxAP = GetEntityAttackAP(entityID)
    if not currentAP or currentAP < skill.apCost then
        print("[PlayerScript] Not enough Attack AP (" .. tostring(currentAP) .. " < " .. skill.apCost .. ")")
        ClearActivePreview()
        return
    end

    -- Branch: projectile skills spawn a projectile entity
    if skill.skillType == "projectile" then
        local px, py = GetEntityGridPosition(entityID)
        if not px or not py then
            ClearActivePreview()
            return
        end

        -- Get the player's world position for spawning
        local worldX, worldY = TileToWorld(px, py)
        if not worldX or not worldY then
            ClearActivePreview()
            return
        end

        -- Get the facing direction in grid space, then convert to world direction
        local gridDirX, gridDirY = getFacingWorldDirection()
        local targetWorldX, targetWorldY = TileToWorld(px + gridDirX, py + gridDirY)
        local dirX, dirY = 0, 0
        if targetWorldX and targetWorldY then
            dirX = targetWorldX - worldX
            dirY = targetWorldY - worldY
            local len = math.sqrt(dirX * dirX + dirY * dirY)
            if len > 0 then dirX = dirX / len; dirY = dirY / len end
        end

        -- Fallback: if adjacent tile was out of bounds, use grid direction directly
        if dirX == 0 and dirY == 0 then
            dirX, dirY = gridDirX, gridDirY
        end

        -- Determine projectile visuals per player/skill
        local tintR, tintG, tintB, tintA = 1, 1, 1, 1   -- default white
        local spritePath = nil                             -- nil = default bullet.png
        if getPlayerIndex() == 2
           and (skillID == "Fireball" or skillID == "PiercingShot") then
            tintR, tintG, tintB, tintA = 1, 0, 0, 1      -- red
            spritePath = ""                                 -- no texture → solid color
        end

        -- Spawn the projectile via C++ bridge
        local projID = SpawnSkillProjectile(
            worldX, worldY,
            dirX, dirY,
            skill.projSpeed or 3.0,
            skill.damage or 1,
            skill.pierce or false,
            tintR, tintG, tintB, tintA,
            spritePath
        )
        if not projID then
            print("[PlayerScript] ERROR: SpawnSkillProjectile returned nil - not consuming AP")
            ClearActivePreview()
            return
        end

        print("[PlayerScript] Spawned projectile ID=" .. tostring(projID)
            .. " dir=(" .. dirX .. "," .. dirY .. ")"
            .. " speed=" .. (skill.projSpeed or 3.0)
            .. " dmg=" .. skill.damage
            .. " pierce=" .. tostring(skill.pierce or false))

        -- Consume AP only after successful spawn
        ConsumeEntityAttackAP(entityID, skill.apCost)

        -- Trigger AP crystal animation (once per AP spent)
        if UIManager and UIManager.GetComponent then
            local comp = UIManager.GetComponent("attackAP")
            if comp and comp.ConsumeOneAP then
                for i = 1, skill.apCost do
                    pcall(function() comp:ConsumeOneAP() end)
                end
            end
        else
            pcall(function() TriggerAttackAPAnimation() end)
        end

        -- Play attack animation
        currentAnimGroup = AnimGroup.Attack
        SetAnimationGroup(entityID, currentAnimGroup)
        SetAnimationLoop(entityID, false)

        ClearActivePreview()
        return
    end

    -- Melee skills: find enemies in pattern and deal instant damage
    local enemies = FindEnemiesInPattern(skillID)
    if #enemies == 0 then
        print("[PlayerScript] No enemies in " .. skill.name .. " range")
        ClearActivePreview()
        return
    end

    -- Face the first enemy found
    do
        local px, py = GetEntityGridPosition(entityID)
        local first = enemies[1]
        if px and py and first then
            local dx = first.x - px
            local dy = first.y - py
            local newDir = currentAnimDirection
            local newFlip = isFlippedX

            if math.abs(dx) > math.abs(dy) then
                newDir = AnimDirection.Side
                newFlip = dx > 0
            elseif dy > 0 then
                newDir = AnimDirection.Back
            elseif dy < 0 then
                newDir = AnimDirection.Front
            end

            if newDir ~= currentAnimDirection then
                currentAnimDirection = newDir
                SetAnimationDirection(entityID, currentAnimDirection)
            end
            if newFlip ~= isFlippedX then
                isFlippedX = newFlip
                SetAnimationFlipX(entityID, isFlippedX)
            end
        end
    end

    -- Deal damage to all enemies in pattern
    local enemiesHit = 0
    for _, enemy in ipairs(enemies) do
        local success = DamageEntity(enemy.id, skill.damage)
        if success then
            enemiesHit = enemiesHit + 1
            PulseTile(enemy.x, enemy.y, 0.5, 1.0, 0.0, 0.0)

            local hpAfter = GetEntityHP(enemy.id)
            if hpAfter and hpAfter <= 0 then
                print("[PlayerScript] Enemy " .. enemy.id .. " DEFEATED!")
            end
        end
    end

    -- Consume AP
    ConsumeEntityAttackAP(entityID, skill.apCost)
    print("[PlayerScript] " .. skill.name .. ": hit " .. enemiesHit .. " enemies for " .. skill.damage .. " damage each")

    -- Trigger AP crystal animation (once per AP spent)
    if UIManager and UIManager.GetComponent then
        local comp = UIManager.GetComponent("attackAP")
        if comp and comp.ConsumeOneAP then
            for i = 1, skill.apCost do
                pcall(function() comp:ConsumeOneAP() end)
            end
        end
    else
        pcall(function() TriggerAttackAPAnimation() end)
    end

    -- Play attack animation
    currentAnimGroup = AnimGroup.Attack
    SetAnimationGroup(entityID, currentAnimGroup)
    SetAnimationLoop(entityID, false)

    ClearActivePreview()
end

-- ============================================================================
-- LIFECYCLE: OnDestroy
-- ============================================================================

function OnDestroy()
    ClearActivePreview()
    Log("[PlayerScript] Destroyed for entity " .. entityID)
end

-- ============================================================================
-- EXTERNAL API
-- ============================================================================

function GetPlayerState()
    if playerFSM then
        return playerFSM:getCurrentState()
    end
    return "Unknown"
end

function GetPlayerFSM()
    return playerFSM
end

_G.GetPlayerState = GetPlayerState
_G.GetPlayerFSM = GetPlayerFSM