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
local turnsSincePathUpdate = 0  -- Track when to recalculate path
local lastKnownPlayerX = nil    -- Cache player position
local lastKnownPlayerY = nil
local hasActedThisTurn = false  -- Track if this enemy has acted this turn
local lastEnemyTurn = nil       -- Track which turn we last acted on
local isMyTurnToAct = false     -- Track if it's currently this enemy's turn to act

-- Movement timing (for visible, sequential moves)
local moveTimer = 0.0           -- Timer for next move
local moveDelay = 0.3           -- Delay between moves in seconds (0.3s = visible movement)
local movesThisTurn = 0         -- Track how many moves made this turn

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
    -- DEBUG: Verify OnUpdate is being called
    print("[Enemy " .. entityID .. "] OnUpdate called (dt=" .. string.format("%.3f", dt) .. ")")

    -- Enemy AI only runs during enemy turn
    local currentTurn = GetCurrentTurn()
    print("[Enemy " .. entityID .. "]   Current turn: " .. tostring(currentTurn))

    if currentTurn ~= "Enemy" then
        -- Reset acted flag when it's not enemy turn
        if lastEnemyTurn == "Enemy" then
            hasActedThisTurn = false
            isMyTurnToAct = false
            moveTimer = 0.0
            movesThisTurn = 0
            print("[Enemy " .. entityID .. "]   Resetting turn flags (was enemy turn, now " .. tostring(currentTurn) .. ")")
        end
        lastEnemyTurn = currentTurn
        return
    end

    -- Track that this is enemy turn
    lastEnemyTurn = "Enemy"
    print("[Enemy " .. entityID .. "]   It IS enemy turn!")

    -- SEQUENTIAL TURN SYSTEM: Only act if this enemy is the active one
    local isActive = IsActiveEnemy(entityID)
    print("[Enemy " .. entityID .. "]   IsActiveEnemy(" .. entityID .. "): " .. tostring(isActive))
    if not isActive then
        -- Not my turn yet, wait
        return
    end

    -- Check if action timer is ready (for visual delay between enemies)
    local actionReady = IsEnemyActionReady()
    print("[Enemy " .. entityID .. "]   IsEnemyActionReady(): " .. tostring(actionReady))
    if not actionReady then
        -- Still in delay, wait
        return
    end

    -- Check if this enemy has already acted this turn
    print("[Enemy " .. entityID .. "]   hasActedThisTurn: " .. tostring(hasActedThisTurn))
    if hasActedThisTurn then
        -- Already acted, don't process again
        return
    end

    -- Update move timer
    print("[Enemy " .. entityID .. "]   moveTimer: " .. moveTimer)
    if moveTimer > 0 then
        moveTimer = moveTimer - dt
        if moveTimer < 0 then
            moveTimer = 0
        end
        -- Still waiting for move delay
        print("[Enemy " .. entityID .. "]   Waiting for move delay (timer: " .. moveTimer .. ")")
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
            print("[Enemy " .. entityID .. "] Targeting Player " .. closestPlayer .. " (distance: " .. closestDistance .. ")")
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
    Log("[EnemyScript] Enemy " .. entityID .. " destroyed")
end

-- ============================================================================
-- AI DECISION MAKING
-- ============================================================================

function ProcessAITurn()
    -- Get enemy AP
    local currentAP, maxAP = GetEntityAP(entityID)

    print("[Enemy " .. entityID .. "] ProcessAITurn - AP: " .. tostring(currentAP) .. "/" .. tostring(maxAP))

    if not currentAP or currentAP == 0 or currentAP < config.apCostPerMove then
        print("[Enemy " .. entityID .. "] Not enough AP to act (need " .. config.apCostPerMove .. "), finishing turn")
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
        print("[Enemy " .. entityID .. "] State is IDLE, ending turn")
        FinishEnemyAction()
    end
end

function UpdateAIState()
    -- Recalculate closest player BEFORE making state decisions
    local closestPlayer, closestDistance = FindClosestPlayer()

    if closestPlayer and closestPlayer > 0 then
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
            print("[Enemy " .. entityID .. "] Within attack range (" .. distance .. " <= " .. config.attackRange .. ") - ATTACKING Player " .. targetPlayerID)
            currentState = STATE.ATTACKING
        else
            print("[Enemy " .. entityID .. "] Outside attack range (" .. distance .. " > " .. config.attackRange .. ") - CHASING Player " .. targetPlayerID)
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
    print("[Enemy " .. entityID .. "] ATTACKING Player " .. targetPlayerID .. " for " .. config.attackDamage .. " damage at distance " .. distance)

    local success = DamageEntity(targetPlayerID, config.attackDamage)
    print("[Enemy " .. entityID .. "] DamageEntity returned: " .. tostring(success))

    if success then
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

    -- Recalculate closest player BEFORE pathfinding
    local closestPlayer, closestDistance = FindClosestPlayer()

    if closestPlayer and closestPlayer > 0 then
        if closestPlayer ~= targetPlayerID then
            -- Clear old path since we're changing targets
            currentPath = {}
            pathIndex = 1
        end
        targetPlayerID = closestPlayer
    else
        FinishEnemyAction()
        return
    end

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
        print("[Enemy " .. entityID .. "] Need new path: path empty or completed")
    elseif lastKnownPlayerX ~= playerX or lastKnownPlayerY ~= playerY then
        -- Player moved - recalculate path immediately instead of waiting
        needsNewPath = true
        turnsSincePathUpdate = 0
        print("[Enemy " .. entityID .. "] Need new path: player moved from (" .. tostring(lastKnownPlayerX) .. ", " .. tostring(lastKnownPlayerY) .. ") to (" .. playerX .. ", " .. playerY .. ")")
    end

    -- Calculate path to player
    if needsNewPath then
        print("[Enemy " .. entityID .. "] Finding path from (" .. enemyX .. ", " .. enemyY .. ") to Player " .. targetPlayerID .. " at (" .. playerX .. ", " .. playerY .. ")")

        -- CRITICAL FIX: Pass grid coordinates, not entity ID!
        currentPath = FindPathToTarget(enemyX, enemyY, playerX, playerY)
        pathIndex = 1
        lastKnownPlayerX = playerX
        lastKnownPlayerY = playerY

        if not currentPath or #currentPath == 0 then
            print("[Enemy " .. entityID .. "] NO PATH FOUND from (" .. enemyX .. ", " .. enemyY .. ") to player at (" .. playerX .. ", " .. playerY .. ") - stuck!")
            FinishEnemyAction()
            return
        else
            print("[Enemy " .. entityID .. "] Path found with " .. #currentPath .. " steps:")
            -- Print first few steps of the path
            for i = 1, math.min(3, #currentPath) do
                print("  Step " .. i .. ": (" .. currentPath[i].x .. ", " .. currentPath[i].y .. ")")
            end
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
            print("[Enemy " .. entityID .. "] Close to player - reserving " .. apNeededForAttack .. " AP for attack (can move " .. maxMoves .. " times)")
        else
            print("[Enemy " .. entityID .. "] Close to player but not enough AP to reserve for attack - using all AP for movement")
        end
    end

    -- Move ONE tile per frame (not all at once!)
    if currentAP >= config.apCostPerMove and pathIndex <= #currentPath and movesThisTurn < maxMoves then
        local nextTile = currentPath[pathIndex]

        print("[Enemy " .. entityID .. "] Attempting to move to tile (" .. nextTile.x .. ", " .. nextTile.y .. ") - move " .. (movesThisTurn + 1) .. " of " .. maxMoves)

        -- Validate tile is still walkable
        if IsWalkableTile(nextTile.x, nextTile.y) then
            local success = MoveEntityToTile(entityID, nextTile.x, nextTile.y)

            if success then
                print("[Enemy " .. entityID .. "] Successfully moved to (" .. nextTile.x .. ", " .. nextTile.y .. ")")
                ConsumeEnemyAP(entityID, config.apCostPerMove)
                currentAP = currentAP - config.apCostPerMove
                movesThisTurn = movesThisTurn + 1
                pathIndex = pathIndex + 1

                -- Visual feedback
                ShowTileBorder(nextTile.x, nextTile.y, 0.3)
                PulseTile(nextTile.x, nextTile.y, 0.2, 1.0, 0.5, 0.0)  -- Orange pulse

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
    print("[Enemy " .. entityID .. "] ===== FindClosestPlayer() CALLED =====")

    -- Get enemy position
    local enemyX, enemyY = GetEntityGridPosition(entityID)
    if not enemyX then
        print("[Enemy " .. entityID .. "] ERROR: GetEntityGridPosition returned nil!")
        return nil
    end
    print("[Enemy " .. entityID .. "] Enemy position: (" .. enemyX .. ", " .. enemyY .. ")")

    -- Use GetAllPlayers() (C++ function available in all Lua states)
    print("[Enemy " .. entityID .. "] Calling GetAllPlayers() from C++...")
    local players = GetAllPlayers()
    print("[Enemy " .. entityID .. "] GetAllPlayers() returned type: " .. type(players))

    -- DEBUG: Show what GetAllPlayers() returned
    if players and type(players) == "table" then
        print("[Enemy " .. entityID .. "] GetAllPlayers() returned table with " .. #players .. " entries")

        if #players > 0 then
            local playerList = ""
            for i, pid in ipairs(players) do
                playerList = playerList .. pid
                if i < #players then playerList = playerList .. ", " end
            end
            print("[Enemy " .. entityID .. "] GetAllPlayers() found: [" .. playerList .. "]")
        else
            print("[Enemy " .. entityID .. "] GetAllPlayers() returned EMPTY table - using fallback FindPlayer()")
            return FindPlayer()
        end
    else
        print("[Enemy " .. entityID .. "] GetAllPlayers() returned nil or non-table - using fallback")
        return FindPlayer()
    end

    -- Find closest player
    local closestPlayerID = nil
    local closestDistance = 999999

    for i, playerID in ipairs(players) do
        local playerX, playerY = GetEntityGridPosition(playerID)
        if playerX then
            local distance = CalculateDistance(enemyX, enemyY, playerX, playerY)
            print("[Enemy " .. entityID .. "]   Player " .. playerID .. " at (" .. playerX .. ", " .. playerY .. ") - distance: " .. distance)
            if distance < closestDistance then
                closestDistance = distance
                closestPlayerID = playerID
                print("[Enemy " .. entityID .. "]     ^^ NEW CLOSEST!")
            end
        else
            print("[Enemy " .. entityID .. "]   Player " .. playerID .. " - ERROR: GetEntityGridPosition returned nil!")
        end
    end

    if closestPlayerID then
        print("[Enemy " .. entityID .. "] === CLOSEST: Player " .. closestPlayerID .. " at distance " .. closestDistance .. " ===")
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
