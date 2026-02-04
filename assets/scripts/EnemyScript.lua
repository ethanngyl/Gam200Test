-- ============================================================================
-- EnemyScript.lua
-- Enemy AI behavior component for grid-based turn-based combat
-- ============================================================================
-- Handles enemy turn logic, pathfinding, movement, and attacking
-- Attach to enemy entities via: AddScriptComponentToEntity(enemyID, "assets/scripts/EnemyScript.lua")
-- ============================================================================

-- ============================================================================
-- CONFIGURATION
-- ============================================================================

-- Enemy behavior types
local BEHAVIOR = {
    AGGRESSIVE = "aggressive",  -- Always chase and attack player
    DEFENSIVE = "defensive",    -- Only attack if player gets close
    PATROL = "patrol",          -- Move randomly unless player is near
    RANGED = "ranged"          -- Keep distance, attack from afar
}

-- AI state machine
local STATE = {
    IDLE = "idle",
    CHASING = "chasing",
    ATTACKING = "attacking",
    FLEEING = "fleeing",
    PATROLLING = "patrolling"
}

-- Entity state
local entityID = 0              -- This enemy's ID (set in OnInit)
local currentState = STATE.IDLE
local behaviorType = BEHAVIOR.AGGRESSIVE  -- Default behavior
local targetPlayerID = 0        -- Player to chase/attack

-- AI parameters (configurable per enemy type)
local config = {
    -- Movement
    apCostPerMove = 1,          -- AP cost to move 1 tile
    maxMovesPerTurn = 3,        -- Maximum tiles to move per turn

    -- Combat
    attackRange = 1,            -- How many tiles away enemy can attack
    attackAPCost = 2,           -- AP cost to attack
    attackDamage = 1,           -- Damage dealt per attack

    -- Behavior
    aggroRange = 8,             -- Tiles away to detect player (used by DEFENSIVE/PATROL behaviors; AGGRESSIVE ignores this)
    fleeHealthPercent = 0.3,    -- Flee when health drops below this
    preferredDistance = 1,      -- For ranged enemies

    -- Pathfinding
    maxPathLength = 10,         -- Maximum path length to consider
    recalculatePathEveryNTurns = 3  -- Recalculate path periodically
}

-- Internal state
local currentPath = {}          -- List of tiles to move through
local pathIndex = 1             -- Current position in path
local pathTargetPlayerID = 0    -- Which player the current path is targeting (for invalidation)
local turnsSincePathUpdate = 0  -- Track when to recalculate path
local lastKnownPlayerX = nil    -- Cache player position
local lastKnownPlayerY = nil
local hasActedThisTurn = false  -- Track if this enemy has acted this turn
local lastEnemyTurn = nil       -- Track which turn we last acted on
local isMyTurnToAct = false     -- Track if it's currently this enemy's turn to act

-- Movement timing (for visible, sequential moves)
local moveTimer = 0.0           -- Timer for next move
local moveDelay = _G.ENEMY_MOVE_DELAY or 0.6  -- Delay between moves, configurable via global
local movesThisTurn = 0         -- Track how many moves made this turn

-- DEBUG: Print delay value when enemy script loads
print("[EnemyScript] Script loaded with moveDelay = " .. moveDelay)

-- Global flag to track if any enemy has panned camera this turn
if not _G.EnemyCameraPannedThisTurn then
    _G.EnemyCameraPannedThisTurn = false
end

-- ============================================================================
-- LIFECYCLE CALLBACKS
-- ============================================================================

function OnInit()
    -- Get this entity's ID from the script system (automatically set as 'self')
    entityID = self

    if not entityID or entityID == 0 then
        Log("[EnemyScript] ERROR: Invalid entity ID")
        return
    end

    Log("[EnemyScript] Enemy " .. entityID .. " initialized")

    -- Initialize state
    currentState = STATE.IDLE
    currentPath = {}
    pathIndex = 1

    -- CRITICAL: Set initial tile occupancy so players can't walk through this enemy
    local enemyX, enemyY = GetEntityGridPosition(entityID)
    if enemyX and enemyY then
        -- Use SetTileOccupant if available, otherwise the C++ MoveEntityToTile handles it
        if SetTileOccupant then
            SetTileOccupant(enemyX, enemyY, entityID)
            Log("[EnemyScript] Enemy " .. entityID .. " set occupancy at (" .. enemyX .. ", " .. enemyY .. ")")
        end
    end

    -- CRITICAL FIX: Use FindClosestPlayer() instead of FindPlayer()
    -- FindPlayer() returns the FIRST player (usually 547), not the closest
    -- We'll find the actual closest player on first update
    -- For now, just use FindPlayer() as a fallback
    targetPlayerID = FindPlayer()
    if targetPlayerID and targetPlayerID > 0 then
        Log("[EnemyScript] Enemy " .. entityID .. " initialized with fallback player " .. targetPlayerID)
    else
        Log("[EnemyScript] WARNING: Enemy " .. entityID .. " could not find player")
    end
end

function OnUpdate(dt)
    -- Enemy AI only runs during enemy turn
    local currentTurn = GetCurrentTurn()

    if currentTurn ~= "Enemy" then
        -- Reset acted flag when it's not enemy turn
        if lastEnemyTurn == "Enemy" then
            hasActedThisTurn = false
            isMyTurnToAct = false
            moveTimer = 0.0
            movesThisTurn = 0
        end
        lastEnemyTurn = currentTurn
        return
    end

    -- Track that this is enemy turn
    lastEnemyTurn = "Enemy"

    -- SEQUENTIAL TURN SYSTEM: Only act if this enemy is the active one
    if not IsActiveEnemy then
        print("[Enemy " .. entityID .. "] ERROR: IsActiveEnemy is nil!")
        return
    end

    local success, isActive = pcall(IsActiveEnemy, entityID)
    if not success then
        print("[Enemy " .. entityID .. "] ERROR: IsActiveEnemy() threw error: " .. tostring(isActive))
        return
    end

    if not isActive then
        -- Not my turn yet, wait
        return
    end

    -- Check if action timer is ready (for visual delay between enemies)
    if not IsEnemyActionReady then
        print("[Enemy " .. entityID .. "] ERROR: IsEnemyActionReady is nil!")
        return
    end

    local success, actionReady = pcall(IsEnemyActionReady)
    if not success then
        print("[Enemy " .. entityID .. "] ERROR: IsEnemyActionReady() threw error: " .. tostring(actionReady))
        return
    end

    if not actionReady then
        -- Still in delay, wait
        return
    end

    -- Check if this enemy has already acted this turn
    if hasActedThisTurn then
        -- Already acted, don't process again
        return
    end

    -- Update move timer
    if moveTimer > 0 then
        local oldTimer = moveTimer
        moveTimer = moveTimer - dt
        if moveTimer < 0 then
            moveTimer = 0
        end
        print(string.format("[Enemy %d] WAITING for moveTimer: %.2f -> %.2f (moveDelay=%.2f)", entityID, oldTimer, moveTimer, moveDelay))
        -- Still waiting for move delay
        return
    end

    -- First time acting - set up turn
    if not isMyTurnToAct then
        isMyTurnToAct = true
        movesThisTurn = 0
        print("[Enemy " .. entityID .. "] ========== STARTING TURN ==========")

        -- CRITICAL: Find closest player dynamically each turn
        local closestPlayer, closestDistance = FindClosestPlayer()

        if closestPlayer and closestPlayer > 0 then
            targetPlayerID = closestPlayer
        else
            print("[Enemy " .. entityID .. "] No player found!")
            hasActedThisTurn = true  -- Mark as acted even if no target
            MarkEnemyActionComplete()  -- Advance to next enemy
            return  -- No player to target
        end
    end

    -- Execute AI decision making (will make ONE move per frame)
    ProcessAITurn()
end

function OnDestroy()
    -- CRITICAL: Clear tile occupancy when enemy is destroyed
    -- This ensures players can walk through tiles where enemies died
    local enemyX, enemyY = GetEntityGridPosition(entityID)
    if enemyX and enemyY then
        if SetTileOccupant then
            SetTileOccupant(enemyX, enemyY, 0)  -- 0 clears the occupant
            Log("[EnemyScript] Enemy " .. entityID .. " cleared occupancy at (" .. enemyX .. ", " .. enemyY .. ")")
        end
    end
    Log("[EnemyScript] Enemy " .. entityID .. " destroyed")
end

-- ============================================================================
-- AI DECISION MAKING
-- ============================================================================

function ProcessAITurn()
    -- Get enemy AP
    local currentAP, maxAP = GetEntityAP(entityID)

    if not currentAP or currentAP == 0 or currentAP < config.apCostPerMove then
        print("[Enemy " .. entityID .. "] Not enough AP, finishing turn")
        FinishEnemyAction()
        return
    end

    -- Update state based on situation
    UpdateAIState()

    -- Execute action based on current state
    if currentState == STATE.ATTACKING then
        ExecuteAttack()
    elseif currentState == STATE.CHASING then
        ExecuteChase()
    elseif currentState == STATE.FLEEING then
        ExecuteFlee()
    elseif currentState == STATE.PATROLLING then
        ExecutePatrol()
    else
        -- IDLE or unknown state
        FinishEnemyAction()
    end
end

function UpdateAIState()
    -- Recalculate closest player BEFORE making state decisions
    local closestPlayer, closestDistance = FindClosestPlayer()

    if closestPlayer and closestPlayer > 0 then
        -- Clear path if target changed
        if closestPlayer ~= targetPlayerID and targetPlayerID ~= 0 then
            print("[Enemy " .. entityID .. "] Target changed P" .. targetPlayerID .. " -> P" .. closestPlayer .. ", clearing path")
            currentPath = {}
            pathIndex = 1
            pathTargetPlayerID = 0

            -- CRITICAL FIX: Update C++ EnemyAI target so pathfinding uses correct player
            -- The C++ Pathfinding system (Pathfinding.cpp) uses ai.targetEntity for pathfinding
            -- We must sync the Lua targetPlayerID with the C++ ai.targetEntity
            print("[Enemy " .. entityID .. "] Updating C++ target via SetEnemyTarget(" .. entityID .. ", " .. closestPlayer .. ")")
            SetEnemyTarget(entityID, closestPlayer)
        end
        targetPlayerID = closestPlayer
    else
        currentState = STATE.IDLE
        return
    end

    -- Get enemy and player positions
    local enemyX, enemyY = GetEntityGridPosition(entityID)
    local playerX, playerY = GetEntityGridPosition(targetPlayerID)

    if not enemyX or not playerX then
        currentState = STATE.IDLE
        return
    end

    -- Calculate distance to player
    local distance = CalculateDistance(enemyX, enemyY, playerX, playerY)

    -- Check health for flee condition
    local hp, maxHP = GetEntityHP(entityID)
    local healthPercent = hp / maxHP

    if healthPercent <= config.fleeHealthPercent then
        currentState = STATE.FLEEING
        return
    end

    -- State transition logic based on behavior type and distance
    if behaviorType == BEHAVIOR.AGGRESSIVE then
        -- AGGRESSIVE enemies ALWAYS chase players regardless of distance
        if distance <= config.attackRange then
            currentState = STATE.ATTACKING
        else
            currentState = STATE.CHASING
        end

    elseif behaviorType == BEHAVIOR.DEFENSIVE then
        if distance <= config.attackRange then
            currentState = STATE.ATTACKING
        elseif distance <= 3 then  -- Smaller aggro range
            currentState = STATE.CHASING
        else
            currentState = STATE.IDLE
        end

    elseif behaviorType == BEHAVIOR.RANGED then
        if distance <= config.attackRange and distance >= config.preferredDistance then
            currentState = STATE.ATTACKING
        elseif distance < config.preferredDistance then
            currentState = STATE.FLEEING  -- Too close, back away
        elseif distance > config.attackRange then
            currentState = STATE.CHASING  -- Too far, move closer
        end

    elseif behaviorType == BEHAVIOR.PATROL then
        if distance <= config.attackRange then
            currentState = STATE.ATTACKING
        elseif distance <= 4 then
            currentState = STATE.CHASING
        else
            currentState = STATE.PATROLLING
        end
    end
end

-- ============================================================================
-- ACTION EXECUTION
-- ============================================================================

function ExecuteAttack()
    local currentAP, maxAP = GetEntityAP(entityID)

    -- Check if we have enough AP to attack
    if currentAP < config.attackAPCost then
        ExecuteChase()
        return
    end

    -- Recalculate closest player BEFORE attacking
    local closestPlayer, closestDistance = FindClosestPlayer()

    if closestPlayer and closestPlayer > 0 then
        -- Clear path if target changed
        if closestPlayer ~= targetPlayerID and targetPlayerID ~= 0 then
            currentPath = {}
            pathIndex = 1
            pathTargetPlayerID = 0

            -- CRITICAL FIX: Update C++ EnemyAI target
            SetEnemyTarget(entityID, closestPlayer)
        end
        targetPlayerID = closestPlayer
    else
        FinishEnemyAction()
        return
    end

    -- Verify player is still in range
    local enemyX, enemyY = GetEntityGridPosition(entityID)
    local playerX, playerY = GetEntityGridPosition(targetPlayerID)
    local distance = CalculateDistance(enemyX, enemyY, playerX, playerY)

    if distance > config.attackRange then
        -- Player moved out of range, chase instead
        currentState = STATE.CHASING
        ExecuteChase()
        return
    end

    -- Execute attack
    print("[Enemy " .. entityID .. "] ATTACKING Player " .. targetPlayerID .. " for " .. config.attackDamage .. " damage")
    
    -- Debug: Check player HP before attack
    local hpBefore, maxHP = GetEntityHP(targetPlayerID)
    print("[Enemy " .. entityID .. "] Player " .. targetPlayerID .. " HP BEFORE: " .. tostring(hpBefore) .. "/" .. tostring(maxHP))

    local success = DamageEntity(targetPlayerID, config.attackDamage)
    
    print("[Enemy " .. entityID .. "] DamageEntity returned: " .. tostring(success))

    if success then
        -- Debug: Check player HP after attack
        local hpAfter, _ = GetEntityHP(targetPlayerID)
        print("[Enemy " .. entityID .. "] Player " .. targetPlayerID .. " HP AFTER: " .. tostring(hpAfter))
        
        -- Consume attack AP
        ConsumeEnemyAP(entityID, config.attackAPCost)

        -- Visual feedback
        PulseTile(playerX, playerY, 0.3, 1.0, 0.0, 0.0)  -- Red pulse for damage

        -- Show damage number popup (one-time animation)
        if PopupManager and PopupManager.ShowDamageNumber then
            local worldX, worldY = GetEntityWorldPosition(targetPlayerID)
            if worldX then
                PopupManager.ShowDamageNumber(worldX, worldY + 0.2, config.attackDamage)
            end
        end
    else
        print("[Enemy " .. entityID .. "] ATTACK FAILED! Player " .. targetPlayerID .. " may not have Health component")
    end

    -- Check if we can still act
    currentAP = currentAP - config.attackAPCost
    if currentAP >= config.apCostPerMove then
        -- We can still move, try to chase
        ExecuteChase()
    else
        -- End turn
        FinishEnemyAction()
    end
end

function ExecuteChase()
    local currentAP, maxAP = GetEntityAP(entityID)

    -- Get positions
    local enemyX, enemyY = GetEntityGridPosition(entityID)
    local playerX, playerY = GetEntityGridPosition(targetPlayerID)

    if not enemyX or not playerX then
        FinishEnemyAction()
        return
    end

    -- Check if we need to recalculate path
    local needsNewPath = false
    if #currentPath == 0 or pathIndex > #currentPath then
        needsNewPath = true
    elseif pathTargetPlayerID ~= targetPlayerID then
        -- Target changed, need new path
        needsNewPath = true
    elseif lastKnownPlayerX ~= playerX or lastKnownPlayerY ~= playerY then
        -- Player moved - recalculate path immediately instead of waiting
        needsNewPath = true
        turnsSincePathUpdate = 0
    end

    -- Calculate path to player
    if needsNewPath then
        print("[Enemy " .. entityID .. "] Calculating path to P" .. targetPlayerID .. " at (" .. playerX .. ", " .. playerY .. ")")
        -- CRITICAL FIX: Pass grid coordinates, not entity ID!
        currentPath = FindPathToTarget(enemyX, enemyY, playerX, playerY)
        pathIndex = 1
        pathTargetPlayerID = targetPlayerID
        lastKnownPlayerX = playerX
        lastKnownPlayerY = playerY

        if not currentPath or #currentPath == 0 then
            print("[Enemy " .. entityID .. "] NO PATH to P" .. targetPlayerID .. " at (" .. playerX .. ", " .. playerY .. ")")
            FinishEnemyAction()
            return
        end
    end

    -- Calculate maximum moves we can make this turn
    local maxMoves = config.maxMovesPerTurn

    -- CRITICAL: Reserve AP for attacking if we're getting close to the player
    -- Check if we'll be in attack range after moving
    local distanceToPlayer = CalculateDistance(enemyX, enemyY, playerX, playerY)
    if distanceToPlayer <= config.maxMovesPerTurn + config.attackRange then
        -- We might reach attack range this turn, reserve AP for attacking
        local apNeededForAttack = config.attackAPCost
        local apAvailableForMovement = currentAP - apNeededForAttack

        -- Only reserve if we have enough AP, otherwise just use all available AP for movement
        if apAvailableForMovement >= config.apCostPerMove then
            maxMoves = math.min(maxMoves, math.floor(apAvailableForMovement / config.apCostPerMove))
        end
    end

    -- Move ONE tile per frame (not all at once!)
    if currentAP >= config.apCostPerMove and pathIndex <= #currentPath and movesThisTurn < maxMoves then
        local nextTile = currentPath[pathIndex]

        -- Validate tile is still walkable
        if IsWalkableTile(nextTile.x, nextTile.y) then
            -- NOTE: MoveEntityToTile in C++ already handles:
            -- 1. Clearing occupancy at old position
            -- 2. Setting occupancy at new position
            -- No need to manually call SetTileOccupant here
            local success = MoveEntityToTile(entityID, nextTile.x, nextTile.y)

            if success then
                ConsumeEnemyAP(entityID, config.apCostPerMove)
                currentAP = currentAP - config.apCostPerMove
                movesThisTurn = movesThisTurn + 1
                pathIndex = pathIndex + 1

                -- Visual feedback
                ShowTileBorder(nextTile.x, nextTile.y, 0.3)
                PulseTile(nextTile.x, nextTile.y, 0.2, 1.0, 0.5, 0.0)  -- Orange pulse

                -- Manage tile occupancy
                --local enemyX, enemyY = GetEntityGridPosition(entityID)
                --if enemyX and enemyY then
                    -- Only set occupancy if the enemy actually moved
                  --  if not SetTileOccupant or SetTileOccupant(enemyX, enemyY, entityID) then
                    --    Log("[EnemyScript] Enemy " .. entityID .. " set occupancy at (" .. enemyX .. ", " .. enemyY .. ")")
                    --end
                --end

                -- Set timer for next move (creates visible delay)
                moveTimer = moveDelay

                -- Return to let next frame handle the next move
                return
            else
                -- Movement failed, recalculate path next turn
                currentPath = {}
                FinishEnemyAction()
                return
            end
        else
            -- Tile became unwalkable, recalculate path
            currentPath = {}
            FinishEnemyAction()
            return
        end
    end

    -- If we get here, we can't move anymore (out of AP, path, or maxMoves)
    -- Check if we're now in attack range
    local newEnemyX, newEnemyY = GetEntityGridPosition(entityID)
    local distance = CalculateDistance(newEnemyX, newEnemyY, playerX, playerY)

    if distance <= config.attackRange and currentAP >= config.attackAPCost then
        -- We reached attack range and have AP, attack!
        currentState = STATE.ATTACKING
        ExecuteAttack()
    else
        -- End turn
        FinishEnemyAction()
    end
end

function ExecuteFlee()
    -- Move away from player
    local currentAP, maxAP = GetEntityAP(entityID)
    local enemyX, enemyY = GetEntityGridPosition(entityID)
    local playerX, playerY = GetEntityGridPosition(targetPlayerID)

    if not enemyX or not playerX then
        FinishEnemyAction()
        return
    end

    -- Calculate direction away from player
    local deltaX = enemyX - playerX
    local deltaY = enemyY - playerY

    -- Try to move in opposite direction
    local fleeX = enemyX
    local fleeY = enemyY

    if math.abs(deltaX) > math.abs(deltaY) then
        -- Move horizontally away
        fleeX = enemyX + (deltaX > 0 and 1 or -1)
    else
        -- Move vertically away
        fleeY = enemyY + (deltaY > 0 and 1 or -1)
    end

    -- Attempt flee movement
    if IsWalkableTile(fleeX, fleeY) and currentAP >= config.apCostPerMove then
        local success = MoveEntityToTile(entityID, fleeX, fleeY)
        if success then
            ConsumeEnemyAP(entityID, config.apCostPerMove)
            PulseTile(fleeX, fleeY, 0.2, 1.0, 1.0, 0.0)  -- Yellow pulse (fleeing)
        end
    end

    FinishEnemyAction()
end

function ExecutePatrol()
    -- Simple random movement
    local currentAP, maxAP = GetEntityAP(entityID)
    local enemyX, enemyY = GetEntityGridPosition(entityID)

    if currentAP < config.apCostPerMove then
        FinishEnemyAction()
        return
    end

    -- Pick random adjacent tile
    local directions = {
        {x = 1, y = 0},
        {x = -1, y = 0},
        {x = 0, y = 1},
        {x = 0, y = -1}
    }

    local dir = directions[math.random(1, #directions)]
    local newX = enemyX + dir.x
    local newY = enemyY + dir.y

    if IsWalkableTile(newX, newY) then
        local success = MoveEntityToTile(entityID, newX, newY)
        if success then
            ConsumeEnemyAP(entityID, config.apCostPerMove)
            PulseTile(newX, newY, 0.2, 0.5, 0.5, 1.0)  -- Blue pulse (patrol)
        end
    end

    FinishEnemyAction()
end

-- ============================================================================
-- HELPER FUNCTIONS
-- ============================================================================

function CalculateDistance(x1, y1, x2, y2)
    -- Manhattan distance (grid-based)
    return math.abs(x2 - x1) + math.abs(y2 - y1)
end

-- Find the closest player from all party members
function FindClosestPlayer()
    -- Get enemy position
    local enemyX, enemyY = GetEntityGridPosition(entityID)
    if not enemyX then
        print("[Enemy " .. entityID .. "] ERROR: GetEntityGridPosition returned nil!")
        return nil
    end

    -- Use GetAllPlayers() (C++ function available in all Lua states)
    local players = GetAllPlayers()

    -- Validate player list
    if not players or type(players) ~= "table" or #players == 0 then
        print("[Enemy " .. entityID .. "] WARNING: GetAllPlayers() returned no players, using fallback")
        return FindPlayer()
    end

    -- Find closest player and build detailed distance report
    local closestPlayerID = nil
    local closestDistance = 999999
    local distanceReport = {}

    for i, playerID in ipairs(players) do
        -- Skip dead players
        local currentHP, maxHP = GetEntityHP(playerID)
        if not currentHP or currentHP <= 0 then
            table.insert(distanceReport, "P" .. playerID .. "=DEAD")
            -- Skip dead players - don't target them
        else
            local playerX, playerY = GetEntityGridPosition(playerID)
            if playerX then
                local distance = CalculateDistance(enemyX, enemyY, playerX, playerY)
                table.insert(distanceReport, "P" .. playerID .. "=" .. distance)
                if distance < closestDistance then
                    closestDistance = distance
                    closestPlayerID = playerID
                end
            end
        end
    end

    if closestPlayerID then
        print("[Enemy " .. entityID .. "] Target: P" .. closestPlayerID .. " [" .. table.concat(distanceReport, ", ") .. "]")
    else
        print("[Enemy " .. entityID .. "] WARNING: No valid player found from " .. #players .. " candidates")
    end

    return closestPlayerID, closestDistance
end

-- GetEntityHP(entityID) C++ function already exists in LevelLoader API
-- No wrapper needed - just call it directly from anywhere in this script
-- Removed old Lua wrapper that was using the broken GetPlayerHP()

-- ============================================================================
-- CONFIGURATION API (Called from Level Scripts)
-- ============================================================================

function SetBehavior(behavior)
    behaviorType = behavior
    Log("[EnemyScript] Enemy " .. entityID .. " behavior set to: " .. behavior)
end

function SetAggression(range)
    config.aggroRange = range
end

function SetAttackRange(range)
    config.attackRange = range
end

function SetAttackDamage(damage)
    config.attackDamage = damage
end

function SetMovementSpeed(tilesPerTurn)
    config.maxMovesPerTurn = tilesPerTurn
end

-- ============================================================================
-- ENEMY TURN COORDINATION
-- ============================================================================

-- Mark this enemy as having completed its turn and advance to next enemy
function FinishEnemyAction()
    hasActedThisTurn = true
    movesThisTurn = 0  -- Reset for next turn
    moveTimer = 0.0    -- Reset timer
    Log("[Enemy " .. entityID .. "] Finished turn")
    MarkEnemyActionComplete()  -- Advance to next enemy in sequence
end

-- REMOVED: CheckAllEnemiesActed() - replaced by EnemyTurnManager
-- Sequential turn system now handles enemy turn advancement and camera panning

-- ============================================================================
-- Export behavior types for level scripts
-- ============================================================================
ENEMY_BEHAVIOR = BEHAVIOR
