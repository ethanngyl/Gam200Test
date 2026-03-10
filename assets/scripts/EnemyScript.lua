--[[
===============================================================================
 File:          EnemyScript.lua
 Authors:       Ethan Ng Yong Le
 Date:          01/25/2026
 Contribution:  100%
 ------------------------------------------------------------------------------

 ENEMY SCRIPT - AI Behavior for Grid-Based Turn-Based Combat

 Brief:
    Handles enemy turn logic, pathfinding, movement, and attacking on a
    grid-based tactical map. Uses a state machine to manage behavior modes
    (Idle, Chasing, Attacking, Fleeing, Patrolling). Supports multiple
    behavior types for different enemy archetypes. Integrates with the
    sequential enemy turn system for coordinated multi-enemy turns.

 Behavior Types:
    AGGRESSIVE  - Always chase and attack the closest player
    DEFENSIVE   - Only attack if player enters aggro range
    PATROL      - Move randomly unless player is nearby
    RANGED      - Maintain distance, attack from afar

 States:
    [IDLE] -> Player detected -> [CHASING] -> In range -> [ATTACKING]
    [CHASING] -> Low health -> [FLEEING]
    [IDLE] -> No player -> [PATROLLING]

 Usage:
    -- Attach to enemy entity
    AddScriptComponentToEntity(enemyID, "assets/scripts/EnemyScript.lua")

    -- Configure behavior (optional, call from level script)
    SetBehavior(ENEMY_BEHAVIOR.AGGRESSIVE)
    SetAttackRange(2)
    SetAttackDamage(1)
    SetAggression(8)


 Copyright (C) 2026 DigiPen Institute of Technology.
 Reproduction or disclosure of this file or its contents
 without the prior written consent of DigiPen Institute of
 Technology is prohibited.
===============================================================================
]]--
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
local moveDelay = 0.55           -- Delay between moves in seconds (0.3s = visible movement)
local movesThisTurn = 0         -- Track how many moves made this turn

-- Safety: track how long this enemy has been active to detect stuck state
local activeTimer = 0.0         -- Time spent as active enemy this turn
local maxActiveTime = 8.0       -- Max seconds before force-finishing turn

-- Smooth glide state (lerps sprite between tiles)
local glideActive = false
local glideElapsed = 0.0
local glideDuration = 0.5      -- Seconds to glide (tune to taste)
local glideStartX = 0.0
local glideStartY = 0.0
local glideEndX = 0.0
local glideEndY = 0.0
local pendingFinishAfterGlide = false  -- When true, FinishEnemyAction() is called after glide ends

-- Health bar state
local healthBarBG = nil        -- Background bar entity (dark)
local healthBarFG = nil        -- Foreground bar entity (colored)
local healthBarWidth = 0.1     -- Total bar width in world units
local healthBarHeight = 0.02  -- Bar height in world units
local healthBarOffsetY = 0.05  -- How far above enemy center
local healthBarLayer = 2       -- Render layer (below UI)

-- ============================================================================
-- ENEMY ANIMATION (manual sprite sheet switching)
-- NOTE: Enemy.zip only has Idle + Attack, so movement uses Idle as "walk".
-- ============================================================================

local ENEMY_ANIM = {
    idleFront  = { tex = "assets/Enemy/Enemy_Knight_Idle_Front-Sheet.png",  rows = 1, cols = 12, frames = 12, time = 0.08, loop = true  },
    idleBack   = { tex = "assets/Enemy/Enemy_Knight_Idle_Back-Sheet.png",   rows = 1, cols = 4,  frames = 4,  time = 0.10, loop = true  },
    idleSide   = { tex = "assets/Enemy/Enemy_Knight_Idle_Side-Sheet.png",   rows = 1, cols = 12, frames = 12, time = 0.08, loop = true  },

    atkFront   = { tex = "assets/Enemy/Enemy_Knight_Attack_Front-Sheet.png", rows = 1, cols = 8, frames = 8, time = 0.07, loop = false },
    atkBack    = { tex = "assets/Enemy/Enemy_Knight_Attack_Back-Sheet.png",  rows = 1, cols = 8, frames = 8, time = 0.07, loop = false },
    atkLeft    = { tex = "assets/Enemy/Enemy_Knight_Attack_Left-Sheet.png",  rows = 1, cols = 7, frames = 7, time = 0.07, loop = false }
}

local lastAnimKey = nil
local lastFlipX = false

-- Pending attack (play animation first, then apply damage)
local pendingAttack = false
local pendingAttackTimer = 0.0
local pendingAttackTarget = 0
local pendingAttackDamage = 0
local pendingAttackAPCost = 0
local pendingAttackPlayerX = 0
local pendingAttackPlayerY = 0

local function ApplySheet(animKey, flipX)
    if not SetSpriteAnimationSheet then
        print("[EnemyScript] SetSpriteAnimationSheet is NIL! animKey=" .. tostring(animKey))
        return
    end

    if animKey == lastAnimKey and flipX == lastFlipX then
        return
    end

    local a = ENEMY_ANIM[animKey]
    if not a then
        print("[EnemyScript] Missing ENEMY_ANIM key: " .. tostring(animKey))
        return
    end

    local ok = SetSpriteAnimationSheet(entityID, a.tex, a.rows, a.cols, a.frames, a.time, a.loop)
    print("[EnemyScript] ApplySheet entity=" .. tostring(entityID) ..
          " key=" .. tostring(animKey) ..
          " flipX=" .. tostring(flipX) ..
          " tex=" .. tostring(a.tex) ..
          " ok=" .. tostring(ok))

    if SetAnimationFlipX then
        SetAnimationFlipX(entityID, flipX and true or false)
    end

    lastAnimKey = animKey
    lastFlipX = flipX and true or false
end


local function SetFacingFromDelta(dx, dy)
    -- dy>0 means target is "above" enemy on grid => back view (match your existing convention)
    if math.abs(dx) > math.abs(dy) then
        -- Side
        -- Use idleSide; flipX true when facing LEFT (dx < 0), false when facing RIGHT (dx > 0)
        ApplySheet("idleSide", dx < 0)
    else
        if dy > 0 then
            ApplySheet("idleBack", false)
        else
            ApplySheet("idleFront", false)
        end
    end
end

local function SetAttackFacing(enemyX, enemyY, playerX, playerY)
    local dx = playerX - enemyX
    local dy = playerY - enemyY

    if math.abs(dx) > math.abs(dy) then
        -- Side attack: we only have LEFT sheet, so flip for right
        -- If player is to the RIGHT (dx > 0), flipX = true to mirror left attack to right
        ApplySheet("atkLeft", dx > 0)
    else
        if dy > 0 then
            ApplySheet("atkBack", false)
        else
            ApplySheet("atkFront", false)
        end
    end
end


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

    ApplySheet("idleFront", false)

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

    -- Spawn health bar sprites above enemy
    local wx, wy = GetEntityWorldPosition(entityID)
    if wx and wy then
        local barY = wy + healthBarOffsetY
        -- Background (dark gray)
        healthBarBG = SpawnSprite("", wx, barY, healthBarWidth, healthBarHeight, healthBarLayer)
        if healthBarBG and healthBarBG > 0 then
            SetSpriteColor(healthBarBG, 0.15, 0.15, 0.15, 0.85)
        end
        -- Foreground (green, drawn on top)
        healthBarFG = SpawnSprite("", wx, barY, healthBarWidth, healthBarHeight, healthBarLayer + 1)
        if healthBarFG and healthBarFG > 0 then
            SetSpriteColor(healthBarFG, 0.0, 0.85, 0.0, 1.0)
        end
        Log("[EnemyScript] Enemy " .. entityID .. " health bar spawned (BG=" .. tostring(healthBarBG) .. " FG=" .. tostring(healthBarFG) .. ")")
    end
end

-- Update health bar position and fill based on current HP
local function UpdateHealthBar()
    if not healthBarBG or not healthBarFG then return end
    if healthBarBG <= 0 or healthBarFG <= 0 then return end

    local wx, wy = GetEntityWorldPosition(entityID)
    if not wx or not wy then return end

    local barY = wy + healthBarOffsetY

    -- Position background (always full width, centered on enemy)
    SetSpritePosition(healthBarBG, wx, barY)

    -- Get HP ratio
    local currentHP, maxHP = GetEntityHP(entityID)
    if not currentHP or not maxHP or maxHP <= 0 then return end

    local ratio = currentHP / maxHP
    if ratio < 0 then ratio = 0 end
    if ratio > 1 then ratio = 1 end

    -- Scale foreground width by HP ratio, offset to align left edges
    local fgWidth = healthBarWidth * ratio
    local fgX = wx - (healthBarWidth - fgWidth) * 0.5
    SetSpritePosition(healthBarFG, fgX, barY)
    SetScale(healthBarFG, fgWidth, healthBarHeight)

    -- Color: green -> yellow -> red based on HP
    local r, g, b = 0.0, 0.85, 0.0
    if ratio <= 0.25 then
        r, g, b = 0.9, 0.1, 0.1       -- red
    elseif ratio <= 0.5 then
        r, g, b = 0.95, 0.65, 0.0      -- orange
    elseif ratio <= 0.75 then
        r, g, b = 0.95, 0.95, 0.0      -- yellow
    end
    SetSpriteColor(healthBarFG, r, g, b, 1.0)
end

function OnUpdate(dt)
    -- Update health bar every frame (runs regardless of turn)
    UpdateHealthBar()

    -- Enemy AI only runs during enemy turn
    local currentTurn = GetCurrentTurn()

    if currentTurn == "Enemy" then
        -- Only print once when entering enemy turn to avoid spam
        if not isMyTurnToAct and not hasActedThisTurn then
            print("[Enemy " .. entityID .. " FRAME] turn=Enemy glide=" .. tostring(glideActive) .. " acted=" .. tostring(hasActedThisTurn))
        end
    end

    if currentTurn ~= "Enemy" then
        -- Reset acted flag when it's not enemy turn
        if lastEnemyTurn == "Enemy" then
            hasActedThisTurn = false
            isMyTurnToAct = false
            moveTimer = 0.0
            movesThisTurn = 0
            activeTimer = 0.0
            -- Snap glide to end position if interrupted by turn change
            if glideActive then
                SetSpritePosition(entityID, glideEndX, glideEndY)
                glideActive = false
            end
        end
        lastEnemyTurn = currentTurn
        return
    end

    -- Track that this is enemy turn
    lastEnemyTurn = "Enemy"

    -- Smooth glide: interpolate sprite position each frame
    if glideActive then
        glideElapsed = glideElapsed + dt
        local t = glideElapsed / glideDuration
        if t > 1.0 then t = 1.0 end

        -- Ease-out quadratic
        local eased = 1.0 - (1.0 - t) * (1.0 - t)

        local x = glideStartX + (glideEndX - glideStartX) * eased
        local y = glideStartY + (glideEndY - glideStartY) * eased
        SetSpritePosition(entityID, x, y)
        print("[Enemy " .. entityID .. " GLIDE UPDATE] t=" .. string.format("%.2f", t) .. " pos=(" .. string.format("%.4f", x) .. "," .. string.format("%.4f", y) .. ")")  -- THIS ONE

        if t >= 1.0 then
            SetSpritePosition(entityID, glideEndX, glideEndY)
            glideActive = false
            print("[Enemy " .. entityID .. " GLIDE] Complete!")
            
            -- If Flee/Patrol requested finish after glide, do it now
            if pendingFinishAfterGlide then
                pendingFinishAfterGlide = false
                FinishEnemyAction()
                return
            end
            
            -- Set move delay AFTER glide completes (not when glide starts)
            -- This prevents stacking glide duration + moveDelay
            moveTimer = moveDelay
        end
        -- While gliding, don't process AI (wait for glide to finish)
        if glideActive then
            return
        end
    end

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

    -- Check if this enemy is stunned (status effects are decremented centrally in ResetPartyTurn)
    if HasStatusEffect and HasStatusEffect(entityID, "stun") then
        print("[Enemy " .. entityID .. "] STUNNED - skipping turn")
        FinishEnemyAction()
        return
    end

    -- Earthen Bind: consume extra AP to reduce movement this turn
    if HasStatusEffect and HasStatusEffect(entityID, "earthenBind") then
        local reduction = 2  -- movement points reduced
        ConsumeEnemyAP(entityID, reduction)
        print("[Enemy " .. entityID .. "] EARTHEN BIND - movement AP reduced by " .. reduction)
    end

    -- Mana Drain: consume AP to reduce available actions this turn
    if HasStatusEffect and HasStatusEffect(entityID, "manaDrain") then
        local reduction = 1
        ConsumeEnemyAP(entityID, reduction)
        print("[Enemy " .. entityID .. "] MANA DRAIN - AP reduced by " .. reduction)
    end

    -- Status effects are decremented centrally in ResetPartyTurn (once per turn cycle)

    -- Update move timer
    if moveTimer > 0 then
        moveTimer = moveTimer - dt
        if moveTimer < 0 then
            moveTimer = 0
        end
        return
    end

    

    -- Safety: track active time and force-finish if stuck
    activeTimer = activeTimer + dt
    if activeTimer >= maxActiveTime then
        print("[Enemy " .. entityID .. "] SAFETY: Active for " .. string.format("%.1f", activeTimer) .. "s, force-finishing turn!")
        FinishEnemyAction()
        return
    end

    -- First time acting - set up turn
    if not isMyTurnToAct then
        isMyTurnToAct = true
        movesThisTurn = 0
        activeTimer = 0.0  -- Reset active timer at turn start
        print("[Enemy " .. entityID .. "] ========== STARTING TURN ==========")
        print("[Enemy " .. entityID .. "] glideActive=" .. tostring(glideActive) .. " moveTimer=" .. moveTimer .. " hasActed=" .. tostring(hasActedThisTurn))

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

        -- If an attack is pending, play animation first, then apply damage when timer ends
    if pendingAttack then
        pendingAttackTimer = pendingAttackTimer - dt
        if pendingAttackTimer > 0 then
            return
        end

        -- Timer finished: apply damage now
        pendingAttack = false

        -- damageModifier on target already handles Bolstered Morale
        local totalDmg = pendingAttackDamage

        print("[Enemy " .. entityID .. "] ATTACK HIT Player " .. pendingAttackTarget .. " for " .. totalDmg .. " damage")

        local hpBefore, maxHP = GetEntityHP(pendingAttackTarget)
        print("[Enemy " .. entityID .. "] Player " .. pendingAttackTarget .. " HP BEFORE: " .. tostring(hpBefore) .. "/" .. tostring(maxHP))

        local success = DamageEntity(pendingAttackTarget, totalDmg, entityID)
        print("[Enemy " .. entityID .. "] DamageEntity returned: " .. tostring(success))

        if success then
            local hpAfter, _ = GetEntityHP(pendingAttackTarget)
            print("[Enemy " .. entityID .. "] Player " .. pendingAttackTarget .. " HP AFTER: " .. tostring(hpAfter))

            ConsumeEnemyAP(entityID, pendingAttackAPCost)

            PulseTile(pendingAttackPlayerX, pendingAttackPlayerY, 0.3, 1.0, 0.0, 0.0)

            if PopupManager and PopupManager.ShowDamageNumber then
                local worldX, worldY = GetEntityWorldPosition(pendingAttackTarget)
                if worldX then
                    PopupManager.ShowDamageNumber(worldX, worldY + 0.2, totalDmg)
                end
            end

            -- CHECK: Did we kill the player? If all players dead -> game over
            if not hpAfter or hpAfter <= 0 then
                print("[Enemy " .. entityID .. "] Player " .. pendingAttackTarget .. " KILLED!")

                -- Check if ALL players are dead
                local allDead = true
                local players = GetAllPlayers()
                if players and #players > 0 then
                    for _, pid in ipairs(players) do
                        local php, _ = GetEntityHP(pid)
                        if php and php > 0 then
                            allDead = false
                            break
                        end
                    end
                end

                if allDead then
                    print("[Enemy " .. entityID .. "] ALL PLAYERS DEFEATED - GAME OVER!")
                    if SetNextGameState then
                        SetNextGameState("LEVEL_END")
                    end
                    return  -- Stop processing, game is over
                end
            end
        else
            print("[Enemy " .. entityID .. "] ATTACK FAILED! Player " .. pendingAttackTarget .. " may not have Health component")
        end

        -- Return to idle after attack
        local ex, ey = GetEntityGridPosition(entityID)
        local px, py = GetEntityGridPosition(targetPlayerID)
        if ex and px then
            SetFacingFromDelta(px - ex, py - ey)
        else
            ApplySheet("idleFront", false)
        end

        -- Continue AI after resolving hit (may move if AP left)
        local ok, err = pcall(ProcessAITurn)
        if not ok then
            print("[Enemy " .. entityID .. "] ERROR in ProcessAITurn (post-attack): " .. tostring(err))
            FinishEnemyAction()
        end
        return
    end

    -- Execute AI decision making (will make ONE move per frame)
    -- Wrapped in pcall to catch errors and prevent stuck turns
    local aiOk, aiErr = pcall(ProcessAITurn)
    if not aiOk then
        print("[Enemy " .. entityID .. "] ERROR in ProcessAITurn: " .. tostring(aiErr))
        print("[Enemy " .. entityID .. "] Force-finishing turn due to error")
        FinishEnemyAction()
        return
    end

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
    -- Clean up health bar sprites
    if healthBarBG and healthBarBG > 0 then
        DestroyEntity(healthBarBG)
        healthBarBG = nil
    end
    if healthBarFG and healthBarFG > 0 then
        DestroyEntity(healthBarFG)
        healthBarFG = nil
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

    
    -- Play attack animation first, then apply damage when animation finishes
    print("[Enemy " .. entityID .. "] ATTACKING Player " .. targetPlayerID .. " (animation first)")

    SetAttackFacing(enemyX, enemyY, playerX, playerY)

    pendingAttack = true
    pendingAttackTarget = targetPlayerID
    pendingAttackDamage = config.attackDamage
    pendingAttackAPCost = config.attackAPCost
    pendingAttackPlayerX = playerX
    pendingAttackPlayerY = playerY

    -- Duration = frames * frameTime (small buffer to ensure last frame shows)
    local key = lastAnimKey
    local a = ENEMY_ANIM[key]
    if a then
        pendingAttackTimer = (a.frames * a.time) + 0.02
    else
        pendingAttackTimer = 0.20
    end

    moveTimer = 0.0
    -- Stop here; OnUpdate will apply damage when timer ends
    return

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

    print("[Enemy " .. entityID .. "] MOVE CHECK: AP=" .. currentAP .. " pathIdx=" .. pathIndex .. "/" .. #currentPath .. " moves=" .. movesThisTurn .. "/" .. maxMoves .. " glide=" .. tostring(glideActive))
    -- Move ONE tile per frame (not all at once!)
    if currentAP >= config.apCostPerMove and pathIndex <= #currentPath and movesThisTurn < maxMoves then
        local nextTile = currentPath[pathIndex]

        -- Validate tile is walkable (terrain check)
        if not IsWalkableTile(nextTile.x, nextTile.y) then
            -- Tile became unwalkable, recalculate path
            currentPath = {}
            FinishEnemyAction()
            return
        end

        -- Validate tile is not occupied by another entity
        local isOccupied = IsTileOccupied(nextTile.x, nextTile.y)
        if isOccupied then
            print("[Enemy " .. entityID .. "] Path blocked at (" .. nextTile.x .. ", " .. nextTile.y .. ") - tile occupied!")
            -- Tile is blocked, recalculate path next turn
            currentPath = {}
            FinishEnemyAction()
            return
        end

        -- Tile is valid and empty, move to it
        -- NOTE: MoveEntityToTile in C++ already handles:
        -- 1. Clearing occupancy at old position
        -- 2. Setting occupancy at new position
        -- No need to manually call SetTileOccupant here

        -- Update facing + idle-as-walk animation based on movement direction
        local dx = nextTile.x - enemyX
        local dy = nextTile.y - enemyY
        SetFacingFromDelta(dx, dy)

        -- SMOOTH GLIDE: Save start pos, snap grid, then lerp sprite back
        local sx, sy = GetEntityWorldPosition(entityID)
        print("[Enemy GLIDE] GetEntityWorldPosition returned sx=" .. tostring(sx) .. " sy=" .. tostring(sy))  -- THIS ONE


        local success = MoveEntityToTile(entityID, nextTile.x, nextTile.y)

            if success then
                -- Start glide: get end position, yank sprite back to start
                local ex, ey = GetEntityWorldPosition(entityID)
                print("[Enemy GLIDE] After move: ex=" .. tostring(ex) .. " ey=" .. tostring(ey))  -- THIS ONE
                if sx and sy and ex and ey then
                    glideActive = true
                    glideElapsed = 0.0
                    glideStartX = sx
                    glideStartY = sy
                    glideEndX = ex
                    glideEndY = ey
                    SetSpritePosition(entityID, sx, sy)  -- Yank back to start
                    print("[Enemy GLIDE] Started! (" .. sx .. "," .. sy .. ") -> (" .. ex .. "," .. ey .. ")")  -- THIS ONE
                else
                    print("[Enemy GLIDE] FAILED: nil positions! ...")  -- THIS ONE
                end

                ConsumeEnemyAP(entityID, config.apCostPerMove)
                currentAP = currentAP - config.apCostPerMove
                movesThisTurn = movesThisTurn + 1
                pathIndex = pathIndex + 1

                -- Visual feedback
                ShowTileBorder(nextTile.x, nextTile.y, 0.3)
                PulseTile(nextTile.x, nextTile.y, 0.2, 1.0, 0.5, 0.0)  -- Orange pulse

                -- Don't set moveTimer here - it's set in glide completion handler
                -- This prevents double-stacking glide duration + moveDelay

                -- Return to let next frame handle the glide, then the next move
                return
            else
                -- Movement failed, recalculate path next turn
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
    if currentAP >= config.apCostPerMove then
        -- Check if flee tile is walkable and not occupied
        if IsWalkableTile(fleeX, fleeY) and not IsTileOccupied(fleeX, fleeY) then
            local sx, sy = GetEntityWorldPosition(entityID)
            local success = MoveEntityToTile(entityID, fleeX, fleeY)
            if success then
                -- Start glide
                local ex, ey = GetEntityWorldPosition(entityID)
                if sx and sy and ex and ey then
                    glideActive = true
                    glideElapsed = 0.0
                    glideStartX = sx
                    glideStartY = sy
                    glideEndX = ex
                    glideEndY = ey
                    SetSpritePosition(entityID, sx, sy)
                end
                ConsumeEnemyAP(entityID, config.apCostPerMove)
                PulseTile(fleeX, fleeY, 0.2, 1.0, 1.0, 0.0)  -- Yellow pulse (fleeing)
            end
        else
            print("[Enemy " .. entityID .. "] Cannot flee - tile (" .. fleeX .. ", " .. fleeY .. ") blocked!")
        end
    end

    -- If glide started, finish turn after glide completes; otherwise finish now
    if glideActive then
        pendingFinishAfterGlide = true
    else
        FinishEnemyAction()
    end
    -- If glideActive, the glide will complete in OnUpdate, then FinishEnemyAction()
    -- will be called via the pendingFinishAfterGlide flag
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

    -- Check if patrol tile is walkable and not occupied
    if IsWalkableTile(newX, newY) and not IsTileOccupied(newX, newY) then
        local sx, sy = GetEntityWorldPosition(entityID)
        local success = MoveEntityToTile(entityID, newX, newY)
        if success then
            -- Start glide
            local ex, ey = GetEntityWorldPosition(entityID)
            if sx and sy and ex and ey then
                glideActive = true
                glideElapsed = 0.0
                glideStartX = sx
                glideStartY = sy
                glideEndX = ex
                glideEndY = ey
                SetSpritePosition(entityID, sx, sy)
            end
            ConsumeEnemyAP(entityID, config.apCostPerMove)
            PulseTile(newX, newY, 0.2, 0.5, 0.5, 1.0)  -- Blue pulse (patrol)
        end
    end

    -- If glide started, finish turn after glide completes; otherwise finish now
    if glideActive then
        pendingFinishAfterGlide = true
    else
        FinishEnemyAction()
    end
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
    glideActive = false -- Reset glide
    pendingFinishAfterGlide = false  -- Reset pending flag
    pendingAttack = false -- Reset pending attack
    activeTimer = 0.0  -- Reset active timer
    currentPath = {}   -- Clear stale path
    pathIndex = 1
    Log("[Enemy " .. entityID .. "] Finished turn")
    MarkEnemyActionComplete()  -- Advance to next enemy in sequence
end

-- REMOVED: CheckAllEnemiesActed() - replaced by EnemyTurnManager
-- Sequential turn system now handles enemy turn advancement and camera panning

-- ============================================================================
-- Export behavior types for level scripts
-- ============================================================================
ENEMY_BEHAVIOR = BEHAVIOR