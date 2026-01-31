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

    -- Try to find player automatically
    targetPlayerID = FindPlayer()
    if targetPlayerID and targetPlayerID > 0 then
        Log("[EnemyScript] Enemy " .. entityID .. " locked onto Player " .. targetPlayerID)
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
            _G.EnemyCameraPannedThisTurn = false  -- Reset camera flag for new enemy turn
        end
        lastEnemyTurn = currentTurn
        return
    end

    -- Track that this is enemy turn
    lastEnemyTurn = "Enemy"

    -- Check if this enemy has already acted this turn
    if hasActedThisTurn then
        -- Already acted, don't process again
        return
    end

    print("[Enemy " .. entityID .. "] Starting turn")

    -- Pan camera to this enemy when starting to act (only first enemy)
    if not isMyTurnToAct then
        isMyTurnToAct = true
        -- Only pan camera if no other enemy has panned it yet this turn
        if not _G.EnemyCameraPannedThisTurn then
            _G.EnemyCameraPannedThisTurn = true
            SetCameraFollowTarget(entityID)
        end
    end

    -- CRITICAL: Find closest player dynamically each turn
    local closestPlayer, closestDistance = FindClosestPlayer()

    if closestPlayer and closestPlayer > 0 then
        print("[Enemy " .. entityID .. "] Targeting Player " .. closestPlayer .. " (distance: " .. closestDistance .. ")")
        targetPlayerID = closestPlayer
    else
        print("[Enemy " .. entityID .. "] No player found!")
        hasActedThisTurn = true  -- Mark as acted even if no target
        CheckAllEnemiesActed()
        return  -- No player to target
    end

    -- Execute AI decision making
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

    if not currentAP or currentAP == 0 or currentAP < config.apCostPerMove then
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
    Log("[EnemyScript] Enemy " .. entityID .. " attacks Player " .. targetPlayerID .. " for " .. config.attackDamage .. " damage!")

    local success = DamageEntity(targetPlayerID, config.attackDamage)
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
    elseif lastKnownPlayerX ~= playerX or lastKnownPlayerY ~= playerY then
        turnsSincePathUpdate = turnsSincePathUpdate + 1
        if turnsSincePathUpdate >= config.recalculatePathEveryNTurns then
            needsNewPath = true
            turnsSincePathUpdate = 0
        end
    end

    -- Calculate path to player
    if needsNewPath then
        currentPath = FindPathToTarget(entityID, playerX, playerY)
        pathIndex = 1
        lastKnownPlayerX = playerX
        lastKnownPlayerY = playerY

        if not currentPath or #currentPath == 0 then
            Log("[EnemyScript] Enemy " .. entityID .. " could not find path to player")
            FinishEnemyAction()
            return
        end
    end

    -- Move along path
    local movesMade = 0
    while currentAP >= config.apCostPerMove and pathIndex <= #currentPath and movesMade < config.maxMovesPerTurn do
        local nextTile = currentPath[pathIndex]

        -- Validate tile is still walkable
        if IsWalkableTile(nextTile.x, nextTile.y) then
            local success = MoveEntityToTile(entityID, nextTile.x, nextTile.y)

            if success then
                ConsumeEnemyAP(entityID, config.apCostPerMove)
                currentAP = currentAP - config.apCostPerMove
                movesMade = movesMade + 1
                pathIndex = pathIndex + 1

                -- Visual feedback
                ShowTileBorder(nextTile.x, nextTile.y, 0.3)
                PulseTile(nextTile.x, nextTile.y, 0.2, 1.0, 0.5, 0.0)  -- Orange pulse
            else
                -- Movement failed, recalculate path next turn
                currentPath = {}
                break
            end
        else
            -- Tile became unwalkable, recalculate path
            currentPath = {}
            break
        end
    end

    -- Check if we're now in attack range after moving
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
        return nil
    end

    -- Use GetAllPlayers() (C++ function available in all Lua states)
    local players = GetAllPlayers()

    -- DEBUG: Show what GetAllPlayers() returned
    if players and type(players) == "table" and #players > 0 then
        local playerList = ""
        for i, pid in ipairs(players) do
            playerList = playerList .. pid
            if i < #players then playerList = playerList .. ", " end
        end
        print("[Enemy " .. entityID .. "] GetAllPlayers() found: [" .. playerList .. "]")
    else
        print("[Enemy " .. entityID .. "] GetAllPlayers() returned no players - using fallback")
        return FindPlayer()
    end

    -- Find closest player
    local closestPlayerID = nil
    local closestDistance = 999999

    for i, playerID in ipairs(players) do
        local playerX, playerY = GetEntityGridPosition(playerID)
        if playerX then
            local distance = CalculateDistance(enemyX, enemyY, playerX, playerY)
            if distance < closestDistance then
                closestDistance = distance
                closestPlayerID = playerID
            end
        end
    end

    return closestPlayerID, closestDistance
end

function GetEntityHP(entity)
    -- TODO: Add GetEntityHP(entityID) API to LevelLoader
    -- For now, check if it's the player
    if entity == targetPlayerID then
        local hp, maxHP = GetPlayerHP()
        if hp and maxHP then
            return hp, maxHP
        end
    end

    -- Return default HP for enemies (assume full health for now)
    -- This means flee behavior won't trigger until API is added
    return 5, 5
end

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

-- Mark this enemy as having completed its turn and check if all enemies are done
function FinishEnemyAction()
    hasActedThisTurn = true
    Log("[Enemy " .. entityID .. "] Finished turn")
    CheckAllEnemiesActed()
end

-- Check if all enemies have acted, and if so, end the enemy turn
function CheckAllEnemiesActed()
    local enemies = GetAllEnemies()
    if not enemies or #enemies == 0 then
        return
    end

    -- Find this enemy's index in the enemy list
    local myIndex = nil
    for i, enemyID in ipairs(enemies) do
        if enemyID == entityID then
            myIndex = i
            break
        end
    end

    if not myIndex then
        return
    end

    -- If this is the last enemy, end the enemy turn
    if myIndex == #enemies then
        -- Pan camera back to the active player character before ending turn
        local activePlayerID = GetActiveCharacter()
        if activePlayerID and activePlayerID > 0 then
            SetCameraFollowTarget(activePlayerID)
        end

        EndEnemyTurn()
    end
end

-- ============================================================================
-- Export behavior types for level scripts
-- ============================================================================
ENEMY_BEHAVIOR = BEHAVIOR
