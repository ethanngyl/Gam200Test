--[[
===============================================================================
 File:           EnemyScript.lua
 Author:         Converted from PathfindingSystem.cpp
 Type:           Entity Component Script
 Description:    Enemy AI component for pathfinding and combat

 Usage:
   1. Add ScriptComponent to enemy entity in Entity Inspector
   2. Set script path to "assets/scripts/EnemyScript.lua"
   3. Enemy will automatically chase and attack player

 Features:
 - A* pathfinding to chase player
 - Sequential turn-based movement
 - AP-based movement system
 - Melee combat when adjacent to player
 - Automatic path recalculation
===============================================================================
--]]

-- Entity-specific state
local entity = nil
local currentPath = {}
local pathIndex = 0
local moveTimer = 0.0
local moveDelay = 0.7  -- Seconds between moves
local targetEntity = nil
local hasReachedTarget = false

--[[
    Called once when entity is created
    @param entityID The ID of this entity
--]]
function OnInit(entityID)
    entity = entityID
    currentPath = {}
    pathIndex = 0
    moveTimer = 0.0
    hasReachedTarget = false

    -- Find player as target
    targetEntity = FindPlayer()

    Log("EnemyScript initialized on entity: " .. tostring(entityID))
end

--[[
    Called every frame during enemy turn
    @param dt Delta time in seconds
--]]
function OnUpdate(dt)
    -- Only update during enemy turn
    local turn = GetCurrentTurn()
    if turn ~= "Enemy" then
        return
    end

    -- Update movement timer
    moveTimer = moveTimer - dt
    if moveTimer > 0.0 then
        return  -- Still waiting
    end

    -- Get enemy AP (assuming enemy has AP component)
    local ap = GetEnemyAP(entity)
    if not ap or ap <= 0 then
        return  -- Out of AP
    end

    -- Validate target
    if not targetEntity then
        targetEntity = FindPlayer()
        if not targetEntity then
            return
        end
    end

    -- Get current and target positions
    local enemyX, enemyY = GetEntityGridPosition(entity)
    if not enemyX or not enemyY then
        return
    end

    local targetX, targetY = GetPlayerGridPosition()
    if not targetX or not targetY then
        return
    end

    -- Check if adjacent to target (Manhattan distance = 1)
    local distance = math.abs(enemyX - targetX) + math.abs(enemyY - targetY)

    if distance == 1 then
        -- ATTACK!
        AttackTarget(entity, targetEntity)
        moveTimer = moveDelay
        ConsumeEnemyAP(entity, 1)
        return
    end

    -- Calculate path to target
    currentPath = FindPathToTarget(enemyX, enemyY, targetX, targetY)

    if not currentPath or #currentPath == 0 then
        Log("Enemy " .. entity .. ": No path found!")
        ConsumeEnemyAP(entity, 999)  -- End turn
        return
    end

    -- Skip starting position if it's in path
    if currentPath[1].x == enemyX and currentPath[1].y == enemyY then
        pathIndex = 2
    else
        pathIndex = 1
    end

    -- Move to next tile in path
    if pathIndex <= #currentPath then
        local nextTile = currentPath[pathIndex]

        -- Check if tile is walkable
        if IsWalkableTile(nextTile.x, nextTile.y) or
           (nextTile.x == targetX and nextTile.y == targetY) then

            -- Move enemy
            MoveEntityToTile(entity, nextTile.x, nextTile.y)

            -- Consume AP
            ConsumeEnemyAP(entity, 1)

            -- Reset timer
            moveTimer = moveDelay

            Log("Enemy " .. entity .. " moved to (" .. nextTile.x .. "," .. nextTile.y .. ")")
        end
    end
end

--[[
    Attack the target entity
    @param attacker The attacking entity
    @param target The target entity
--]]
function AttackTarget(attacker, target)
    -- Deal damage (requires C++ API)
    DamageEntity(target, 1)

    Log("Enemy " .. attacker .. " ATTACKS target!")

    -- Play attack sound
    PlaySound("damage_basic")
end

--[[
    Called when entity is destroyed
--]]
function OnDestroy()
    Log("EnemyScript destroyed on entity: " .. tostring(entity))
end
