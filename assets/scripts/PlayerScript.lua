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

-- Helper: get actual movement cost (0 if Bloody Warcry free move is active)
local function getMovementCost()
    if bloodyWarcryFreeMove then
        return 0
    end
    return apCostPerMove
end

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
--   1. Add its definition to assets/JSON/Skills.json
--   2. Assign it to a player + key in PlayerSkills below
--   3. Done. No new functions or state variables needed.
-- ============================================================================

-- Load skill pattern definitions (provides SkillPatterns.GetPattern)
dofile("assets/scripts/SkillPatterns.lua")

-- Skill definitions loaded from JSON in OnInit() (single source of truth)
-- skillType: nil/"melee" = instant damage, "projectile" = spawns a projectile
local SkillDefs = {}

-- Per-player skill assignments: playerIndex -> { key -> skillID }
-- Player index is determined by spawn order (1 = first spawned, etc.)
-- Keys 1-4 = show skill preview, Space = execute the previewed skill
-- Defaults are overridden by SkillLoadout.json if it exists (written by SkillSwapUI)
local PlayerSkills = {
    [1] = { ["1"] = "Thrust", ["2"] = "SoulRend" },
    [2] = { ["1"] = "Fireball", ["2"] = "SoulRend" },
    [3] = { ["1"] = "SwiftBlow" },
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

-- Dash skill state: when a dash skill is previewed, WASD picks direction instead of moving
-- nil when not in dash mode; { skillID = ..., dirX = 0, dirY = 0 } when active
local dashMode = nil

-- Ally targeting state: when an ally_target skill is previewed, Tab/Shift+Tab to cycle
-- nil when not targeting; { skillID, targets, currentIndex, selectedAlly } when active
local allyTargetMode = nil

-- Enemy targeting state: when an enemy_target skill is previewed, Tab/Shift+Tab to cycle
-- nil when not targeting; { skillID, targets = {eid,...}, currentIndex, selectedEnemy } when active
local enemyTargetMode = nil

-- Tab key state for edge detection (Tab/Shift+Tab cycling)
local lastTabKeyDown = false

-- Berserker: Dark Omens once-per-level tracking
local darkOmensUsedThisLevel = false

-- Berserker: Siphon Charge state tracking (next attack consumes all AP + heals)
local siphonChargeActive = false
local siphonTriggeredThisSkill = false

-- Berserker: Bloody Warcry tracking (first move free, next skill +1 dmg -1 HP)
local bloodyWarcryFreeMove = false
local bloodyWarcryDamageBonus = false

-- Berserker: Dark Omens triggered tracking (2 free skills, then die)
local darkOmensSkillsRemaining = 0

-- Berserker: current warcry damage bonus (set per-skill in ExecuteSkill)
local currentWarcryBonus = 0

-- Turn start initialization flag (resets when character becomes active)
local turnStartInitialized = false
local lastActiveState = false

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
            local isActive = IsActiveCharacter(entityID)
            if not isActive then
                lastActiveState = false
                return
            end

            -- Detect turn start (transition from inactive to active)
            if not lastActiveState then
                lastActiveState = true
                -- Initialize Berserker turn-start effects
                if HasStatusEffect and HasStatusEffect(entityID, "bloodyWarcry") then
                    bloodyWarcryFreeMove = true
                    bloodyWarcryDamageBonus = true
                    print("[PlayerScript] Bloody Warcry active: first move free, next skill +1 dmg -1 HP")
                end
                if HasStatusEffect and HasStatusEffect(entityID, "darkOmensTriggered") then
                    darkOmensSkillsRemaining = 2
                    print("[PlayerScript] Dark Omens Triggered: 2 free skills this turn, then death")
                end
                if HasStatusEffect and HasStatusEffect(entityID, "siphonCharge") then
                    siphonChargeActive = true
                    print("[PlayerScript] Siphon Charge active: first attack consumes all AP + heals 3 HP")
                end
            end

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

            -- ============================================================
            -- DASH MODE: WASD picks dash direction instead of moving
            -- ============================================================
            if dashMode then
                local wDown = IsKeyDown("W") and not blockedKeys["W"]
                local sDown = IsKeyDown("S") and not blockedKeys["S"]
                local aDown = IsKeyDown("A") and not blockedKeys["A"]
                local dDown = IsKeyDown("D") and not blockedKeys["D"]

                local wPressed = wDown and not lastWKeyDown
                local sPressed = sDown and not lastSKeyDown
                local aPressed = aDown and not lastAKeyDown
                local dPressed = dDown and not lastDKeyDown

                lastWKeyDown = wDown
                lastSKeyDown = sDown
                lastAKeyDown = aDown
                lastDKeyDown = dDown

                local newDirX, newDirY = nil, nil
                if wPressed then newDirX, newDirY = 0, 1       -- up
                elseif sPressed then newDirX, newDirY = 0, -1  -- down
                elseif aPressed then newDirX, newDirY = -1, 0  -- left
                elseif dPressed then newDirX, newDirY = 1, 0   -- right
                end

                if newDirX and newDirY then
                    local skill = SkillDefs[dashMode.skillID]
                    local dashRange = skill and skill.dashRange or 3

                    -- Validate: check that the first tile isn't a wall
                    -- (enemies are OK - dash passes through them)
                    local px, py = GetEntityGridPosition(entityID)
                    local firstX = px + newDirX
                    local firstY = py + newDirY
                    local firstIsWall = not px or not py
                        or not IsValidGridPosition(firstX, firstY)
                        or (not IsWalkableTile(firstX, firstY) and not IsTileOccupied(firstX, firstY))
                    if not firstIsWall then
                        dashMode.dirX = newDirX
                        dashMode.dirY = newDirY

                        -- Clear old preview tiles
                        if activePreview and activePreview.tiles then
                            for _, tile in ipairs(activePreview.tiles) do
                                TintTile(tile.x, tile.y, 1.0, 1.0, 1.0, 1.0)
                            end
                        end

                        -- Show dash path preview (passes through enemies)
                        local tiles = {}
                        for i = 1, dashRange do
                            local tileX = px + newDirX * i
                            local tileY = py + newDirY * i
                            if not IsValidGridPosition(tileX, tileY) then
                                break  -- out of bounds
                            end
                            local walkable = IsWalkableTile(tileX, tileY)
                            local occupied = IsTileOccupied(tileX, tileY)
                            if not walkable and not occupied then
                                break  -- wall stops dash
                            end
                            if occupied then
                                TintTile(tileX, tileY, 1.0, 0.2, 0.0, 0.7)  -- red for enemy in path
                            else
                                TintTile(tileX, tileY, 1.0, 0.5, 0.0, 0.7)  -- orange for dash path
                            end
                            table.insert(tiles, {x = tileX, y = tileY})
                        end
                        activePreview = { skillID = dashMode.skillID, tiles = tiles }
                        print("[PlayerScript] Dash direction: (" .. newDirX .. "," .. newDirY .. "), Space to execute")
                    else
                        PulseTile(currentX, currentY, 0.3, 1.0, 0.3, 0.3)
                        print("[PlayerScript] Dash blocked: wall in that direction")
                    end
                end

                return  -- consume all input while in dash mode
            end

            -- ============================================================
            -- ALLY TARGET MODE: Tab/Shift+Tab to cycle, Space to confirm
            -- ============================================================
            if allyTargetMode then
                local tabDown = IsKeyDown("Tab")
                local tabPressed = tabDown and not lastTabKeyDown
                lastTabKeyDown = tabDown

                if tabPressed and #allyTargetMode.targets > 0 then
                    local n = #allyTargetMode.targets
                    local idx = allyTargetMode.currentIndex
                    if IsKeyDown("Shift") then
                        idx = idx - 1
                        if idx < 1 then idx = n end
                    else
                        idx = idx + 1
                        if idx > n then idx = 1 end
                    end
                    allyTargetMode.currentIndex = idx
                    allyTargetMode.selectedAlly = allyTargetMode.targets[idx]
                    -- Re-tint: highlight selected (selected=bold green, unselected=very dim)
                    for i, t in ipairs(activePreview and activePreview.tiles or {}) do
                        local bright = (i == idx)
                        TintTile(t.x, t.y, bright and 0.0 or 0.1, bright and 1.0 or 0.15, bright and 0.0 or 0.1, bright and 0.95 or 0.35)
                    end
                    print("[PlayerScript] Ally selected: " .. tostring(allyTargetMode.selectedAlly) .. " (" .. idx .. "/" .. n .. "), Space to confirm")
                end

                local wDown = IsKeyDown("W") and not blockedKeys["W"]
                local sDown = IsKeyDown("S") and not blockedKeys["S"]
                local aDown = IsKeyDown("A") and not blockedKeys["A"]
                local dDown = IsKeyDown("D") and not blockedKeys["D"]
                lastWKeyDown = wDown
                lastSKeyDown = sDown
                lastAKeyDown = aDown
                lastDKeyDown = dDown
                return  -- consume all input while in ally target mode
            end

            -- ============================================================
            -- ENEMY TARGET MODE: Tab/Shift+Tab to cycle, Space to confirm
            -- ============================================================
            if enemyTargetMode then
                local tabDown = IsKeyDown("Tab")
                local tabPressed = tabDown and not lastTabKeyDown
                lastTabKeyDown = tabDown

                if tabPressed and #enemyTargetMode.targets > 0 then
                    local n = #enemyTargetMode.targets
                    local idx = enemyTargetMode.currentIndex
                    if IsKeyDown("Shift") then
                        idx = idx - 1
                        if idx < 1 then idx = n end
                    else
                        idx = idx + 1
                        if idx > n then idx = 1 end
                    end
                    enemyTargetMode.currentIndex = idx
                    enemyTargetMode.selectedEnemy = enemyTargetMode.targets[idx]
                    -- Re-tint: highlight selected (selected=bold red, unselected=very dim)
                    for i, t in ipairs(activePreview and activePreview.tiles or {}) do
                        local bright = (i == idx)
                        TintTile(t.x, t.y, bright and 1.0 or 0.15, bright and 0.0 or 0.1, bright and 0.0 or 0.1, bright and 0.95 or 0.35)
                    end
                    print("[PlayerScript] Enemy selected: " .. tostring(enemyTargetMode.selectedEnemy) .. " (" .. idx .. "/" .. n .. "), Space to execute")
                end

                local wDown = IsKeyDown("W") and not blockedKeys["W"]
                local sDown = IsKeyDown("S") and not blockedKeys["S"]
                local aDown = IsKeyDown("A") and not blockedKeys["A"]
                local dDown = IsKeyDown("D") and not blockedKeys["D"]
                lastWKeyDown = wDown
                lastSKeyDown = sDown
                lastAKeyDown = aDown
                lastDKeyDown = dDown
                return  -- consume all input while in enemy target mode
            end

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
    -- MOVING STATE (Smooth Glide)
    -- ========================================================================
    fsm:addState("Moving", {
        -- Lerp state
        startWorldX = 0,
        startWorldY = 0,
        endWorldX   = 0,
        endWorldY   = 0,
        elapsed     = 0,
        duration    = 0.18,  -- seconds to glide (set high to test, lower later)
        moveValid   = false,

        -- Direction / target
        moveDirX = 0,
        moveDirY = 0,
        targetX  = 0,
        targetY  = 0,

        enter = function(self)
            print("[MOVING] enter() called")
            self.moveValid = false
            self.elapsed = 0

            local currentX, currentY = GetEntityGridPosition(entityID)
            if not currentX or not currentY then
                print("[MOVING] EARLY EXIT: no grid position")
                self.fsm:changeState("WaitingForInput")
                return
            end

            self.targetX  = currentX
            self.targetY  = currentY
            self.moveDirX = 0
            self.moveDirY = 0

            -- Determine direction from FSM data
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

            -- ==============================================================
            -- VALIDATION
            -- ==============================================================

            local isValid = IsValidGridPosition(self.targetX, self.targetY)
            if not isValid then
                print("[PlayerScript] FAILED: Invalid grid position!")
                self.fsm:changeState("WaitingForInput")
                return
            end

            local isWalkable = IsWalkableTile(self.targetX, self.targetY)
            if not isWalkable then
                print("[PlayerScript] FAILED: Tile not walkable!")
                PulseTile(self.targetX, self.targetY, 0.3, 1.0, 0.3, 0.3)
                self.fsm:changeState("WaitingForInput")
                return
            end

            local isOccupied = IsTileOccupied(self.targetX, self.targetY)
            if isOccupied then
                print("[PlayerScript] FAILED: Tile occupied!")
                PulseTile(self.targetX, self.targetY, 0.3, 1.0, 0.5, 0.0)
                self.fsm:changeState("WaitingForInput")
                return
            end

            local actualMoveCost = getMovementCost()
            local currentAP, maxAP = GetEntityAP(entityID)
            if currentAP < actualMoveCost then
                print("[PlayerScript] FAILED: Not enough AP!")
                PulseTile(self.targetX, self.targetY, 0.3, 1.0, 1.0, 0.3)
                self.fsm:changeState("WaitingForInput")
                return
            end

            -- ==============================================================
            -- ALL CHECKS PASSED
            -- ==============================================================
            print("[MOVING] All checks passed! Setting up glide...")

            -- 1) Save the current world position BEFORE the snap
            local sx, sy = GetEntityWorldPosition(entityID)
            if not sx or not sy then
                print("[MOVING] WARNING: Could not get start position, falling back to instant")
                MoveEntityToTile(entityID, self.targetX, self.targetY)
                ConsumeEntityAP(entityID, actualMoveCost)
                if bloodyWarcryFreeMove then bloodyWarcryFreeMove = false end
                self.fsm:changeState("WaitingForInput")
                return
            end
            self.startWorldX = sx
            self.startWorldY = sy

            -- 2) Do the normal MoveEntityToTile (updates grid occupancy + snaps position)
            local success = MoveEntityToTile(entityID, self.targetX, self.targetY)
            if not success then
                print("[MOVING] MoveEntityToTile failed!")
                self.fsm:changeState("WaitingForInput")
                return
            end

            -- 3) Save the end position (where MoveEntityToTile just snapped us to)
            local ex, ey = GetEntityWorldPosition(entityID)
            if not ex or not ey then
                print("[MOVING] WARNING: Could not get end position")
                ConsumeEntityAP(entityID, actualMoveCost)
                if bloodyWarcryFreeMove then bloodyWarcryFreeMove = false end
                self.fsm:changeState("WaitingForInput")
                return
            end
            self.endWorldX = ex
            self.endWorldY = ey

            -- 4) Yank the sprite BACK to the start position (grid is already updated)
            SetSpritePosition(entityID, self.startWorldX, self.startWorldY)

            -- 5) Consume AP (Bloody Warcry: first move is free)
            ConsumeEntityAP(entityID, actualMoveCost)
            if bloodyWarcryFreeMove then
                print("[PlayerScript] Bloody Warcry: free movement!")
                bloodyWarcryFreeMove = false
            end
            local newAP, _ = GetEntityAP(entityID)
            print("[PlayerScript] AP consumed. Remaining: " .. tostring(newAP))

            if newAP == 0 then
                print("[PlayerScript] Movement AP depleted - player can still attack or press P to end turn")
            end

            -- 6) Update animation direction + switch to Walk
            updateAnimationDirection(self.moveDirX, self.moveDirY)

            if currentAnimGroup ~= AnimGroup.Attack and
               currentAnimGroup ~= AnimGroup.Injured and
               currentAnimGroup ~= AnimGroup.Death then
                currentAnimGroup = AnimGroup.Walk
                SetAnimationGroup(entityID, currentAnimGroup)
                SetAnimationPlaying(entityID, true)
            end

            -- 7) Visual feedback
            ShowTileBorder(self.targetX, self.targetY, 0.5)
            PulseTile(self.targetX, self.targetY, 0.3, 0.3, 1.0, 0.3)

            -- 8) Check for chest/goal
            if HasChestAtTile(self.targetX, self.targetY) then
                CollectChest(self.targetX, self.targetY)
                Log("[PlayerScript] Collected chest at (" .. self.targetX .. ", " .. self.targetY .. ")")
            end

            if HasGoalAtTile(self.targetX, self.targetY) then
                Log("[PlayerScript] Reached goal! Level complete!")
            end

            -- 9) Mark glide as valid + set cooldown
            self.moveValid = true
            moveCooldown = moveCooldownTime

            print("[MOVING] Glide: (" .. sx .. "," .. sy .. ") -> (" .. ex .. "," .. ey .. ") over " .. self.duration .. "s")
            -- NOTE: Do NOT changeState here! update() will handle the glide.
        end,

        update = function(self, dt)
            if not self.moveValid then
                self.fsm:changeState("WaitingForInput")
                return
            end

            self.elapsed = self.elapsed + dt
            local t = self.elapsed / self.duration
            if t > 1.0 then t = 1.0 end

            -- Ease-out quadratic: fast launch, gentle arrival
            local eased = 1.0 - (1.0 - t) * (1.0 - t)

            -- Lerp position
            local x = self.startWorldX + (self.endWorldX - self.startWorldX) * eased
            local y = self.startWorldY + (self.endWorldY - self.startWorldY) * eased

            -- Set ONLY the visual position (grid already updated in enter)
            SetSpritePosition(entityID, x, y)

            print("[GLIDE] t=" .. string.format("%.2f", t) .. " pos=(" .. string.format("%.4f", x) .. "," .. string.format("%.4f", y) .. ")")

            -- Arrived?
            if t >= 1.0 then
                SetSpritePosition(entityID, self.endWorldX, self.endWorldY)
                self.fsm:changeState("WaitingForInput")
            end
        end,

        exit = function(self)
            -- Safety: ensure we're exactly on the target tile
            if self.moveValid then
                SetSpritePosition(entityID, self.endWorldX, self.endWorldY)
            end
            self.moveValid = false
        end,
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

    -- Load skill definitions from JSON
    local skillData = LoadJSON("assets/JSON/Skills.json")
    if skillData and skillData.skills then
        SkillDefs = skillData.skills
        local n = 0; for _ in pairs(SkillDefs) do n = n + 1 end
        print("[PlayerScript] Loaded " .. n .. " skills from Skills.json")
    else
        print("[PlayerScript] ERROR: Failed to load Skills.json!")
    end

    -- Load skill loadout from JSON (written by SkillSwapUI between levels)
    local loadoutData = LoadJSON("assets/JSON/SkillLoadout.json")
    if loadoutData and loadoutData.players then
        for i = 1, 3 do
            local p = loadoutData.players[tostring(i)]
            if p then
                PlayerSkills[i] = {}
                for slot = 1, 4 do
                    local sid = p[tostring(slot)]
                    if sid then
                        PlayerSkills[i][tostring(slot)] = sid
                    end
                end
            end
        end
        print("[PlayerScript] Loaded skill loadout from SkillLoadout.json")
    else
        print("[PlayerScript] No SkillLoadout.json found, using default skills")
    end

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
            -- Just became inactive
            lastActiveCheck = false
            hasLoggedActive = false
            blockedKeys = {}
            lastPKeyDown = false
            for _, key in ipairs(skillSlotKeys) do
                lastSkillKeyDown[key] = false
            end
            lastSpaceKeyDown = false
            lastTabKeyDown = false
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

    -- Dark Omens: block preview if already used this level
    if skill.oncePerLevel and darkOmensUsedThisLevel then
        print("[PlayerScript] " .. skill.name .. " already used this level!")
        PulseTile(currentX, currentY, 0.3, 0.5, 0.5, 0.5)
        return
    end

    -- HP cost check: block preview if not enough HP
    if skill.hpCost and skill.hpCost > 0 then
        local currentHP = GetEntityHP(entityID)
        if not currentHP or currentHP <= skill.hpCost then
            print("[PlayerScript] Not enough HP for " .. skill.name .. " (HP:" .. tostring(currentHP) .. " <= cost:" .. skill.hpCost .. ")")
            PulseTile(currentX, currentY, 0.3, 1.0, 0.5, 0.0)
            return
        end
    end

    -- Projectile skills: no tile preview, just register as active for Space to execute
    if skill.skillType == "projectile" then
        activePreview = { skillID = skillID, tiles = {} }
        print("[PlayerScript] Preview active (projectile, no tiles): " .. skill.name)
        return
    end

    -- Self-buff skills (Guard, Parry): tint own tile, Space to execute
    if skill.skillType == "self_buff" then
        TintTile(currentX, currentY, 0.3, 0.3, 1.0, 0.7)  -- blue tint on self
        activePreview = { skillID = skillID, tiles = {{x = currentX, y = currentY}} }
        print("[PlayerScript] Preview active (self-buff): " .. skill.name)
        return
    end

    -- Dash skills (Swift Blow): enter dash mode, WASD picks direction
    if skill.skillType == "dash" then
        dashMode = { skillID = skillID, dirX = 0, dirY = 0 }
        -- Tint own tile yellow to indicate dash mode
        TintTile(currentX, currentY, 1.0, 1.0, 0.3, 0.7)
        activePreview = { skillID = skillID, tiles = {{x = currentX, y = currentY}} }
        print("[PlayerScript] Dash mode active: use WASD to pick direction, Space to execute")
        return
    end

    -- Global damage skills (Lightning Strike): tint self, Space to hit all enemies
    if skill.skillType == "global_damage" then
        TintTile(currentX, currentY, 1.0, 1.0, 0.3, 0.7)  -- yellow tint on self
        activePreview = { skillID = skillID, tiles = {{x = currentX, y = currentY}} }
        print("[PlayerScript] Preview active (global damage): " .. skill.name)
        return
    end

    -- Self-overload skills (Overload): tint self, Space to execute
    if skill.skillType == "self_overload" then
        TintTile(currentX, currentY, 1.0, 0.5, 0.0, 0.7)  -- orange tint on self
        activePreview = { skillID = skillID, tiles = {{x = currentX, y = currentY}} }
        print("[PlayerScript] Preview active (self-overload): " .. skill.name)
        return
    end

    -- Groundshatter: 5x5 area centered on self, tint all surrounding tiles
    if skill.skillType == "groundshatter" then
        local pattern = SkillPatterns.GetPattern(skill.pattern, currentAnimDirection, skill.range, isFlippedX)
        local tiles = {}
        for _, offset in ipairs(pattern) do
            local tileX = currentX + offset.x
            local tileY = currentY + offset.y
            if IsValidGridPosition(tileX, tileY) then
                TintTile(tileX, tileY, 1.0, 0.3, 0.0, 0.7)  -- orange-red for AoE damage
                table.insert(tiles, {x = tileX, y = tileY})
            end
        end
        -- Also tint self tile (caster is stunned but not damaged by own skill)
        TintTile(currentX, currentY, 1.0, 0.5, 0.0, 0.7)
        table.insert(tiles, {x = currentX, y = currentY})
        activePreview = { skillID = skillID, tiles = tiles }
        print("[PlayerScript] Preview active (groundshatter 5x5): " .. skill.name .. " (" .. #tiles .. " tiles)")
        return
    end

    -- Enemy target skills (Earthen Bind, Mana Drain, Soul Rend): Tab to cycle, Space to confirm
    if skill.skillType == "enemy_target" then
        local targets = {}
        local tiles = {}
        local enemies = GetAllEnemies()
        if enemies then
            for _, eID in ipairs(enemies) do
                local ex, ey = GetEntityGridPosition(eID)
                if ex and ey then
                    table.insert(targets, eID)
                    table.insert(tiles, {x = ex, y = ey})
                end
            end
        end
        if #targets == 0 then
            print("[PlayerScript] No enemies to target")
            PulseTile(currentX, currentY, 0.3, 1.0, 0.5, 0.0)
            return
        end
        local idx = 1
        enemyTargetMode = { skillID = skillID, targets = targets, currentIndex = idx, selectedEnemy = targets[idx] }
        activePreview = { skillID = skillID, tiles = tiles }
        -- Tint: dim all, highlight first (selected=bold red, unselected=very dim)
        for i, t in ipairs(tiles) do
            TintTile(t.x, t.y, (i == idx) and 1.0 or 0.15, (i == idx) and 0.0 or 0.1, (i == idx) and 0.0 or 0.1, (i == idx) and 0.95 or 0.35)
        end
        print("[PlayerScript] Enemy target mode: Tab/Shift+Tab to cycle, Space to execute")
        return
    end

    -- Ally target skills (Knight's Oath, Soul Merge): Tab to cycle, Space to confirm
    if skill.skillType == "ally_target" then
        local targets = {}
        local tiles = {}
        local allPlayers = GetAllPlayers()
        if allPlayers then
            for _, pid in ipairs(allPlayers) do
                if pid ~= entityID then
                    local px, py = GetEntityGridPosition(pid)
                    if px and py then
                        table.insert(targets, pid)
                        table.insert(tiles, {x = px, y = py})
                    end
                end
            end
        end
        if #targets == 0 then
            print("[PlayerScript] No allies to target")
            PulseTile(currentX, currentY, 0.3, 1.0, 0.5, 0.0)
            return
        end
        local idx = 1
        allyTargetMode = { skillID = skillID, targets = targets, currentIndex = idx, selectedAlly = targets[idx] }
        activePreview = { skillID = skillID, tiles = tiles }
        -- Tint: dim all, highlight first (selected=bold green, unselected=very dim)
        for i, t in ipairs(tiles) do
            TintTile(t.x, t.y, (i == idx) and 0.0 or 0.1, (i == idx) and 1.0 or 0.15, (i == idx) and 0.0 or 0.1, (i == idx) and 0.95 or 0.35)
        end
        print("[PlayerScript] Ally target mode: Tab/Shift+Tab to cycle, Space to confirm")
        return
    end

    -- Standard pattern-based skills (melee, melee_cc, melee_debuff)
    local pattern = SkillPatterns.GetPattern(skill.pattern, currentAnimDirection, skill.range, isFlippedX)
    local tiles = {}

    -- Choose tint color based on skill type
    local tR, tG, tB = 1.0, 0.3, 0.3  -- red for damage
    if skill.skillType == "melee_cc" then
        tR, tG, tB = 1.0, 1.0, 0.3    -- yellow for CC
    elseif skill.skillType == "melee_debuff" then
        tR, tG, tB = 0.8, 0.3, 1.0    -- purple for debuff
    end

    for _, offset in ipairs(pattern) do
        local tileX = currentX + offset.x
        local tileY = currentY + offset.y
        if IsValidGridPosition(tileX, tileY) then
            TintTile(tileX, tileY, tR, tG, tB, 0.7)
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
    dashMode = nil
    allyTargetMode = nil
    enemyTargetMode = nil
    lastTabKeyDown = false
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

-- Helper: consume attack AP and trigger UI animation
local function consumeAttackAPAndAnimate(cost)
    -- Dark Omens Triggered: free skills cost 0 AP
    if darkOmensSkillsRemaining > 0 then
        print("[PlayerScript] Dark Omens: skill costs 0 AP (" .. darkOmensSkillsRemaining .. " remaining)")
        return
    end

    -- Siphon Charge: consume ALL remaining AP instead of normal cost
    if siphonTriggeredThisSkill then
        local remainingAP = GetEntityAttackAP(entityID)
        if remainingAP and remainingAP > 0 then
            ConsumeEntityAttackAP(entityID, remainingAP)
            for i = 1, remainingAP do
                if UIManager and UIManager.GetComponent then
                    local comp = UIManager.GetComponent("attackAP")
                    if comp and comp.ConsumeOneAP then
                        pcall(function() comp:ConsumeOneAP() end)
                    end
                else
                    pcall(function() TriggerAttackAPAnimation() end)
                end
            end
            print("[PlayerScript] Siphon Charge: consumed all " .. remainingAP .. " AP")
        end
        siphonTriggeredThisSkill = false
        return
    end

    ConsumeEntityAttackAP(entityID, cost)
    for i = 1, cost do
        if UIManager and UIManager.GetComponent then
            local comp = UIManager.GetComponent("attackAP")
            if comp and comp.ConsumeOneAP then
                pcall(function() comp:ConsumeOneAP() end)
            end
        else
            pcall(function() TriggerAttackAPAnimation() end)
        end
    end
end

-- Helper: play the attack animation
local function playAttackAnimation()
    currentAnimGroup = AnimGroup.Attack
    SetAnimationGroup(entityID, currentAnimGroup)
    SetAnimationLoop(entityID, false)
end

-- Helper: face toward a grid position
local function faceToward(targetX, targetY)
    local px, py = GetEntityGridPosition(entityID)
    if not px or not py then return end
    local dx = targetX - px
    local dy = targetY - py
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

-- Execute any skill based on its skillType
-- Helper: heal a specific entity by amount (capped at max HP)
local function healEntity(targetID, amount)
    local hp, maxHP = GetEntityHP(targetID)
    if hp and maxHP and hp > 0 and hp < maxHP then
        local newHP = math.min(hp + amount, maxHP)
        SetEntityHP(targetID, newHP)
        return true
    end
    return false
end

-- Helper: get bonus damage from soulMergeBuff
local function getSoulMergeBonusDamage()
    if HasStatusEffect and HasStatusEffect(entityID, "soulMergeBuff") then
        return 1
    end
    return 0
end

-- Helper: after damaging an enemy, check for soulRend (heal all players) and kill heal
-- hadSoulRend: must be checked BEFORE DamageEntity, because killing the enemy destroys the entity
--              and HasStatusEffect would fail on a destroyed entity
local function checkPostDamageEffects(enemyID, hadSoulRend)
    -- Soul Rend: if enemy HAD soulRend (before damage), heal all players for 1 HP
    -- Works even when the attack kills the enemy
    if hadSoulRend then
        local allPlayers = GetAllPlayers()
        if allPlayers then
            for _, pid in ipairs(allPlayers) do
                if healEntity(pid, 1) then
                    print("[PlayerScript] Soul Rend: healed player " .. pid .. " for 1 HP")
                end
            end
        end
    end

    -- Soul Merge Buff: if caster has soulMergeBuff and enemy died, heal caster for 1 HP
    if HasStatusEffect and HasStatusEffect(entityID, "soulMergeBuff") then
        local hp = GetEntityHP(enemyID)
        if hp and hp <= 0 then
            if healEntity(entityID, 1) then
                print("[PlayerScript] Soul Merge: kill heal +1 HP for player " .. entityID)
            end
        end
    end
end

-- Helper: damage an enemy with soulMergeBuff bonus and post-damage effects
local function damageEnemyWithEffects(enemyID, baseDamage)
    local damage = baseDamage + getSoulMergeBonusDamage() + currentWarcryBonus
    -- Check soulRend BEFORE damage: if attack kills enemy, entity is destroyed and HasStatusEffect would fail
    local hadSoulRend = HasStatusEffect and HasStatusEffect(enemyID, "soulRend")
    local success = DamageEntity(enemyID, damage)
    if success then
        checkPostDamageEffects(enemyID, hadSoulRend)
    end
    return success
end

function ExecuteSkill(skillID)
    local skill = SkillDefs[skillID]
    if not skill then
        ClearActivePreview()
        return
    end

    print("[PlayerScript] ===== EXECUTING: " .. skill.name .. " =====")

    -- Dark Omens Triggered: skills cost 0 AP, but only 2 skills allowed
    local darkOmensFreeSkill = (darkOmensSkillsRemaining > 0)
    if darkOmensFreeSkill and darkOmensSkillsRemaining <= 0 then
        print("[PlayerScript] Dark Omens: no more free skills this turn!")
        ClearActivePreview()
        return
    end

    -- Check AP (skip if Dark Omens free skill)
    local currentAP, maxAP = GetEntityAttackAP(entityID)
    if not darkOmensFreeSkill then
        if not currentAP or currentAP < skill.apCost then
            print("[PlayerScript] Not enough Attack AP (" .. tostring(currentAP) .. " < " .. skill.apCost .. ")")
            ClearActivePreview()
            return
        end
    end

    -- Check HP cost (Berserker skills)
    local hpCost = skill.hpCost or 0
    if hpCost > 0 then
        local currentHP, maxHP = GetEntityHP(entityID)
        if not currentHP or currentHP <= hpCost then
            print("[PlayerScript] Not enough HP for " .. skill.name .. " (HP:" .. tostring(currentHP) .. " <= cost:" .. hpCost .. ")")
            ClearActivePreview()
            return
        end
    end

    -- Dark Omens: once per level check
    if skill.oncePerLevel and darkOmensUsedThisLevel then
        print("[PlayerScript] " .. skill.name .. " already used this level!")
        ClearActivePreview()
        return
    end

    -- Siphon Charge: if active, first attack consumes all AP and heals 3 HP
    siphonTriggeredThisSkill = false
    if siphonChargeActive and skill.damage and skill.damage > 0 then
        print("[PlayerScript] Siphon Charge triggered! Will consume all AP and heal 3 HP")
        siphonTriggeredThisSkill = true
        siphonChargeActive = false
        RemoveStatusEffect(entityID, "siphonCharge")
        healEntity(entityID, 3)
    end

    -- Deduct HP cost
    if hpCost > 0 then
        local currentHP = GetEntityHP(entityID)
        SetEntityHP(entityID, currentHP - hpCost)
        print("[PlayerScript] " .. skill.name .. " HP cost: -" .. hpCost .. " HP (now " .. (currentHP - hpCost) .. ")")
    end

    -- Bloody Warcry: next skill gets +1 damage and costs 1 HP
    currentWarcryBonus = 0
    if bloodyWarcryDamageBonus and skill.damage and skill.damage > 0 then
        currentWarcryBonus = 1
        bloodyWarcryDamageBonus = false
        RemoveStatusEffect(entityID, "bloodyWarcry")
        local currentHP = GetEntityHP(entityID)
        if currentHP and currentHP > 1 then
            SetEntityHP(entityID, currentHP - 1)
            print("[PlayerScript] Bloody Warcry: +1 damage, -1 HP (now " .. (currentHP - 1) .. ")")
        end
    end

    -- Dark Omens Triggered: track skill usage (2 free skills)
    if darkOmensSkillsRemaining > 0 then
        darkOmensSkillsRemaining = darkOmensSkillsRemaining - 1
        print("[PlayerScript] Dark Omens: " .. darkOmensSkillsRemaining .. " free skills remaining")
    end

    -- ================================================================
    -- SELF-BUFF skills (Guard, Parry, Siphon Charge, Futile Resistance, Dark Omens, Bloody Warcry)
    -- ================================================================
    if skill.skillType == "self_buff" then
        ApplyStatusEffect(entityID, skill.effect, skill.duration, entityID)

        -- Dark Omens: mark as used this level
        if skill.effect == "darkOmens" then
            darkOmensUsedThisLevel = true
        end

        -- Siphon Charge: activate for next attack
        if skill.effect == "siphonCharge" then
            siphonChargeActive = true
        end

        -- Bloody Warcry: apply warcry buff to ALL party members
        if skill.effect == "bloodyWarcry" then
            local allPlayers = GetAllPlayers()
            if allPlayers then
                for _, pid in ipairs(allPlayers) do
                    if pid ~= entityID then
                        ApplyStatusEffect(pid, "bloodyWarcry", skill.duration, entityID)
                    end
                end
            end
            print("[PlayerScript] Bloody Warcry: applied to all party members")
        end

        consumeAttackAPAndAnimate(skill.apCost)
        print("[PlayerScript] " .. skill.name .. ": applied '" .. skill.effect .. "' to self")
        playAttackAnimation()
        ClearActivePreview()
        return
    end

    -- ================================================================
    -- MELEE CC skills (Bash - stun)
    -- ================================================================
    if skill.skillType == "melee_cc" then
        local enemies = FindEnemiesInPattern(skillID)
        if #enemies == 0 then
            print("[PlayerScript] No enemies in " .. skill.name .. " range")
            ClearActivePreview()
            return
        end

        faceToward(enemies[1].x, enemies[1].y)

        -- Apply CC effect to first adjacent enemy found
        local target = enemies[1]
        ApplyStatusEffect(target.id, skill.effect, skill.effectDuration, entityID)
        PulseTile(target.x, target.y, 0.5, 1.0, 1.0, 0.0)  -- yellow pulse for CC

        -- Apply damage if any
        if skill.damage and skill.damage > 0 then
            damageEnemyWithEffects(target.id, skill.damage)
        end

        consumeAttackAPAndAnimate(skill.apCost)
        print("[PlayerScript] " .. skill.name .. ": applied '" .. skill.effect .. "' to enemy " .. target.id)
        playAttackAnimation()
        ClearActivePreview()
        return
    end

    -- ================================================================
    -- MELEE DEBUFF skills (Exploit Weakness - vulnerability)
    -- ================================================================
    if skill.skillType == "melee_debuff" then
        local enemies = FindEnemiesInPattern(skillID)
        if #enemies == 0 then
            print("[PlayerScript] No enemies in " .. skill.name .. " range")
            ClearActivePreview()
            return
        end

        faceToward(enemies[1].x, enemies[1].y)

        -- Apply debuff to first adjacent enemy
        local target = enemies[1]
        local extraDmg = skill.extraDamage or 1
        ApplyStatusEffect(target.id, skill.effect, skill.effectDuration, entityID, 0, extraDmg)
        PulseTile(target.x, target.y, 0.5, 0.8, 0.3, 1.0)  -- purple pulse for debuff

        -- Apply damage if any
        if skill.damage and skill.damage > 0 then
            damageEnemyWithEffects(target.id, skill.damage)
        end

        consumeAttackAPAndAnimate(skill.apCost)
        print("[PlayerScript] " .. skill.name .. ": applied '" .. skill.effect .. "' (+" .. extraDmg .. " dmg) to enemy " .. target.id)
        playAttackAnimation()
        ClearActivePreview()
        return
    end

    -- ================================================================
    -- DASH skills (Swift Blow)
    -- ================================================================
    if skill.skillType == "dash" then
        if not dashMode or (dashMode.dirX == 0 and dashMode.dirY == 0) then
            print("[PlayerScript] Dash: no direction selected yet (use WASD)")
            return  -- don't clear preview, wait for direction
        end

        local px, py = GetEntityGridPosition(entityID)
        if not px or not py then
            ClearActivePreview()
            return
        end

        local dirX, dirY = dashMode.dirX, dashMode.dirY
        local dashRange = skill.dashRange or 3
        local enemiesHit = 0
        local landX, landY = px, py

        -- Walk along the dash path (passes through enemies, stops at walls)
        for i = 1, dashRange do
            local nextX = px + dirX * i
            local nextY = py + dirY * i

            if not IsValidGridPosition(nextX, nextY) then
                break  -- out of bounds
            end

            local walkable = IsWalkableTile(nextX, nextY)
            local occupied = IsTileOccupied(nextX, nextY)

            if not walkable and not occupied then
                break  -- hit a wall, stop before this tile
            end

            if occupied then
                -- Enemy at this tile: damage them and dash through
                local enemies = GetAllEnemies()
                if enemies then
                    for _, eID in ipairs(enemies) do
                        local ex, ey = GetEntityGridPosition(eID)
                        if ex == nextX and ey == nextY then
                            damageEnemyWithEffects(eID, skill.damage)
                            PulseTile(nextX, nextY, 0.5, 1.0, 0.0, 0.0)
                            enemiesHit = enemiesHit + 1
                            break
                        end
                    end
                end
                -- Don't update landX/landY - can't land on an enemy
            else
                -- Empty tile: can land here
                landX = nextX
                landY = nextY
            end
        end

        -- Move player to landing position
        if landX ~= px or landY ~= py then
            MoveEntityToTile(entityID, landX, landY)
        end

        consumeAttackAPAndAnimate(skill.apCost)
        print("[PlayerScript] " .. skill.name .. ": dashed to (" .. landX .. "," .. landY .. "), hit " .. enemiesHit .. " enemies")
        playAttackAnimation()
        ClearActivePreview()
        return
    end

    -- ================================================================
    -- ALLY TARGET skills (Knight's Oath, Soul Merge, Cannibalism)
    -- ================================================================
    if skill.skillType == "ally_target" then
        if not allyTargetMode or not allyTargetMode.selectedAlly then
            print("[PlayerScript] Ally target: no ally selected yet (click an ally)")
            return  -- don't clear preview, wait for selection
        end

        local allyID = allyTargetMode.selectedAlly

        if skill.effect == "cannibalism" then
            -- Cannibalism: ally loses 1 HP, caster heals 2 HP
            local allyHP = GetEntityHP(allyID)
            if allyHP and allyHP > 1 then
                SetEntityHP(allyID, allyHP - 1)
                healEntity(entityID, 2)
                print("[PlayerScript] Cannibalism: ally " .. allyID .. " lost 1 HP, healed self for 2 HP")
            else
                print("[PlayerScript] Cannibalism: ally " .. allyID .. " HP too low (" .. tostring(allyHP) .. ")")
                ClearActivePreview()
                return
            end
            consumeAttackAPAndAnimate(skill.apCost)
            playAttackAnimation()
            ClearActivePreview()
            return
        end

        if skill.effect == "soulMerge" then
            -- Soul Merge: sacrifice self, permanently buff the ally
            -- 1. Mark self as merged (permanent - turn will be skipped)
            ApplyStatusEffect(entityID, "soulMerge", -1, entityID)

            -- 2. Buff the ally
            ApplyStatusEffect(allyID, "soulMergeBuff", -1, entityID, allyID)

            -- 3. +2 Health (increase current and max)
            local allyHP, allyMaxHP = GetEntityHP(allyID)
            if allyHP and allyMaxHP then
                SetEntityHP(allyID, allyHP + 2, allyMaxHP + 2)
                print("[PlayerScript] Soul Merge: ally " .. allyID .. " HP " .. allyHP .. " -> " .. (allyHP + 2) .. " (max " .. (allyMaxHP + 2) .. ")")
            end

            -- 4. +1 Movement AP (add to current; PartyTurnManager handles future turns)
            ConsumeEntityAP(allyID, -1)

            -- 5. +1 Attack AP (add to current; PartyTurnManager handles future turns)
            ConsumeEntityAttackAP(allyID, -1)

            consumeAttackAPAndAnimate(skill.apCost)
            print("[PlayerScript] Soul Merge: " .. entityID .. " sacrificed for ally " .. allyID)
            playAttackAnimation()
            ClearActivePreview()
            return
        end

        -- Default ally_target behavior (Knight's Oath)
        ApplyStatusEffect(allyID, skill.effect, skill.duration, entityID, allyID)
        consumeAttackAPAndAnimate(skill.apCost)
        print("[PlayerScript] " .. skill.name .. ": protecting ally " .. allyID .. " for " .. skill.duration .. " turns")
        playAttackAnimation()
        ClearActivePreview()
        return
    end

    -- ================================================================
    -- GROUNDSHATTER (5x5 AoE centered on self, hits ALL characters, stuns self)
    -- ================================================================
    if skill.skillType == "groundshatter" then
        local px, py = GetEntityGridPosition(entityID)
        if not px or not py then
            ClearActivePreview()
            return
        end

        local totalHit = 0

        -- Damage all enemies in 5x5 area
        local enemies = GetAllEnemies()
        if enemies then
            for _, eID in ipairs(enemies) do
                local ex, ey = GetEntityGridPosition(eID)
                if ex and ey and math.abs(ex - px) <= 2 and math.abs(ey - py) <= 2 then
                    damageEnemyWithEffects(eID, skill.damage)
                    PulseTile(ex, ey, 0.5, 1.0, 0.0, 0.0)
                    totalHit = totalHit + 1
                end
            end
        end

        -- Damage all allies in 5x5 area (including self excluded from pattern but still affected)
        local allPlayers = GetAllPlayers()
        if allPlayers then
            for _, pid in ipairs(allPlayers) do
                if pid ~= entityID then
                    local ax, ay = GetEntityGridPosition(pid)
                    if ax and ay and math.abs(ax - px) <= 2 and math.abs(ay - py) <= 2 then
                        local allyHP = GetEntityHP(pid)
                        if allyHP and allyHP > 0 then
                            SetEntityHP(pid, math.max(allyHP - skill.damage, 0))
                            PulseTile(ax, ay, 0.5, 1.0, 0.5, 0.0)
                            totalHit = totalHit + 1
                            print("[PlayerScript] Groundshatter: ally " .. pid .. " took " .. skill.damage .. " damage")
                        end
                    end
                end
            end
        end

        -- Stun self for next turn
        ApplyStatusEffect(entityID, "stun", 1, entityID)

        consumeAttackAPAndAnimate(skill.apCost)
        print("[PlayerScript] Groundshatter: hit " .. totalHit .. " characters for " .. skill.damage .. " damage, self stunned")
        playAttackAnimation()
        ClearActivePreview()
        return
    end

    -- ================================================================
    -- GLOBAL DAMAGE skills (Lightning Strike)
    -- ================================================================
    if skill.skillType == "global_damage" then
        local enemies = GetAllEnemies()
        local enemiesHit = 0
        if enemies then
            for _, eID in ipairs(enemies) do
                local success = damageEnemyWithEffects(eID, skill.damage)
                if success then
                    enemiesHit = enemiesHit + 1
                    local ex, ey = GetEntityGridPosition(eID)
                    if ex and ey then
                        PulseTile(ex, ey, 0.5, 1.0, 1.0, 0.0)  -- yellow pulse for lightning
                    end
                end
            end
        end
        consumeAttackAPAndAnimate(skill.apCost)
        print("[PlayerScript] " .. skill.name .. ": hit " .. enemiesHit .. " enemies for " .. skill.damage .. " damage each")
        playAttackAnimation()
        ClearActivePreview()
        return
    end

    -- ================================================================
    -- SELF-OVERLOAD skills (Overload)
    -- ================================================================
    if skill.skillType == "self_overload" then
        local apGain = skill.apGain or 2
        -- Gain AP immediately (consume negative = add)
        ConsumeEntityAttackAP(entityID, -apGain)
        -- Apply overload: next turn AP won't refill
        ApplyStatusEffect(entityID, "overload", 1, entityID)
        if skill.apCost > 0 then
            consumeAttackAPAndAnimate(skill.apCost)
        end
        print("[PlayerScript] " .. skill.name .. ": gained " .. apGain .. " AP, next turn AP won't refill")
        playAttackAnimation()
        ClearActivePreview()
        return
    end

    -- ================================================================
    -- ENEMY TARGET skills (Earthen Bind, Mana Drain, Soul Rend)
    -- ================================================================
    if skill.skillType == "enemy_target" then
        if not enemyTargetMode or not enemyTargetMode.selectedEnemy then
            print("[PlayerScript] Enemy target: no enemy selected yet (click an enemy)")
            return  -- don't clear preview, wait for selection
        end

        local targetID = enemyTargetMode.selectedEnemy
        local ex, ey = GetEntityGridPosition(targetID)

        -- Apply the skill's effect to the target enemy
        local extra = skill.extraData or 0
        ApplyStatusEffect(targetID, skill.effect, skill.effectDuration, entityID, 0, extra)
        if ex and ey then
            PulseTile(ex, ey, 0.5, 0.8, 0.3, 1.0)  -- purple pulse for debuff
        end

        -- Apply damage if any
        if skill.damage and skill.damage > 0 then
            damageEnemyWithEffects(targetID, skill.damage)
        end

        consumeAttackAPAndAnimate(skill.apCost)
        print("[PlayerScript] " .. skill.name .. ": applied '" .. skill.effect .. "' to enemy " .. targetID)
        playAttackAnimation()
        ClearActivePreview()
        return
    end

    -- ================================================================
    -- PROJECTILE skills (Fireball, PiercingShot)
    -- ================================================================
    if skill.skillType == "projectile" then
        local px, py = GetEntityGridPosition(entityID)
        if not px or not py then
            ClearActivePreview()
            return
        end

        local worldX, worldY = TileToWorld(px, py)
        if not worldX or not worldY then
            ClearActivePreview()
            return
        end

        local gridDirX, gridDirY = getFacingWorldDirection()
        local targetWorldX, targetWorldY = TileToWorld(px + gridDirX, py + gridDirY)
        local dirX, dirY = 0, 0
        if targetWorldX and targetWorldY then
            dirX = targetWorldX - worldX
            dirY = targetWorldY - worldY
            local len = math.sqrt(dirX * dirX + dirY * dirY)
            if len > 0 then dirX = dirX / len; dirY = dirY / len end
        end

        if dirX == 0 and dirY == 0 then
            dirX, dirY = gridDirX, gridDirY
        end

        local tintR, tintG, tintB, tintA = 1, 1, 1, 1
        local spritePath = nil
        if getPlayerIndex() == 2
           and (skillID == "Fireball" or skillID == "PiercingShot") then
            tintR, tintG, tintB, tintA = 1, 0, 0, 1
            spritePath = ""
        end

        local projID = SpawnSkillProjectile(
            worldX, worldY,
            dirX, dirY,
            skill.projSpeed or 3.0,
            skill.damage or 1,
            skill.pierce or false,
            tintR, tintG, tintB, tintA,
            spritePath,
            false,    -- isEnemyProjectile
            entityID  -- sourceEntityID (for Soul Rend, Soul Merge kill heal)
        )
        if not projID then
            print("[PlayerScript] ERROR: SpawnSkillProjectile returned nil")
            ClearActivePreview()
            return
        end

        print("[PlayerScript] Spawned projectile ID=" .. tostring(projID)
            .. " dir=(" .. dirX .. "," .. dirY .. ")"
            .. " speed=" .. (skill.projSpeed or 3.0)
            .. " dmg=" .. skill.damage
            .. " pierce=" .. tostring(skill.pierce or false))

        -- Bladed Whirlwind: apply DOT to enemies in the projectile path
        if skill.effect and skill.effectDuration then
            local enemies = GetAllEnemies()
            if enemies then
                local range = skill.range or 7
                for _, eID in ipairs(enemies) do
                    local ex, ey = GetEntityGridPosition(eID)
                    if ex and ey then
                        -- Check if enemy is in the line of fire
                        local dx = ex - px
                        local dy = ey - py
                        local inLine = false
                        if gridDirX ~= 0 and gridDirY == 0 then
                            inLine = (dy == 0 and dx * gridDirX > 0 and math.abs(dx) <= range)
                        elseif gridDirY ~= 0 and gridDirX == 0 then
                            inLine = (dx == 0 and dy * gridDirY > 0 and math.abs(dy) <= range)
                        end
                        if inLine then
                            ApplyStatusEffect(eID, skill.effect, skill.effectDuration, entityID)
                            print("[PlayerScript] " .. skill.name .. ": applied '" .. skill.effect .. "' DOT to enemy " .. eID)
                        end
                    end
                end
            end
        end

        consumeAttackAPAndAnimate(skill.apCost)
        playAttackAnimation()
        ClearActivePreview()
        return
    end

    -- ================================================================
    -- MELEE skills (Thrust, Sweeping Slash, and any default)
    -- ================================================================
    local enemies = FindEnemiesInPattern(skillID)
    if #enemies == 0 then
        print("[PlayerScript] No enemies in " .. skill.name .. " range")
        ClearActivePreview()
        return
    end

    faceToward(enemies[1].x, enemies[1].y)

    local enemiesHit = 0
    for _, enemy in ipairs(enemies) do
        local success = damageEnemyWithEffects(enemy.id, skill.damage)
        if success then
            enemiesHit = enemiesHit + 1
            PulseTile(enemy.x, enemy.y, 0.5, 1.0, 0.0, 0.0)

            local hpAfter = GetEntityHP(enemy.id)
            if hpAfter and hpAfter <= 0 then
                print("[PlayerScript] Enemy " .. enemy.id .. " DEFEATED!")
            end
        end
    end

    consumeAttackAPAndAnimate(skill.apCost)
    print("[PlayerScript] " .. skill.name .. ": hit " .. enemiesHit .. " enemies for " .. skill.damage .. " damage each")
    playAttackAnimation()
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