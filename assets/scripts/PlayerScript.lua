-- ============================================================================
-- PlayerScript.lua
-- Grid-based player movement and animation component script
-- ============================================================================
-- This script handles player movement on a grid using arrow keys or WASD
-- Features:
-- - Grid-based movement (one tile at a time)
-- - Input handling for arrow keys and WASD
-- - Action Point (AP) consumption
-- - Turn-based movement with cooldown
-- - Visual feedback (tile borders and pulses)
-- - Automatic animation state management (Idle/Walk/Attack/Injured/Death)
-- ============================================================================

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

local entityID = 0  -- Will be set by the engine when attached
local moveCooldown = 0.0
local moveCooldownTime = 0.2  -- Seconds between moves (prevents rapid input)
local apCostPerMove = 1  -- AP cost for each movement

-- Animation state
local currentAnimGroup = AnimGroup.Idle
local currentAnimDirection = AnimDirection.Front
local isFlippedX = false

-- Debug tracking
local hasLoggedActive = false  -- Reset when turn changes
local lastTurnPrint = nil      -- Track turn phase for debug printing

-- Input state tracking (prevents carry-over from previous character's turn)
local lastActiveCheck = false   -- Track if we were active last frame
local blockedKeys = {}          -- Keys that were held when we became active (must be released first)
-- blockedKeys["W"] = true means W was held when turn started, ignore until released

-- P key state tracking (for single-press detection)
local lastPKeyDown = false      -- Track if P was down last frame

-- Attack state tracking
local lastSpaceKeyDown = false  -- Track if SPACE was down last frame
local attackPreviewActive = false  -- Track if attack preview is showing
local attackPreviewTiles = {}   -- List of preview tile entity IDs
local attackRange = 1           -- Attack range in tiles (Manhattan distance)
local attackAPCost = 1          -- AP cost to attack

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

    print("[PlayerScript] Entity " .. entityID .. " initialized successfully")
    print("============================================================")
end

-- ============================================================================
-- LIFECYCLE: OnUpdate
-- ============================================================================

function OnUpdate(dt)
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
    -- Prevents all 3 party members from responding to input simultaneously
    local isActive = IsActiveCharacter(entityID)
    if not isActive then
        -- Not this character's turn - reset state
        if lastActiveCheck then
            -- Just became inactive - reset animation to Idle
            lastActiveCheck = false
            hasLoggedActive = false
            blockedKeys = {}  -- Clear blocked keys
            lastPKeyDown = false  -- Reset P key state
            lastSpaceKeyDown = false  -- Reset SPACE key state
            ClearAttackPreview()  -- Clear any active attack preview

            -- Set animation to Idle when no longer active
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

    -- ========================================================================
    -- INPUT STATE TRACKING (prevents key carry-over from previous turn)
    -- ========================================================================

    -- Detect if we just became active this frame
    if isActive and not lastActiveCheck then
        -- Just became active - check which keys are currently held and block them
        print("[PlayerScript] Entity " .. entityID .. " just became active - checking held keys...")

        blockedKeys = {}  -- Reset blocked keys

        -- Check each movement key and block it if currently held
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

        -- Check P key for turn ending
        if IsKeyDown("P") then
            lastPKeyDown = true
            print("[PlayerScript]   P is held - will ignore until released")
        else
            lastPKeyDown = false
        end

        -- Check SPACE key for attacking
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
    lastActiveCheck = isActive

    -- Update blocked keys - unblock keys that have been released
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

    print("[PlayerScript] DEBUG: Passed all blocking checks, checking for input...")

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
        lastPKeyDown = false  -- Reset P key state

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
        print("[PlayerScript] SPACE key pressed")

        if not attackPreviewActive then
            -- First press: Show attack preview
            local currentAP, maxAP = GetEntityAP(entityID)
            if currentAP >= attackAPCost then
                -- Check if there's an enemy in range before showing preview
                local testEnemy = FindEnemyInRange()
                if testEnemy then
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
            ExecuteAttack()
        end
    end
    lastSpaceKeyDown = spaceKeyDown  -- Update SPACE key state for next frame

    -- ========================================================================
    -- UPDATE ATTACK PREVIEW (Keep it visible)
    -- ========================================================================

    -- Refresh attack preview visuals each frame to keep them visible
    if attackPreviewActive and #attackPreviewTiles > 0 then
        for _, tile in ipairs(attackPreviewTiles) do
            -- Continuously show borders and pulses
            ShowTileBorder(tile.x, tile.y, 0.1)  -- Short refresh
            -- Note: PulseTile creates one-time animations, so we don't call it every frame
        end
    end

    -- ========================================================================
    -- MOVEMENT INPUT
    -- ========================================================================

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
        -- Consume AP (use entity-based API for party system)
        ConsumeEntityAP(entityID, apCostPerMove)

        -- Check if AP depleted after movement
        local newAP, maxAP = GetEntityAP(entityID)
        print("[PlayerScript] After movement: Entity " .. entityID .. " AP: " .. tostring(newAP) .. "/" .. tostring(maxAP))

        if newAP == 0 then
            print("[PlayerScript] AP depleted after movement - ending turn!")

            -- Set animation back to Idle before ending turn
            currentAnimGroup = AnimGroup.Idle
            SetAnimationGroup(entityID, currentAnimGroup)

            EndCharacterTurn()
            hasLoggedActive = false  -- Reset for next character
            lastActiveCheck = false  -- Reset active tracking
            blockedKeys = {}  -- Clear blocked keys

            -- IMPORTANT: Return early - don't continue updating animations
            -- The character is no longer active, so we shouldn't modify its state
            return
        end

        -- Visual feedback
        ShowTileBorder(targetX, targetY, 0.5)  -- Show border for 0.5 seconds
        PulseTile(targetX, targetY, 0.3, 0.3, 1.0, 0.3)  -- Green pulse

        -- ====================================================================
        -- UPDATE ANIMATION STATE BASED ON MOVEMENT
        -- ====================================================================

        -- Determine animation direction and flip state
        local newDirection = currentAnimDirection
        local newFlipX = isFlippedX

        if moveDirY > 0 then
            -- Moving up
            newDirection = AnimDirection.Back
        elseif moveDirY < 0 then
            -- Moving down
            newDirection = AnimDirection.Front
        elseif moveDirX > 0 then
            -- Moving right
            newDirection = AnimDirection.Side
            newFlipX = false
        elseif moveDirX < 0 then
            -- Moving left
            newDirection = AnimDirection.Side
            newFlipX = true
        end

        -- Update direction if changed
        if newDirection ~= currentAnimDirection then
            currentAnimDirection = newDirection
            SetAnimationDirection(entityID, currentAnimDirection)
        end

        -- Update flip state if changed
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

        -- Check for chest collection
        if HasChestAtTile(targetX, targetY) then
            CollectChest(targetX, targetY)
            Log("[PlayerScript] Collected chest at (" .. targetX .. ", " .. targetY .. ")")
        end

        -- Check for goal completion
        if HasGoalAtTile(targetX, targetY) then
            Log("[PlayerScript] Reached goal! Level complete!")
            -- Goal completion is handled by C++ TurnSystem
        end

        -- Set movement cooldown
        moveCooldown = moveCooldownTime

        Log("[PlayerScript] Moved to (" .. targetX .. ", " .. targetY .. ") - AP remaining: " .. (currentAP - apCostPerMove))
    else
        Log("[PlayerScript] Failed to move to (" .. targetX .. ", " .. targetY .. ")")
    end
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

    -- Show attack preview tiles in a range pattern (Manhattan distance)
    for r = 1, attackRange do
        local candidates = {
            {x = currentX + r, y = currentY},      -- Right
            {x = currentX - r, y = currentY},      -- Left
            {x = currentX, y = currentY + r},      -- Up
            {x = currentX, y = currentY - r}       -- Down
        }

        for _, tile in ipairs(candidates) do
            if IsValidGridPosition(tile.x, tile.y) then
                -- Show red pulse for attack preview (longer duration)
                PulseTile(tile.x, tile.y, 0.8, 1.0, 0.0, 0.0)  -- Red pulse, 0.8s duration
                ShowTileBorder(tile.x, tile.y, 2.0)  -- Show border for 2 seconds

                -- Track preview tiles
                table.insert(attackPreviewTiles, {x = tile.x, y = tile.y})

                print("[PlayerScript]   Attack preview at (" .. tile.x .. ", " .. tile.y .. ")")
            end
        end
    end

    if #attackPreviewTiles > 0 then
        attackPreviewActive = true
        print("[PlayerScript] Attack preview active with " .. #attackPreviewTiles .. " tiles")
    else
        print("[PlayerScript] WARNING: No valid attack preview tiles found")
    end
end

function ClearAttackPreview()
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
    local currentAP, maxAP = GetEntityAP(entityID)
    print("[PlayerScript] Current AP: " .. currentAP .. "/" .. maxAP .. " (need " .. attackAPCost .. ")")

    if currentAP < attackAPCost then
        print("[PlayerScript] ATTACK BLOCKED: Not enough AP (" .. currentAP .. " < " .. attackAPCost .. ")")
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

    print("[PlayerScript] Target found: Enemy " .. enemyID .. " at (" .. enemyX .. ", " .. enemyY .. ")")
    print("[PlayerScript] Attacking enemy " .. enemyID .. " for " .. 1 .. " damage...")

    -- Deal damage
    local attackDamage = 1  -- Base damage
    local success = DamageEntity(enemyID, attackDamage)

    if success then
        print("[PlayerScript] ✓ Attack SUCCESS! Enemy " .. enemyID .. " damaged for " .. attackDamage .. " HP")

        -- Consume attack AP
        ConsumeEntityAP(entityID, attackAPCost)
        local newAP = GetEntityAP(entityID)
        print("[PlayerScript] AP consumed. New AP: " .. newAP .. "/" .. maxAP)

        -- Visual feedback
        PulseTile(enemyX, enemyY, 0.5, 1.0, 0.0, 0.0)  -- Red pulse for damage

        -- Play attack animation
        currentAnimGroup = AnimGroup.Attack
        SetAnimationGroup(entityID, currentAnimGroup)
        SetAnimationLoop(entityID, false)  -- Play once
        print("[PlayerScript] Playing attack animation")
    else
        print("[PlayerScript] ✗ Attack FAILED: DamageEntity returned false for enemy " .. enemyID)
    end

    print("============================================================")

    -- Clear attack preview
    ClearAttackPreview()
end

-- ============================================================================
-- LIFECYCLE: OnDestroy
-- ============================================================================

function OnDestroy()
    Log("[PlayerScript] Destroyed for entity " .. entityID)
end
