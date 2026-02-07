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


-- Attack state tracking
local lastSpaceKeyDown = false
local attackPreviewActive = false
local attackPreviewTiles = {}
local attackRange = 1
local attackAPCost = 1

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
    if IsKeyDown("Space") then
        lastSpaceKeyDown = true
        print("[PlayerScript]   SPACE is held - will ignore until released")
    else
        lastSpaceKeyDown = false
    end
    if next(blockedKeys) == nil and not lastPKeyDown and not lastSpaceKeyDown then
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
    lastSpaceKeyDown = false
    -- Reset movement key states
    lastWKeyDown = false
    lastSKeyDown = false
    lastAKeyDown = false
    lastDKeyDown = false
    ClearAttackPreview()
end

-- ============================================================================
-- ATTACK HELPER FUNCTIONS 
-- ============================================================================

function ShowAttackPreview()
    ClearAttackPreview()

    local currentX, currentY = GetEntityGridPosition(entityID)
    if not currentX or not currentY then
        print("[PlayerScript] ERROR: Cannot get position for attack preview")
        return
    end

    print("[PlayerScript] Showing attack preview at (" .. currentX .. ", " .. currentY .. ") with range " .. attackRange)

    for r = 1, attackRange do
        local candidates = {
            {x = currentX + r, y = currentY},
            {x = currentX - r, y = currentY},
            {x = currentX, y = currentY + r},
            {x = currentX, y = currentY - r}
        }

        for _, tile in ipairs(candidates) do
            if IsValidGridPosition(tile.x, tile.y) and IsWalkableTile(tile.x, tile.y) then
                local worldX, worldY = TileToWorld(tile.x, tile.y)

                if worldX and worldY then
                    -- Tint the tile red for attack preview
                    TintTile(tile.x, tile.y, 1.0, 0.3, 0.3, 0.7)  -- Red tint with 70% opacity
                    table.insert(attackPreviewTiles, {x = tile.x, y = tile.y})  -- Store tile coords
                    print("[PlayerScript]   Tinted attack preview tile at grid(" .. tile.x .. ", " .. tile.y .. ")")
                else
                    print("[PlayerScript]   WARNING: TileToWorld failed for (" .. tile.x .. ", " .. tile.y .. ")")
                end
            end
        end
    end

    if #attackPreviewTiles > 0 then
        attackPreviewActive = true
        print("[PlayerScript] Attack preview active with " .. #attackPreviewTiles .. " indicator entities")
    else
        print("[PlayerScript] WARNING: No valid attack preview tiles found")
    end
end

function ClearAttackPreview()
    if #attackPreviewTiles > 0 then
        print("[PlayerScript] Clearing " .. #attackPreviewTiles .. " attack preview tiles")
        for _, tile in ipairs(attackPreviewTiles) do
            if tile and tile.x and tile.y then
                -- Reset tile tint to white (no tint)
                TintTile(tile.x, tile.y, 1.0, 1.0, 1.0, 1.0)
                print("[PlayerScript]   Cleared tint on tile (" .. tile.x .. ", " .. tile.y .. ")")
            end
        end
    end

    attackPreviewTiles = {}
    attackPreviewActive = false
end

function FindEnemyInRange()
    local currentX, currentY = GetEntityGridPosition(entityID)
    if not currentX or not currentY then
        return nil
    end

    local enemies = GetAllEnemies()
    if not enemies or #enemies == 0 then
        print("[PlayerScript] No enemies found")
        return nil
    end

    print("[PlayerScript] Searching for enemies in range " .. attackRange .. " from (" .. currentX .. ", " .. currentY .. ")")

    local closestEnemy = nil
    local closestEnemyX, closestEnemyY = nil, nil
    local closestDistance = 999999

    for _, enemyID in ipairs(enemies) do
        local enemyX, enemyY = GetEntityGridPosition(enemyID)
        if enemyX and enemyY then
            local distance = math.abs(enemyX - currentX) + math.abs(enemyY - currentY)
            print("[PlayerScript]   Enemy " .. enemyID .. " at (" .. enemyX .. ", " .. enemyY .. ") - distance: " .. distance)

            if distance <= attackRange and distance < closestDistance then
                closestEnemy = enemyID
                closestEnemyX = enemyX
                closestEnemyY = enemyY
                closestDistance = distance
            end
        end
    end

    if closestEnemy then
        print("[PlayerScript] Found closest enemy " .. closestEnemy .. " at (" .. closestEnemyX .. ", " .. closestEnemyY .. ") - distance: " .. closestDistance)
        return closestEnemy, closestEnemyX, closestEnemyY
    end

    print("[PlayerScript] No enemy in attack range")
    return nil
end

function ExecuteAttack()
    print("============================================================")
    print("[PlayerScript] ===== EXECUTING ATTACK =====")
    print("============================================================")

    -- Use ATTACK AP, not movement AP
    local currentAttackAP, maxAttackAP = GetEntityAttackAP(entityID)
    print("[PlayerScript] Current Attack AP: " .. tostring(currentAttackAP) .. "/" .. tostring(maxAttackAP) .. " (need " .. attackAPCost .. ")")

    if not currentAttackAP or currentAttackAP < attackAPCost then
        print("[PlayerScript] ATTACK BLOCKED: Not enough Attack AP (" .. tostring(currentAttackAP) .. " < " .. attackAPCost .. ")")
        ClearAttackPreview()
        return
    end

    print("[PlayerScript] Searching for enemy in range...")
    local enemyID, enemyX, enemyY = FindEnemyInRange()

    if not enemyID then
        print("[PlayerScript] ATTACK BLOCKED: No enemy in attack range!")
        print("============================================================")
        ClearAttackPreview()
        return
    end

    print("[PlayerScript] Target found: Enemy " .. enemyID .. " at (" .. enemyX .. ", " .. enemyY .. ")")
    print("[PlayerScript] Attacking enemy " .. enemyID .. " for " .. 1 .. " damage...")

    local attackDamage = 1
    local success = DamageEntity(enemyID, attackDamage)

    if success then
        print("[PlayerScript] Attack SUCCESS! Enemy " .. enemyID .. " damaged for " .. attackDamage .. " HP")

        -- Consume ATTACK AP, not movement AP
        ConsumeEntityAttackAP(entityID, attackAPCost)
        local newAttackAP = GetEntityAttackAP(entityID)
        print("[PlayerScript] Attack AP consumed. New Attack AP: " .. tostring(newAttackAP) .. "/" .. tostring(maxAttackAP))

        PulseTile(enemyX, enemyY, 0.5, 1.0, 0.0, 0.0)

        currentAnimGroup = AnimGroup.Attack
        SetAnimationGroup(entityID, currentAnimGroup)
        SetAnimationLoop(entityID, false)
        print("[PlayerScript] Playing attack animation")
    else
        print("[PlayerScript] Attack FAILED: DamageEntity returned false for enemy " .. enemyID)
    end

    print("============================================================")

    ClearAttackPreview()
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

            -- Update cooldown
            if moveCooldown > 0 then
                moveCooldown = moveCooldown - dt
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

            -- Check SPACE key for attack
            local spaceKeyDown = IsKeyDown("Space")
            if spaceKeyDown and not lastSpaceKeyDown then
                print("[PlayerScript] SPACE key pressed")

                if not attackPreviewActive then
                    -- Use ATTACK AP, not movement AP
                    local currentAttackAP, maxAttackAP = GetEntityAttackAP(entityID)
                    if currentAttackAP >= attackAPCost then
                        local testEnemy = FindEnemyInRange()
                        if testEnemy then
                            ShowAttackPreview()
                            print("[PlayerScript] Attack preview shown - enemy in range")
                        else
                            print("[PlayerScript] No enemy in attack range!")
                            PulseTile(currentX, currentY, 0.3, 1.0, 0.5, 0.0)
                        end
                    else
                        print("[PlayerScript] Not enough ATTACK AP to attack (" .. tostring(currentAttackAP) .. " < " .. attackAPCost .. ")")
                        PulseTile(currentX, currentY, 0.3, 1.0, 1.0, 0.3)
                    end
                else
                    ExecuteAttack()
                end
            end
            lastSpaceKeyDown = spaceKeyDown

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

            -- Clear attack preview if moving
            if attackPreviewActive then
                print("[PlayerScript] Clearing attack preview due to movement")
                ClearAttackPreview()
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
            -- Just became inactive
            lastActiveCheck = false
            hasLoggedActive = false
            blockedKeys = {}
            lastPKeyDown = false
            lastSpaceKeyDown = false
            -- Reset movement key states
            lastWKeyDown = false
            lastSKeyDown = false
            lastAKeyDown = false
            lastDKeyDown = false
            ClearAttackPreview()

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

    -- DISABLED: Debug animation triggers (causing errors)
    -- Uncomment if needed, but use IsKeyDown() not IsKeyPressed()
    --[[
    if IsKeyDown(75) then  -- KEY_K = Attack
        currentAnimGroup = AnimGroup.Attack
        SetAnimationGroup(entityID, currentAnimGroup)
        SetAnimationLoop(entityID, false)
        Log("[PlayerScript] Attack animation triggered")
    end

    if IsKeyDown(74) then  -- KEY_J = Injured
        currentAnimGroup = AnimGroup.Injured
        SetAnimationGroup(entityID, currentAnimGroup)
        SetAnimationLoop(entityID, false)
        Log("[PlayerScript] Injured animation triggered")
    end

    if IsKeyDown(76) then  -- KEY_L = Death
        currentAnimGroup = AnimGroup.Death
        SetAnimationGroup(entityID, currentAnimGroup)
        SetAnimationLoop(entityID, false)
        Log("[PlayerScript] Death animation triggered")
        -- Death animation blocks all movement
        return
    end
    ]]--

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

    -- ========================================================================
    -- ATTACK SYSTEM (SPACE KEY)
    -- ========================================================================

    -- Get THIS entity's current grid position (needed for attack preview check)
    local currentX, currentY = GetEntityGridPosition(entityID)
    if currentX == nil or currentY == nil then
        print("[PlayerScript] ERROR: Entity " .. entityID .. " position is nil! (currentX=" .. tostring(currentX) .. ", currentY=" .. tostring(currentY) .. ")")
        return  -- Entity position not available
    end

    -- Allow player to attack enemies with SPACE key
    -- First press: Show attack preview
    -- Second press: Execute attack
    local spaceKeyDown = IsKeyDown("Space")
    if spaceKeyDown and not lastSpaceKeyDown then
        -- SPACE key was just pressed
        print("============================================================")
        print("[PlayerScript] ===== SPACE KEY PRESSED =====")
        print("[PlayerScript] attackPreviewActive: " .. tostring(attackPreviewActive))

        if not attackPreviewActive then
            -- First press: Show attack preview
            local currentAP, maxAP = GetEntityAttackAP(entityID)
            print("[PlayerScript] Current Attack AP: " .. currentAP .. "/" .. maxAP .. " (attack cost: " .. attackAPCost .. ")")

            if currentAP >= attackAPCost then
                print("[PlayerScript] Sufficient AP - checking for enemies in range...")
                -- Check if there's an enemy in range before showing preview
                local testEnemy = FindEnemyInRange()
                print("[PlayerScript] FindEnemyInRange() returned: " .. tostring(testEnemy))

                if testEnemy then
                    print("[PlayerScript] Enemy found - calling ShowAttackPreview()...")
                    ShowAttackPreview()
                    print("[PlayerScript] Attack preview shown - enemy in range")
                else
                    print("[PlayerScript] No enemy in attack range!")
                    PulseTile(currentX, currentY, 0.3, 1.0, 0.5, 0.0)  -- Orange pulse (no target)
                end
            else
                print("[PlayerScript] Not enough AP to attack (" .. currentAP .. " < " .. attackAPCost .. ")")
                PulseTile(currentX, currentY, 0.3, 1.0, 1.0, 0.3)  -- Yellow pulse (not enough AP)
            end
        else
            -- Second press: Execute attack
            print("[PlayerScript] Attack preview already active - executing attack...")
            ExecuteAttack()
        end
        print("============================================================")
    end
    lastSpaceKeyDown = spaceKeyDown  -- Update SPACE key state for next frame

    -- ========================================================================
    -- MOVEMENT INPUT (DISABLED - FSM handles movement now)
    -- ========================================================================
    -- NOTE: This duplicate movement code is disabled because the FSM already
    --       handles all movement input. Keeping this active causes double
    --       EndCharacterTurn() calls and turn skipping bugs.

    --[[ DISABLED DUPLICATE MOVEMENT CODE
    -- Check for movement input
    local targetX, targetY = currentX, currentY
    local moveAttempted = false
    local moveDirX, moveDirY = 0, 0

    -- WASD input only (arrow keys disabled)
    local wDown = IsKeyDown("W") and not blockedKeys["W"]
    local sDown = IsKeyDown("S") and not blockedKeys["S"]
    local aDown = IsKeyDown("A") and not blockedKeys["A"]
    local dDown = IsKeyDown("D") and not blockedKeys["D"]

    if wDown then
        targetY = currentY + 1
        moveDirY = 1
        moveAttempted = true
    elseif sDown then
        targetY = currentY - 1
        moveDirY = -1
        moveAttempted = true
    elseif aDown then
        targetX = currentX - 1
        moveDirX = -1
        moveAttempted = true
    elseif dDown then
        targetX = currentX + 1
        moveDirX = 1
        moveAttempted = true
    end

    -- If no movement input, return to Idle
    if not moveAttempted then
        if currentAnimGroup == AnimGroup.Walk then
            currentAnimGroup = AnimGroup.Idle
            SetAnimationGroup(entityID, currentAnimGroup)
        end
        return
    end

    print("[PlayerScript] Movement attempted! Target: (" .. targetX .. ", " .. targetY .. ")")

    -- Clear attack preview if player moves
    if attackPreviewActive then
        print("[PlayerScript] Clearing attack preview due to movement")
        ClearAttackPreview()
    end

    -- ========================================================================
    -- MOVEMENT VALIDATION
    -- ========================================================================

    print("[PlayerScript] Step 1: Validating target position (" .. targetX .. ", " .. targetY .. ")...")

    -- Check if the target position is valid and walkable
    local isValid = IsValidGridPosition(targetX, targetY)
    print("[PlayerScript]   IsValidGridPosition: " .. tostring(isValid))
    if not isValid then
        print("[PlayerScript] FAILED: Invalid grid position!")
        return
    end

    local isWalkable = IsWalkableTile(targetX, targetY)
    print("[PlayerScript]   IsWalkableTile: " .. tostring(isWalkable))
    if not isWalkable then
        print("[PlayerScript] FAILED: Tile not walkable!")
        PulseTile(targetX, targetY, 0.3, 1.0, 0.3, 0.3)  -- Red pulse
        return
    end

    print("[PlayerScript] Step 1: PASSED - target is valid and walkable")

    -- ========================================================================
    -- AP CHECK
    -- ========================================================================

    print("[PlayerScript] Step 2: Checking AP...")

    -- Use entity-based AP API (supports party system)
    local currentAP, maxAP = GetEntityAP(entityID)
    print("[PlayerScript]   Entity " .. entityID .. " AP: " .. tostring(currentAP) .. "/" .. tostring(maxAP) .. " (need " .. apCostPerMove .. ")")

    if currentAP < apCostPerMove then
        print("[PlayerScript] FAILED: Not enough AP!")
        PulseTile(targetX, targetY, 0.3, 1.0, 1.0, 0.3)  -- Yellow pulse

        -- Automatically end character turn when AP depleted
        if currentAP == 0 then
            print("[PlayerScript] AP depleted - ending turn!")
            EndCharacterTurn()
            hasLoggedActive = false  -- Reset for next character
            lastActiveCheck = false  -- Reset active tracking
            blockedKeys = {}  -- Clear blocked keys
        end

        return
    end

    print("[PlayerScript] Step 2: PASSED - sufficient AP")

    -- ========================================================================
    -- EXECUTE MOVEMENT
    -- ========================================================================

    print("[PlayerScript] Step 3: Calling MoveEntityToTile(" .. entityID .. ", " .. targetX .. ", " .. targetY .. ")...")

    -- Move THIS specific entity (not just "the player")
    -- Use MoveEntityToTile instead of MovePlayerToTile for party system
    local success = MoveEntityToTile(entityID, targetX, targetY)

    print("[PlayerScript] Step 3: MoveEntityToTile returned: " .. tostring(success))

    if success then
    print("[PlayerScript] Step 4: Movement SUCCESS! Consuming AP...")

    -- ====================================================================
    -- UPDATE ANIMATION STATE BASED ON MOVEMENT (DO THIS BEFORE END TURN)
    -- ====================================================================

    local newDirection = currentAnimDirection
    local newFlipX = isFlippedX

    if moveDirY > 0 then
        newDirection = AnimDirection.Back
    elseif moveDirY < 0 then
        newDirection = AnimDirection.Front
    elseif moveDirX > 0 then
        newDirection = AnimDirection.Side
        newFlipX = false
    elseif moveDirX < 0 then
        newDirection = AnimDirection.Side
        newFlipX = true
    end

    if newDirection ~= currentAnimDirection then
        currentAnimDirection = newDirection
        SetAnimationDirection(entityID, currentAnimDirection)
    end

    if newFlipX ~= isFlippedX then
        isFlippedX = newFlipX
        SetAnimationFlipX(entityID, isFlippedX)
    end

    -- Switch to Walk animation (unless in special state)
    if currentAnimGroup ~= AnimGroup.Attack and
       currentAnimGroup ~= AnimGroup.Injured and
       currentAnimGroup ~= AnimGroup.Death then
        currentAnimGroup = AnimGroup.Walk
        SetAnimationGroup(entityID, currentAnimGroup)
        SetAnimationPlaying(entityID, true)
    end

    -- NOW consume AP
    ConsumeEntityAP(entityID, apCostPerMove)

    local newAP, maxAP = GetEntityAP(entityID)
    print("[PlayerScript] After movement: Entity " .. entityID .. " AP: " .. tostring(newAP) .. "/" .. tostring(maxAP))

    -- If AP depleted, end turn AFTER direction/flip has been applied
    if newAP == 0 then
        print("[PlayerScript] AP depleted after movement - ending turn!")

        -- Optional: go Idle, but KEEP direction/flip as just set
        currentAnimGroup = AnimGroup.Idle
        SetAnimationGroup(entityID, currentAnimGroup)

        EndCharacterTurn()
        hasLoggedActive = false
        lastActiveCheck = false
        blockedKeys = {}

        return
    end

    -- Visual feedback
    ShowTileBorder(targetX, targetY, 0.5)
    PulseTile(targetX, targetY, 0.3, 0.3, 1.0, 0.3)

    -- Check for chest collection
    if HasChestAtTile(targetX, targetY) then
        CollectChest(targetX, targetY)
        Log("[PlayerScript] Collected chest at (" .. targetX .. ", " .. targetY .. ")")
    end

    -- Check for goal completion
    if HasGoalAtTile(targetX, targetY) then
        Log("[PlayerScript] Reached goal! Level complete!")
    end

    moveCooldown = moveCooldownTime

    Log("[PlayerScript] Moved to (" .. targetX .. ", " .. targetY .. ") - AP remaining: " .. (currentAP - apCostPerMove))
else
    Log("[PlayerScript] Failed to move to (" .. targetX .. ", " .. targetY .. ")")
end
--]] -- END DISABLED DUPLICATE MOVEMENT CODE

end

-- ============================================================================
-- ATTACK HELPER FUNCTIONS
-- ============================================================================

function ShowAttackPreview()
    -- Clear any existing preview
    ClearAttackPreview()

    -- Get this entity's current position
    local currentX, currentY = GetEntityGridPosition(entityID)
    if not currentX or not currentY then
        print("[PlayerScript] ERROR: Cannot get position for attack preview")
        return
    end

    print("[PlayerScript] Showing attack preview at (" .. currentX .. ", " .. currentY .. ") with range " .. attackRange)

    -- Show attack preview tiles - only adjacent tiles (Manhattan distance = 1)
    -- Only show tiles at exactly distance 1 (4 adjacent tiles: up, down, left, right)
    local adjacentTiles = {
        {x = currentX + 1, y = currentY},      -- Right
        {x = currentX - 1, y = currentY},      -- Left
        {x = currentX, y = currentY + 1},      -- Down
        {x = currentX, y = currentY - 1}       -- Up
    }

    for _, tile in ipairs(adjacentTiles) do
        print("[PlayerScript]   Checking tile at (" .. tile.x .. ", " .. tile.y .. ")")

        local isValid = IsValidGridPosition and IsValidGridPosition(tile.x, tile.y) or true
        print("[PlayerScript]   IsValidGridPosition: " .. tostring(isValid))

        if isValid then
            -- Tint the tile red for attack preview
            print("[PlayerScript]     Calling TintTile(" .. tile.x .. ", " .. tile.y .. ") for attack preview")
            TintTile(tile.x, tile.y, 1.0, 0.3, 0.3, 0.7)  -- Red tint with 70% opacity

            -- Store the grid coordinates so we can clear them later
            table.insert(attackPreviewTiles, {x = tile.x, y = tile.y})
            print("[PlayerScript]   Tinted tile at grid(" .. tile.x .. ", " .. tile.y .. ")")
        else
            print("[PlayerScript]     Tile invalid, skipping")
        end
    end

    if #attackPreviewTiles > 0 then
        attackPreviewActive = true
        print("[PlayerScript] Attack preview active with " .. #attackPreviewTiles .. " tinted tiles")
    else
        print("[PlayerScript] WARNING: No valid attack preview tiles found")
    end
end

function ClearAttackPreview()
    if #attackPreviewTiles > 0 then
        print("[PlayerScript] Clearing " .. #attackPreviewTiles .. " attack preview tiles")
        for _, tile in ipairs(attackPreviewTiles) do
            if tile and tile.x and tile.y then
                -- Reset tile tint to white (no tint)
                TintTile(tile.x, tile.y, 1.0, 1.0, 1.0, 1.0)
                print("[PlayerScript]   Cleared tint on tile (" .. tile.x .. ", " .. tile.y .. ")")
            end
        end
    end

    -- Clear attack preview state
    attackPreviewTiles = {}
    attackPreviewActive = false
    print("[PlayerScript] Attack preview cleared")
end

function FindEnemyInRange()
    -- Get this entity's current position
    local currentX, currentY = GetEntityGridPosition(entityID)
    if not currentX or not currentY then
        return nil
    end

    -- Get all enemies
    local enemies = GetAllEnemies()
    if not enemies or #enemies == 0 then
        print("[PlayerScript] No enemies found")
        return nil
    end

    print("[PlayerScript] Searching for enemies in range " .. attackRange .. " from (" .. currentX .. ", " .. currentY .. ")")

    -- Find the closest enemy within attack range
    local closestEnemy = nil
    local closestEnemyX, closestEnemyY = nil, nil
    local closestDistance = 999999

    for _, enemyID in ipairs(enemies) do
        local enemyX, enemyY = GetEntityGridPosition(enemyID)
        if enemyX and enemyY then
            -- Calculate Manhattan distance
            local distance = math.abs(enemyX - currentX) + math.abs(enemyY - currentY)
            print("[PlayerScript]   Enemy " .. enemyID .. " at (" .. enemyX .. ", " .. enemyY .. ") - distance: " .. distance)

            if distance <= attackRange and distance < closestDistance then
                closestEnemy = enemyID
                closestEnemyX = enemyX
                closestEnemyY = enemyY
                closestDistance = distance
            end
        end
    end

    if closestEnemy then
        print("[PlayerScript] Found closest enemy " .. closestEnemy .. " at (" .. closestEnemyX .. ", " .. closestEnemyY .. ") - distance: " .. closestDistance)
        return closestEnemy, closestEnemyX, closestEnemyY
    end

    print("[PlayerScript] No enemy in attack range")
    return nil
end

function ExecuteAttack()
    print("============================================================")
    print("[PlayerScript] ===== EXECUTING ATTACK =====")
    print("============================================================")

    -- Check AP
    local currentAP, maxAP = GetEntityAttackAP(entityID)
    print("[PlayerScript] Current Attack AP: " .. tostring(currentAP) .. "/" .. tostring(maxAP) .. " (need " .. attackAPCost .. ")")

    if currentAP < attackAPCost then
        print("[PlayerScript] ATTACK BLOCKED: Not enough AP (" .. tostring(currentAP) .. " < " .. attackAPCost .. ")")
        ClearAttackPreview()
        return
    end

    -- Find enemy in range
    print("[PlayerScript] Searching for enemy in range...")
    local enemyID, enemyX, enemyY = FindEnemyInRange()

    if not enemyID then
        print("[PlayerScript] ATTACK BLOCKED: No enemy in attack range!")
        print("============================================================")
        ClearAttackPreview()
        return
    end

    print("[PlayerScript] Target found: Enemy " .. enemyID .. " at (" .. tostring(enemyX) .. ", " .. tostring(enemyY) .. ")")

        -- ============================================================
    -- FIX: Face the enemy before playing Attack_side animation
    -- (Knight side attack sheet is left-facing by default)
    -- ============================================================
    do
        local px, py = GetEntityGridPosition(entityID)
        if px and py and enemyX and enemyY then
            local dx = enemyX - px
            local dy = enemyY - py

            local newDir = currentAnimDirection
            local newFlip = isFlippedX

            -- Decide facing axis (attack is usually 1-tile away, but keep robust)
            if math.abs(dx) > math.abs(dy) then
                newDir = AnimDirection.Side

                -- IMPORTANT: for Knight_Attack_Left sheet,
                -- flip when enemy is on the RIGHT.
                if dx > 0 then
                    newFlip = true   -- enemy right -> flip to face right
                else
                    newFlip = false  -- enemy left  -> keep left
                end
            elseif dy > 0 then
                newDir = AnimDirection.Back
                -- keep newFlip unchanged
            elseif dy < 0 then
                newDir = AnimDirection.Front
                -- keep newFlip unchanged
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


    -- Check enemy HP BEFORE attack
    local enemyHPBefore, enemyMaxHP = GetEntityHP(enemyID)
    print("[PlayerScript] Enemy " .. enemyID .. " HP BEFORE attack: " .. tostring(enemyHPBefore) .. "/" .. tostring(enemyMaxHP))

    -- Deal damage
    local attackDamage = 1  -- Base damage
    print("[PlayerScript] Calling DamageEntity(" .. enemyID .. ", " .. attackDamage .. ")...")
    local success = DamageEntity(enemyID, attackDamage)

    print("[PlayerScript] DamageEntity returned: " .. tostring(success))

    if success then
        print("[PlayerScript] Attack SUCCESS! Enemy " .. enemyID .. " damaged for " .. attackDamage .. " HP")

        -- Check enemy HP AFTER attack
        local enemyHPAfter, _ = GetEntityHP(enemyID)
        print("[PlayerScript] Enemy " .. enemyID .. " HP AFTER attack: " .. tostring(enemyHPAfter))

        -- Check if enemy should be dead
        if enemyHPAfter and enemyHPAfter <= 0 then
            print("[PlayerScript] !!! ENEMY " .. enemyID .. " HP <= 0 - SHOULD BE DESTROYED BY C++ !!!")
        elseif not enemyHPAfter then
            print("[PlayerScript] !!! ENEMY " .. enemyID .. " HP is nil - ENTITY MAY HAVE BEEN DESTROYED !!!")
        else
            print("[PlayerScript] Enemy " .. enemyID .. " still alive with " .. tostring(enemyHPAfter) .. " HP")
        end

        -- Consume attack AP (NOT movement AP!)
        ConsumeEntityAttackAP(entityID, attackAPCost)
        local newAP, newMaxAP = GetEntityAttackAP(entityID)
        print("[PlayerScript] Attack AP consumed. New Attack AP: " .. tostring(newAP) .. "/" .. tostring(newMaxAP))

        -- Trigger attack AP crystal shattering animation
        print("[PlayerScript] ========== ANIMATION DEBUG ==========")
        print("[PlayerScript] Attempting to access UIManager directly from entity script...")
        print("[PlayerScript] UIManager type: " .. tostring(type(UIManager)))
        print("[PlayerScript] UIManager value: " .. tostring(UIManager))

        if UIManager then
            print("[PlayerScript] UIManager exists in entity Lua state!")
            print("[PlayerScript] UIManager.GetComponent type: " .. tostring(type(UIManager.GetComponent)))

            if UIManager.GetComponent then
                print("[PlayerScript] GetComponent method exists!")
                print("[PlayerScript] Calling UIManager.GetComponent('attackAP')...")

                local attackAPComponent = UIManager.GetComponent("attackAP")
                print("[PlayerScript] attackAP component type: " .. tostring(type(attackAPComponent)))
                print("[PlayerScript] attackAP component value: " .. tostring(attackAPComponent))

                if attackAPComponent then
                    print("[PlayerScript] attackAP component exists!")
                    print("[PlayerScript] ConsumeOneAP type: " .. tostring(type(attackAPComponent.ConsumeOneAP)))

                    if attackAPComponent.ConsumeOneAP then
                        print("[PlayerScript] ConsumeOneAP method exists!")
                        print("[PlayerScript] Calling attackAP:ConsumeOneAP()...")

                        local success, errorMsg = pcall(function()
                            attackAPComponent:ConsumeOneAP()
                        end)

                        if success then
                            print("[PlayerScript] SUCCESS! Crystal animation triggered via direct UIManager access")
                        else
                            print("[PlayerScript] ERROR calling ConsumeOneAP(): " .. tostring(errorMsg))
                        end
                    else
                        print("[PlayerScript] ConsumeOneAP method does not exist")
                    end
                else
                    print("[PlayerScript] attackAP component is nil")
                end
            else
                print("[PlayerScript] GetComponent method does not exist")
            end
        else
            print("[PlayerScript] UIManager is nil in entity Lua state - trying C++ bridge...")
            print("[PlayerScript] Calling TriggerAttackAPAnimation() via C++ bridge...")

            local success, errorMsg = pcall(function()
                TriggerAttackAPAnimation()
            end)

            if success then
                print("[PlayerScript] C++ bridge call completed successfully")
            else
                print("[PlayerScript] ERROR calling C++ bridge: " .. tostring(errorMsg))
            end
        end

        print("[PlayerScript] ========== END ANIMATION DEBUG ==========")

        -- Visual feedback
        PulseTile(enemyX, enemyY, 0.5, 1.0, 0.0, 0.0)  -- Red pulse for damage

        -- Play attack animation
        currentAnimGroup = AnimGroup.Attack
        SetAnimationGroup(entityID, currentAnimGroup)
        SetAnimationLoop(entityID, false)  -- Play once
        print("[PlayerScript] Playing attack animation")
    else
        print("[PlayerScript] Attack FAILED: DamageEntity returned false for enemy " .. enemyID)
        print("[PlayerScript] Possible causes:")
        print("[PlayerScript]   - Enemy has no Health component")
        print("[PlayerScript]   - Entity ID is invalid")
        print("[PlayerScript]   - Enemy was already destroyed")
    end

    print("============================================================")

    -- Clear attack preview
    ClearAttackPreview()
end

-- ============================================================================
-- LIFECYCLE: OnDestroy
-- ============================================================================

function OnDestroy()
    ClearAttackPreview()
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