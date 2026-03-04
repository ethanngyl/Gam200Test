--[[
===============================================================================
 File:          EnemyMageScript.lua
 Date:          03/03/2026
 ------------------------------------------------------------------------------

 ENEMY MAGE - Ranged Support Enemy

 Stats: HP 3, AP 4, MP 3

 Skills:
    1. Arcane Bolt - Fires projectile in facing direction, 1 damage, 2 AP
    2. Barrier     - Blocks next damage taken for target character, 2 AP.
                     2-turn cooldown. Priority usage. Targets random enemy
                     ally if possible, otherwise targets self.

 AI Priority:
    Barrier (if off cooldown + allies exist) > Move to range > Arcane Bolt

 Copyright (C) 2026 DigiPen Institute of Technology.
===============================================================================
]]--

-- ============================================================================
-- CONFIGURATION
-- ============================================================================

local PHASE = {
    INIT    = "init",
    BARRIER = "barrier",
    MOVE    = "move",
    ATTACK  = "attack",
    DONE    = "done"
}

local entityID = 0
local targetPlayerID = 0

local config = {
    -- Stats
    maxHP = 3,
    maxAP = 4,
    movementPoints = 3,

    -- Arcane Bolt
    arcaneBoltAPCost = 2,
    arcaneBoltDamage = 1,
    arcaneBoltSpeed = 3.0,
    arcaneBoltRange = 7,    -- Max tiles before despawn

    -- Barrier
    barrierAPCost = 2,
    barrierCooldownMax = 2,

    -- AI
    preferredDistance = 3,  -- Try to stay this far from players
    aggroRange = 99,
    maxPathLength = 10
}

-- ============================================================================
-- INTERNAL STATE
-- ============================================================================

local hasActedThisTurn = false
local lastEnemyTurn = nil
local isMyTurnToAct = false
local currentTurnPhase = PHASE.INIT

-- Movement
local moveTimer = 0.0
local moveDelay = 0.55
local mpRemaining = 0
local currentPath = {}
local pathIndex = 1
local pathTargetPlayerID = 0
local lastKnownPlayerX = nil
local lastKnownPlayerY = nil

-- Safety
local activeTimer = 0.0
local maxActiveTime = 8.0

-- Glide
local glideActive = false
local glideElapsed = 0.0
local glideDuration = 0.5
local glideStartX, glideStartY = 0.0, 0.0
local glideEndX, glideEndY = 0.0, 0.0

-- Barrier
local barrierCooldown = 0

-- Facing direction for projectile
local facingDirX = 0
local facingDirY = -1   -- Default: facing down (front)

-- Pending projectile (wait for animation)
local pendingProjectile = false
local pendingProjectileTimer = 0.0

-- Health bar
local healthBarBG = nil
local healthBarFG = nil
local healthBarWidth = 0.1
local healthBarHeight = 0.02
local healthBarOffsetY = 0.05
local healthBarLayer = 50

-- ============================================================================
-- ANIMATION
-- ============================================================================

local ENEMY_ANIM = {
    idleFront = { tex = "assets/Enemy/Enemy_Knight_Idle_Front-Sheet.png",  rows = 1, cols = 12, frames = 12, time = 0.08, loop = true  },
    idleBack  = { tex = "assets/Enemy/Enemy_Knight_Idle_Back-Sheet.png",   rows = 1, cols = 4,  frames = 4,  time = 0.10, loop = true  },
    idleSide  = { tex = "assets/Enemy/Enemy_Knight_Idle_Side-Sheet.png",   rows = 1, cols = 12, frames = 12, time = 0.08, loop = true  },
    atkFront  = { tex = "assets/Enemy/Enemy_Knight_Attack_Front-Sheet.png", rows = 1, cols = 8, frames = 8, time = 0.07, loop = false },
    atkBack   = { tex = "assets/Enemy/Enemy_Knight_Attack_Back-Sheet.png",  rows = 1, cols = 8, frames = 8, time = 0.07, loop = false },
    atkLeft   = { tex = "assets/Enemy/Enemy_Knight_Attack_Left-Sheet.png",  rows = 1, cols = 7, frames = 7, time = 0.07, loop = false }
}

local lastAnimKey = nil
local lastFlipX = false

local function ApplySheet(animKey, flipX)
    if not SetSpriteAnimationSheet then return end
    if animKey == lastAnimKey and flipX == lastFlipX then return end
    local a = ENEMY_ANIM[animKey]
    if not a then return end
    SetSpriteAnimationSheet(entityID, a.tex, a.rows, a.cols, a.frames, a.time, a.loop)
    if SetAnimationFlipX then SetAnimationFlipX(entityID, flipX and true or false) end
    lastAnimKey = animKey
    lastFlipX = flipX and true or false
end

local function SetFacingFromDelta(dx, dy)
    if math.abs(dx) > math.abs(dy) then
        ApplySheet("idleSide", dx < 0)
        facingDirX = dx > 0 and 1 or -1
        facingDirY = 0
    else
        if dy > 0 then
            ApplySheet("idleBack", false)
            facingDirX = 0
            facingDirY = 1
        else
            ApplySheet("idleFront", false)
            facingDirX = 0
            facingDirY = -1
        end
    end
end

local function SetAttackFacing(ex, ey, px, py)
    local dx, dy = px - ex, py - ey
    if math.abs(dx) > math.abs(dy) then
        ApplySheet("atkLeft", dx > 0)
        facingDirX = dx > 0 and 1 or -1
        facingDirY = 0
    else
        if dy > 0 then
            ApplySheet("atkBack", false)
            facingDirX = 0
            facingDirY = 1
        else
            ApplySheet("atkFront", false)
            facingDirX = 0
            facingDirY = -1
        end
    end
end

local function GetAttackAnimDuration()
    local a = ENEMY_ANIM[lastAnimKey]
    if a then return (a.frames * a.time) + 0.02 end
    return 0.20
end

-- ============================================================================
-- HELPERS
-- ============================================================================

local function CalculateDistance(x1, y1, x2, y2)
    return math.abs(x2 - x1) + math.abs(y2 - y1)
end

local function FindClosestPlayer()
    local ex, ey = GetEntityGridPosition(entityID)
    if not ex then return nil end
    local players = GetAllPlayers()
    if not players or #players == 0 then return nil end
    local closestID, closestDist = nil, 999999
    for _, pid in ipairs(players) do
        local hp = GetEntityHP(pid)
        if hp and hp > 0 then
            local px, py = GetEntityGridPosition(pid)
            if px then
                local dist = CalculateDistance(ex, ey, px, py)
                if dist < closestDist then
                    closestDist = dist
                    closestID = pid
                end
            end
        end
    end
    return closestID, closestDist
end

-- Find a random living enemy ally (not self) for Barrier
local function FindRandomAlly()
    local enemies = GetAllEnemies()
    if not enemies then return nil end
    local allies = {}
    for _, eid in ipairs(enemies) do
        if eid ~= entityID then
            local hp = GetEntityHP(eid)
            if hp and hp > 0 then
                table.insert(allies, eid)
            end
        end
    end
    if #allies == 0 then return nil end
    return allies[math.random(#allies)]
end

-- Check if target player is in line of sight (same row or column, no walls between)
local function IsInLineOfSight(ex, ey, px, py)
    if ex ~= px and ey ~= py then
        return false  -- Not on same row or column
    end

    -- Walk each tile between mage and player checking for walls
    if ex == px then
        -- Same column: walk vertically
        local step = (py > ey) and 1 or -1
        for y = ey + step, py - step, step do
            if IsTileWall and IsTileWall(ex, y) then
                return false  -- Wall blocking LOS
            end
        end
    else
        -- Same row: walk horizontally
        local step = (px > ex) and 1 or -1
        for x = ex + step, px - step, step do
            if IsTileWall and IsTileWall(x, ey) then
                return false  -- Wall blocking LOS
            end
        end
    end

    return true
end

-- ============================================================================
-- HEALTH BAR
-- ============================================================================

local function UpdateHealthBar()
    if not healthBarBG or not healthBarFG then return end
    if healthBarBG <= 0 or healthBarFG <= 0 then return end
    local wx, wy = GetEntityWorldPosition(entityID)
    if not wx or not wy then return end
    local barY = wy + healthBarOffsetY
    SetSpritePosition(healthBarBG, wx, barY)
    local currentHP, maxHP = GetEntityHP(entityID)
    if not currentHP or not maxHP or maxHP <= 0 then return end
    local ratio = math.max(0, math.min(1, currentHP / maxHP))
    local fgWidth = healthBarWidth * ratio
    local fgX = wx - (healthBarWidth - fgWidth) * 0.5
    SetSpritePosition(healthBarFG, fgX, barY)
    SetScale(healthBarFG, fgWidth, healthBarHeight)
    -- Blue color for mage
    SetSpriteColor(healthBarFG, 0.2, 0.4, 1.0, 1.0)
end

-- ============================================================================
-- LIFECYCLE
-- ============================================================================

function OnInit()
    entityID = self
    if not entityID or entityID == 0 then return end
    print("[EnemyMage " .. entityID .. "] Initialized")

    SetEntityHP(entityID, config.maxHP, config.maxHP)
    if SetEntityMaxAP then
        SetEntityMaxAP(entityID, config.maxAP)
    end

    _G.EnemiesDeadThisLevel = _G.EnemiesDeadThisLevel or 0

    ApplySheet("idleFront", false)

    -- Tint blue to distinguish as mage
    if SetSpriteColor then
        SetSpriteColor(entityID, 0.4, 0.6, 1.0, 1.0)
    end

    local ex, ey = GetEntityGridPosition(entityID)
    if ex and ey and SetTileOccupant then
        SetTileOccupant(ex, ey, entityID)
    end

    targetPlayerID = FindClosestPlayer() or 0

    -- Spawn health bar
    local wx, wy = GetEntityWorldPosition(entityID)
    if wx and wy then
        local barY = wy + healthBarOffsetY
        healthBarBG = SpawnSprite("", wx, barY, healthBarWidth, healthBarHeight, healthBarLayer)
        if healthBarBG and healthBarBG > 0 then
            SetSpriteColor(healthBarBG, 0.15, 0.15, 0.15, 0.85)
        end
        healthBarFG = SpawnSprite("", wx, barY, healthBarWidth, healthBarHeight, healthBarLayer + 1)
        if healthBarFG and healthBarFG > 0 then
            SetSpriteColor(healthBarFG, 0.2, 0.4, 1.0, 1.0)
        end
    end
end

function OnDestroy()
    _G.EnemiesDeadThisLevel = (_G.EnemiesDeadThisLevel or 0) + 1
    local ex, ey = GetEntityGridPosition(entityID)
    if ex and ey and SetTileOccupant then SetTileOccupant(ex, ey, 0) end
    if healthBarBG and healthBarBG > 0 then DestroyEntity(healthBarBG) end
    if healthBarFG and healthBarFG > 0 then DestroyEntity(healthBarFG) end
    print("[EnemyMage " .. entityID .. "] Destroyed")
end

-- ============================================================================
-- MAIN UPDATE
-- ============================================================================

function OnUpdate(dt)
    UpdateHealthBar()

    local currentTurn = GetCurrentTurn()
    if currentTurn ~= "Enemy" then
        if lastEnemyTurn == "Enemy" then
            hasActedThisTurn = false
            isMyTurnToAct = false
            moveTimer = 0.0
            mpRemaining = 0
            activeTimer = 0.0
            if glideActive then
                SetSpritePosition(entityID, glideEndX, glideEndY)
                glideActive = false
            end
        end
        lastEnemyTurn = currentTurn
        return
    end
    lastEnemyTurn = "Enemy"

    -- Handle glide
    if glideActive then
        glideElapsed = glideElapsed + dt
        local t = math.min(glideElapsed / glideDuration, 1.0)
        local eased = 1.0 - (1.0 - t) * (1.0 - t)
        local x = glideStartX + (glideEndX - glideStartX) * eased
        local y = glideStartY + (glideEndY - glideStartY) * eased
        SetSpritePosition(entityID, x, y)
        if t >= 1.0 then
            SetSpritePosition(entityID, glideEndX, glideEndY)
            glideActive = false
            moveTimer = moveDelay
        end
        if glideActive then return end
    end

    -- Sequential turn check
    local ok1, isActive = pcall(IsActiveEnemy, entityID)
    if not ok1 or not isActive then return end
    local ok2, actionReady = pcall(IsEnemyActionReady)
    if not ok2 or not actionReady then return end
    if hasActedThisTurn then return end

    -- Stun check
    if HasStatusEffect and HasStatusEffect(entityID, "stun") then
        print("[EnemyMage " .. entityID .. "] STUNNED")
        DecrementStatusEffects(entityID)
        FinishAction()
        return
    end

    if DecrementStatusEffects then DecrementStatusEffects(entityID) end

    if moveTimer > 0 then
        moveTimer = moveTimer - dt
        if moveTimer < 0 then moveTimer = 0 end
        return
    end

    activeTimer = activeTimer + dt
    if activeTimer >= maxActiveTime then
        FinishAction()
        return
    end

    -- Handle pending projectile animation
    if pendingProjectile then
        pendingProjectileTimer = pendingProjectileTimer - dt
        if pendingProjectileTimer > 0 then return end
        pendingProjectile = false

        -- Fire the projectile (offset spawn forward to clear own collider)
        local wx, wy = GetEntityWorldPosition(entityID)
        if wx and wy and SpawnSkillProjectile then
            -- Offset spawn position well past the mage's collider
            local spawnOffsetX = facingDirX * 0.05
            local spawnOffsetY = facingDirY * 0.05
            local projID = SpawnSkillProjectile(
                wx + spawnOffsetX, wy + spawnOffsetY,
                facingDirX, facingDirY,
                config.arcaneBoltSpeed,
                config.arcaneBoltDamage,
                false,           -- no pierce
                0.4, 0.2, 1.0, 1.0,  -- purple tint
                "",              -- default sprite
                true,            -- isEnemyProjectile: damages players, not enemies
                entityID         -- sourceEntityID: prevent self-hit
            )
            print("[EnemyMage " .. entityID .. "] Arcane Bolt fired! Projectile=" .. tostring(projID))
        end

        -- Check if we have enough AP for another arcane bolt
        local remainingAP = GetEntityAP(entityID)
        if remainingAP >= config.arcaneBoltAPCost then
            -- Try to fire again - go back to attack phase
            print("[EnemyMage " .. entityID .. "] AP remaining=" .. remainingAP .. ", checking for another shot")
            currentTurnPhase = PHASE.ATTACK
            moveTimer = 0.3  -- Brief delay between shots for visual clarity
        else
            -- Return to idle facing and end turn
            local ex, ey = GetEntityGridPosition(entityID)
            if ex and targetPlayerID > 0 then
                local px, py = GetEntityGridPosition(targetPlayerID)
                if px then SetFacingFromDelta(px - ex, py - ey) end
            end
            FinishAction()
        end
        return
    end

    -- First frame
    if not isMyTurnToAct then
        isMyTurnToAct = true
        activeTimer = 0.0
        currentTurnPhase = PHASE.INIT

        local bonusMP = (_G.RallyingCryActive) and 1 or 0
        mpRemaining = config.movementPoints + bonusMP

        if barrierCooldown > 0 then barrierCooldown = barrierCooldown - 1 end

        print("[EnemyMage " .. entityID .. "] === STARTING TURN === AP=" .. select(1, GetEntityAP(entityID)) .. " MP=" .. mpRemaining .. " BarrierCD=" .. barrierCooldown)

        targetPlayerID = FindClosestPlayer()
        if not targetPlayerID then
            FinishAction()
            return
        end
    end

    local ok, err = pcall(ProcessTurnPhase)
    if not ok then
        print("[EnemyMage " .. entityID .. "] ERROR: " .. tostring(err))
        FinishAction()
    end
end

-- ============================================================================
-- TURN PHASES
-- ============================================================================

function ProcessTurnPhase()
    if currentTurnPhase == PHASE.INIT then
        PhaseInit()
    elseif currentTurnPhase == PHASE.BARRIER then
        PhaseBarrier()
    elseif currentTurnPhase == PHASE.MOVE then
        PhaseMove()
    elseif currentTurnPhase == PHASE.ATTACK then
        PhaseAttack()
    else
        FinishAction()
    end
end

function PhaseInit()
    local currentAP = GetEntityAP(entityID)

    -- Priority: Barrier if off cooldown and we have AP
    if barrierCooldown <= 0 and currentAP >= config.barrierAPCost then
        currentTurnPhase = PHASE.BARRIER
        return
    end

    -- If player is already in line of sight at good range, skip movement and attack
    if currentAP >= config.arcaneBoltAPCost and targetPlayerID then
        local ex, ey = GetEntityGridPosition(entityID)
        local px, py = GetEntityGridPosition(targetPlayerID)
        if ex and px then
            local dist = CalculateDistance(ex, ey, px, py)
            if IsInLineOfSight(ex, ey, px, py) and dist >= 2 and dist <= config.arcaneBoltRange then
                print("[EnemyMage " .. entityID .. "] Player in LOS at range " .. dist .. ", skipping move to attack")
                currentTurnPhase = PHASE.ATTACK
                return
            end
        end
    end

    currentTurnPhase = PHASE.MOVE
end

function PhaseBarrier()
    local currentAP = GetEntityAP(entityID)
    if currentAP < config.barrierAPCost then
        currentTurnPhase = PHASE.MOVE
        return
    end

    -- Find target: random ally if possible, otherwise self
    local barrierTarget = FindRandomAlly()
    if not barrierTarget then
        barrierTarget = entityID  -- No allies, target self
    end

    -- Check if target already has guard
    if HasStatusEffect and HasStatusEffect(barrierTarget, "guard") then
        -- Already guarded, skip
        currentTurnPhase = PHASE.MOVE
        return
    end

    ConsumeEnemyAP(entityID, config.barrierAPCost)
    barrierCooldown = config.barrierCooldownMax

    -- Apply guard status effect (blocks next damage, permanent until consumed)
    if ApplyStatusEffect then
        ApplyStatusEffect(barrierTarget, "guard", -1, entityID)
    end

    print("[EnemyMage " .. entityID .. "] BARRIER on Entity " .. barrierTarget)

    -- Visual: pulse barrier target
    local tx, ty = GetEntityGridPosition(barrierTarget)
    if tx then PulseTile(tx, ty, 0.5, 0.2, 0.4, 1.0) end
    local ex, ey = GetEntityGridPosition(entityID)
    if ex then PulseTile(ex, ey, 0.3, 0.2, 0.4, 1.0) end

    currentTurnPhase = PHASE.MOVE
end

function PhaseMove()
    if mpRemaining <= 0 then
        currentTurnPhase = PHASE.ATTACK
        return
    end

    local ex, ey = GetEntityGridPosition(entityID)
    local px, py = GetEntityGridPosition(targetPlayerID)
    if not ex or not px then
        currentTurnPhase = PHASE.ATTACK
        return
    end

    local dist = CalculateDistance(ex, ey, px, py)

    -- Mage prefers to stay at range. If already in line of sight at good range, attack
    if IsInLineOfSight(ex, ey, px, py) and dist >= 2 and dist <= config.arcaneBoltRange then
        currentTurnPhase = PHASE.ATTACK
        return
    end

    -- If too close, try to back away
    if dist < 2 then
        local dx = ex - px
        local dy = ey - py
        local fleeX, fleeY = ex, ey
        if math.abs(dx) > math.abs(dy) then
            fleeX = ex + (dx > 0 and 1 or -1)
        else
            fleeY = ey + (dy > 0 and 1 or -1)
        end

        if IsWalkableTile(fleeX, fleeY) and not IsTileOccupied(fleeX, fleeY) then
            SetFacingFromDelta(fleeX - ex, fleeY - ey)
            local sx, sy = GetEntityWorldPosition(entityID)
            local success = MoveEntityToTile(entityID, fleeX, fleeY)
            if success then
                local endX, endY = GetEntityWorldPosition(entityID)
                if sx and sy and endX and endY then
                    glideActive = true
                    glideElapsed = 0.0
                    glideStartX, glideStartY = sx, sy
                    glideEndX, glideEndY = endX, endY
                    SetSpritePosition(entityID, sx, sy)
                end
                mpRemaining = mpRemaining - 1
                return
            end
        end
    end

    -- Move toward player to get in line of sight
    if #currentPath == 0 or pathIndex > #currentPath or pathTargetPlayerID ~= targetPlayerID
       or lastKnownPlayerX ~= px or lastKnownPlayerY ~= py then
        currentPath = FindPathToTarget(ex, ey, px, py)
        pathIndex = 1
        pathTargetPlayerID = targetPlayerID
        lastKnownPlayerX = px
        lastKnownPlayerY = py
        if not currentPath or #currentPath == 0 then
            currentTurnPhase = PHASE.ATTACK
            return
        end
    end

    if pathIndex <= #currentPath and mpRemaining > 0 then
        local nextTile = currentPath[pathIndex]
        if not IsWalkableTile(nextTile.x, nextTile.y) or IsTileOccupied(nextTile.x, nextTile.y) then
            currentPath = {}
            currentTurnPhase = PHASE.ATTACK
            return
        end

        SetFacingFromDelta(nextTile.x - ex, nextTile.y - ey)

        local sx, sy = GetEntityWorldPosition(entityID)
        local success = MoveEntityToTile(entityID, nextTile.x, nextTile.y)
        if success then
            local endX, endY = GetEntityWorldPosition(entityID)
            if sx and sy and endX and endY then
                glideActive = true
                glideElapsed = 0.0
                glideStartX, glideStartY = sx, sy
                glideEndX, glideEndY = endX, endY
                SetSpritePosition(entityID, sx, sy)
            end
            mpRemaining = mpRemaining - 1
            pathIndex = pathIndex + 1
            ShowTileBorder(nextTile.x, nextTile.y, 0.3)
            PulseTile(nextTile.x, nextTile.y, 0.2, 0.2, 0.4, 1.0)
            return
        else
            currentPath = {}
            currentTurnPhase = PHASE.ATTACK
            return
        end
    end

    currentTurnPhase = PHASE.ATTACK
end

function PhaseAttack()
    local currentAP = GetEntityAP(entityID)
    if currentAP < config.arcaneBoltAPCost then
        FinishAction()
        return
    end

    local ex, ey = GetEntityGridPosition(entityID)
    local px, py = GetEntityGridPosition(targetPlayerID)
    if not ex or not px then
        FinishAction()
        return
    end

    -- Only fire if player is in line of sight (same row or column)
    if not IsInLineOfSight(ex, ey, px, py) then
        FinishAction()
        return
    end

    -- Face the target player and fire projectile in that direction
    ConsumeEnemyAP(entityID, config.arcaneBoltAPCost)
    SetAttackFacing(ex, ey, px, py)

    pendingProjectile = true
    pendingProjectileTimer = GetAttackAnimDuration()
    print("[EnemyMage " .. entityID .. "] ARCANE BOLT toward Player " .. targetPlayerID .. " dir=(" .. facingDirX .. "," .. facingDirY .. ")")
end

-- ============================================================================
-- TURN COORDINATION
-- ============================================================================

function FinishAction()
    hasActedThisTurn = true
    mpRemaining = 0
    moveTimer = 0.0
    glideActive = false
    activeTimer = 0.0
    currentPath = {}
    pathIndex = 1
    pendingProjectile = false
    currentTurnPhase = PHASE.DONE
    print("[EnemyMage " .. entityID .. "] Finished turn")
    MarkEnemyActionComplete()
end
