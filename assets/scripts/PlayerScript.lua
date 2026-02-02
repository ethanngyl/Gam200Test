-- ============================================================================
-- PlayerScript.lua (FSM Version)
-- Grid-based player movement and animation component script
-- ============================================================================
-- This script handles player movement on a grid using arrow keys or WASD
-- Features:
-- - FSM-based state management (WaitingForInput, Moving, Attacking)
-- - Grid-based movement (one tile at a time)
-- - Input handling for arrow keys and WASD
-- - Action Point (AP) consumption
-- - Turn-based movement with cooldown
-- - Visual feedback (tile borders and pulses)
-- - Automatic animation state management (Idle/Walk/Attack/Injured/Death)
-- - Attack preview and execution
-- ============================================================================

-- ============================================================================
-- FSM CLASS (Built-in for reuse)
-- ============================================================================

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

-- ============================================================================
-- HELPER FUNCTIONS
-- ============================================================================

local function blockHeldKeys()
    blockedKeys = {}
    if IsKeyDown("W") then
        blockedKeys["W"] = true
        print("[PlayerScript]   W is held - blocking until released")
    end
    if IsKeyDown("S") then
        blockedKeys["S"] = true
        print("[PlayerScript]   S is held - blocking until released")
    end
    if IsKeyDown("A") then
        blockedKeys["A"] = true
        print("[PlayerScript]   A is held - blocking until released")
    end
    if IsKeyDown("D") then
        blockedKeys["D"] = true
        print("[PlayerScript]   D is held - blocking until released")
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
    lastPKeyDown = false
    lastSpaceKeyDown = false
    ClearAttackPreview()
end

-- ============================================================================
-- ATTACK HELPER FUNCTIONS (unchanged from original)
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
                    local indicatorID = SpawnSprite(
                        "assets/TileMap/Attack_Indicator.png",
                        worldX, worldY,
                        0.95, 0.95,
                        2
                    )

                    if indicatorID and indicatorID > 0 then
                        SetSpriteColor(indicatorID, 1.0, 0.0, 0.0, 0.5)
                        table.insert(attackPreviewTiles, indicatorID)
                        print("[PlayerScript]   Spawned attack indicator " .. indicatorID .. " at grid(" .. tile.x .. ", " .. tile.y .. ") world(" .. worldX .. ", " .. worldY .. ")")
                    else
                        print("[PlayerScript]   WARNING: Failed to spawn attack indicator at (" .. tile.x .. ", " .. tile.y .. ")")
                    end
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
        print("[PlayerScript] Clearing " .. #attackPreviewTiles .. " attack indicator entities")
        for _, indicatorID in ipairs(attackPreviewTiles) do
            if indicatorID and indicatorID > 0 then
                DestroyEntity(indicatorID)
                print("[PlayerScript]   Destroyed attack indicator " .. indicatorID)
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

    local currentAP, maxAP = GetEntityAP(entityID)
    print("[PlayerScript] Current AP: " .. currentAP .. "/" .. maxAP .. " (need " .. attackAPCost .. ")")

    if currentAP < attackAPCost then
        print("[PlayerScript] ATTACK BLOCKED: Not enough AP (" .. currentAP .. " < " .. attackAPCost .. ")")
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

        ConsumeEntityAP(entityID, attackAPCost)
        local newAP = GetEntityAP(entityID)
        print("[PlayerScript] AP consumed. New AP: " .. newAP .. "/" .. maxAP)

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
                print("[PlayerScript] P key pressed - manually ending turn for Entity " .. entityID)
                endTurn()
                return
            end
            lastPKeyDown = pKeyDown

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
                    local currentAP, maxAP = GetEntityAP(entityID)
                    if currentAP >= attackAPCost then
                        local testEnemy = FindEnemyInRange()
                        if testEnemy then
                            ShowAttackPreview()
                            print("[PlayerScript] Attack preview shown - enemy in range")
                        else
                            print("[PlayerScript] No enemy in attack range!")
                            PulseTile(currentX, currentY, 0.3, 1.0, 0.5, 0.0)
                        end
                    else
                        print("[PlayerScript] Not enough AP to attack (" .. currentAP .. " < " .. attackAPCost .. ")")
                        PulseTile(currentX, currentY, 0.3, 1.0, 1.0, 0.3)
                    end
                else
                    ExecuteAttack()
                end
            end
            lastSpaceKeyDown = spaceKeyDown

            -- Check movement input
            local wDown = IsKeyDown("W") and not blockedKeys["W"]
            local sDown = IsKeyDown("S") and not blockedKeys["S"]
            local aDown = IsKeyDown("A") and not blockedKeys["A"]
            local dDown = IsKeyDown("D") and not blockedKeys["D"]

            if wDown or sDown or aDown or dDown then
                -- Store movement direction
                fsm:setData("moveW", wDown)
                fsm:setData("moveS", sDown)
                fsm:setData("moveA", aDown)
                fsm:setData("moveD", dDown)
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

            print("[PlayerScript] Step 1: PASSED - target is valid and walkable")

            print("[PlayerScript] Step 2: Checking AP...")
            local currentAP, maxAP = GetEntityAP(entityID)
            print("[PlayerScript]   Entity " .. entityID .. " AP: " .. tostring(currentAP) .. "/" .. tostring(maxAP) .. " (need " .. apCostPerMove .. ")")

            if currentAP < apCostPerMove then
                print("[PlayerScript] FAILED: Not enough AP!")
                PulseTile(self.targetX, self.targetY, 0.3, 1.0, 1.0, 0.3)

                if currentAP == 0 then
                    print("[PlayerScript] AP depleted - ending turn!")
                    endTurn()
                end
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

                if newAP == 0 then
                    print("[PlayerScript] AP depleted after movement - ending turn!")
                    endTurn()
                    return
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

-- ============================================================================
-- LIFECYCLE: OnInit
-- ============================================================================

function OnInit(id)
    print("============================================================")
    print("========== PlayerScript OnInit() CALLED for Entity " .. id .. " ==========")
    print("============================================================")

    entityID = id

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
        print("[PlayerScript] Entity " .. entityID .. " AP: " .. tostring(currentAP) .. "/" .. tostring(maxAP))
        print("[PlayerScript] This entity will now respond to WASD input")
        print("============================================================")
        hasLoggedActive = true
    end

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