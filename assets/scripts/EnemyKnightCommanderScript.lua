--[[
===============================================================================
 File:          EnemyKnightCommanderScript.lua
 Date:          03/03/2026
 ------------------------------------------------------------------------------

 ENEMY KNIGHT COMMANDER - Tactical Support Enemy

 Stats: HP 3, AP 3, MP 3

 Skills:
    1. Strike         - Adjacent tiles, 1 damage, 1 AP
    2. Rallying Cry   - All enemies gain +1 MP next turn, 2 AP,
                        3-turn cooldown, prioritized if not active

 Passive:
    Bolstered Morale  - All enemies gain +1 damage while this unit is alive

 AI Priority:
    Rallying Cry (if off cooldown) > Move toward player > Strike

 Copyright (C) 2026 DigiPen Institute of Technology.
===============================================================================
]]--

-- ============================================================================
-- CONFIGURATION
-- ============================================================================

local PHASE = {
    INIT  = "init",
    SKILL = "skill",
    MOVE  = "move",
    ATTACK = "attack",
    DONE  = "done"
}

local entityID = 0
local targetPlayerID = 0

local config = {
    -- Stats
    maxHP = 3,
    maxAP = 3,
    movementPoints = 3,       -- MP: tiles per turn (does NOT cost AP)

    -- Strike
    strikeRange = 1,
    strikeDamage = 1,
    strikeAPCost = 1,

    -- Rallying Cry
    rallyCryAPCost = 2,
    rallyCryCooldownMax = 3,  -- Can only be used every 3 turns

    -- Pathfinding
    maxPathLength = 10,
    aggroRange = 99           -- Always chases
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

-- Pending attack animation
local pendingAttack = false
local pendingAttackTimer = 0.0
local pendingAttackTarget = 0
local pendingAttackDamage = 0

-- Rallying Cry
local rallyCryCooldown = 0   -- 0 = ready

-- Health bar
local healthBarBG = nil
local healthBarFG = nil
local healthBarWidth = 0.1
local healthBarHeight = 0.02
local healthBarOffsetY = 0.05
local healthBarLayer = 2

-- Turn phase tracking for Rallying Cry lifecycle
local lastTurnPhase = nil

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
    else
        if dy > 0 then ApplySheet("idleBack", false)
        else ApplySheet("idleFront", false) end
    end
end

local function SetAttackFacing(ex, ey, px, py)
    local dx, dy = px - ex, py - ey
    if math.abs(dx) > math.abs(dy) then
        ApplySheet("atkLeft", dx > 0)
    else
        if dy > 0 then ApplySheet("atkBack", false)
        else ApplySheet("atkFront", false) end
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

-- Apply Bolstered Morale passive: +1 damageModifier to all enemies
local function ApplyBolsteredMorale()
    local enemies = GetAllEnemies()
    if not enemies then return end
    _G.BolsteredMoraleParticles = _G.BolsteredMoraleParticles or {}
    for _, eid in ipairs(enemies) do
        if eid ~= entityID then
            local current = GetDamageModifier(eid) or 0
            SetDamageModifier(eid, current + 1)
            print("[KnightCommander " .. entityID .. "] Bolstered Morale: +1 damageModifier on enemy " .. eid)

            -- Spawn yellow particle emitter around the buffed enemy
            if SpawnParticleEmitter then
                local wx, wy = GetEntityWorldPosition(eid)
                if wx and wy then
                    local emitterID = SpawnParticleEmitter(wx, wy, 0.04, 8, 0, 1.0, 0.9, 0.0, 1.0, eid)
                    if emitterID and emitterID > 0 then
                        local key = entityID .. "_" .. eid
                        _G.BolsteredMoraleParticles[key] = emitterID
                        print("[KnightCommander " .. entityID .. "] Spawned morale particles (entity " .. emitterID .. ") on enemy " .. eid)
                    end
                end
            end
        end
    end
end

-- Remove Bolstered Morale passive: -1 damageModifier from all enemies
local function RemoveBolsteredMorale()
    local enemies = GetAllEnemies()
    if not enemies then return end
    for _, eid in ipairs(enemies) do
        if eid ~= entityID then
            local current = GetDamageModifier(eid) or 0
            local newVal = current - 1
            if newVal < 0 then newVal = 0 end
            SetDamageModifier(eid, newVal)

            -- Remove particle emitter for this enemy
            if _G.BolsteredMoraleParticles and DestroyEntity then
                local key = entityID .. "_" .. eid
                local emitterID = _G.BolsteredMoraleParticles[key]
                if emitterID and emitterID > 0 then
                    DestroyEntity(emitterID)
                    _G.BolsteredMoraleParticles[key] = nil
                    print("[KnightCommander " .. entityID .. "] Removed morale particles from enemy " .. eid)
                end
            end
        end
    end
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
    -- Purple-ish color for commander
    SetSpriteColor(healthBarFG, 0.6, 0.2, 0.8, 1.0)
end

-- ============================================================================
-- LIFECYCLE
-- ============================================================================

function OnInit()
    entityID = self
    if not entityID or entityID == 0 then return end
    print("[KnightCommander " .. entityID .. "] Initialized")

    -- Set stats
    SetEntityHP(entityID, config.maxHP, config.maxHP)
    if SetEntityMaxAP then
        SetEntityMaxAP(entityID, config.maxAP)
    end

    -- Register as Knight Commander globally for Bolstered Morale passive
    _G.KnightCommanderIDs = _G.KnightCommanderIDs or {}
    table.insert(_G.KnightCommanderIDs, entityID)

    -- Initialize death counter if not exists
    _G.EnemiesDeadThisLevel = _G.EnemiesDeadThisLevel or 0

    ApplySheet("idleFront", false)

    -- Tint yellow to distinguish from regular knights
    if SetSpriteColor then
        SetSpriteColor(entityID, 1.0, 0.9, 0.3, 1.0)
    end

    -- Set tile occupancy
    local ex, ey = GetEntityGridPosition(entityID)
    if ex and ey and SetTileOccupant then
        SetTileOccupant(ex, ey, entityID)
    end

    targetPlayerID = FindClosestPlayer() or 0

    -- Apply Bolstered Morale passive to all existing enemies
    ApplyBolsteredMorale()

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
            SetSpriteColor(healthBarFG, 0.6, 0.2, 0.8, 1.0)
        end
    end
end

function OnDestroy()
    -- Remove Bolstered Morale from all enemies
    RemoveBolsteredMorale()

    -- Remove from commander list
    if _G.KnightCommanderIDs then
        for i, id in ipairs(_G.KnightCommanderIDs) do
            if id == entityID then
                table.remove(_G.KnightCommanderIDs, i)
                break
            end
        end
    end

    -- Increment death counter
    _G.EnemiesDeadThisLevel = (_G.EnemiesDeadThisLevel or 0) + 1

    -- Cleanup
    local ex, ey = GetEntityGridPosition(entityID)
    if ex and ey and SetTileOccupant then SetTileOccupant(ex, ey, 0) end
    if healthBarBG and healthBarBG > 0 then DestroyEntity(healthBarBG) end
    if healthBarFG and healthBarFG > 0 then DestroyEntity(healthBarFG) end
    print("[KnightCommander " .. entityID .. "] Destroyed")
end

-- ============================================================================
-- RALLYING CRY LIFECYCLE MANAGEMENT
-- ============================================================================

local function ManageRallyingCryLifecycle(currentTurn)
    -- Detect enemy phase transitions
    if currentTurn == "Enemy" and lastTurnPhase ~= "Enemy" then
        -- Enemy phase just started
        if _G.RallyingCryPending then
            _G.RallyingCryActive = true
            _G.RallyingCryPending = false
            print("[KnightCommander " .. entityID .. "] Rallying Cry ACTIVATED for this turn!")
        end
    elseif currentTurn ~= "Enemy" and lastTurnPhase == "Enemy" then
        -- Enemy phase just ended
        if _G.RallyingCryActive then
            _G.RallyingCryActive = false
            print("[KnightCommander " .. entityID .. "] Rallying Cry expired")
        end
    end
    lastTurnPhase = currentTurn
end

-- ============================================================================
-- MAIN UPDATE
-- ============================================================================

function OnUpdate(dt)
    UpdateHealthBar()

    -- Update Bolstered Morale particle positions to follow this enemy
    if _G.BolsteredMoraleParticles and SetSpritePosition then
        local wx, wy = GetEntityWorldPosition(entityID)
        if wx and wy then
            for key, emitterID in pairs(_G.BolsteredMoraleParticles) do
                local targetID = key:match("_(%d+)$")
                if targetID and tonumber(targetID) == entityID then
                    SetSpritePosition(emitterID, wx, wy)
                end
            end
        end
    end

    local currentTurn = GetCurrentTurn()

    -- Manage Rallying Cry lifecycle every frame
    ManageRallyingCryLifecycle(currentTurn)

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

    -- Stun check (status effects are decremented centrally in ResetPartyTurn)
    if HasStatusEffect and HasStatusEffect(entityID, "stun") then
        print("[KnightCommander " .. entityID .. "] STUNNED")
        FinishAction()
        return
    end

    -- Move timer
    if moveTimer > 0 then
        moveTimer = moveTimer - dt
        if moveTimer < 0 then moveTimer = 0 end
        return
    end

    -- Safety timeout
    activeTimer = activeTimer + dt
    if activeTimer >= maxActiveTime then
        print("[KnightCommander " .. entityID .. "] SAFETY: force-finishing turn")
        FinishAction()
        return
    end

    -- Handle pending attack animation
    if pendingAttack then
        pendingAttackTimer = pendingAttackTimer - dt
        if pendingAttackTimer > 0 then return end
        pendingAttack = false

        DamageEntity(pendingAttackTarget, pendingAttackDamage, entityID)
        print("[KnightCommander " .. entityID .. "] Strike hit for " .. pendingAttackDamage .. " damage")

        local px, py = GetEntityGridPosition(pendingAttackTarget)
        if px then PulseTile(px, py, 0.3, 1.0, 0.0, 0.0) end

        -- Return to idle
        local ex, ey = GetEntityGridPosition(entityID)
        if ex and targetPlayerID > 0 then
            local px2, py2 = GetEntityGridPosition(targetPlayerID)
            if px2 then SetFacingFromDelta(px2 - ex, py2 - ey) end
        end

        -- Check if we have AP remaining to attack again
        local remainingAP = GetEntityAP(entityID)
        if remainingAP >= config.strikeAPCost then
            print("[KnightCommander " .. entityID .. "] AP remaining=" .. remainingAP .. ", trying another attack")
            currentTurnPhase = PHASE.ATTACK
            moveTimer = 0.3  -- Brief delay between attacks for visual clarity
        else
            FinishAction()
        end
        return
    end

    -- First frame of turn
    if not isMyTurnToAct then
        isMyTurnToAct = true
        activeTimer = 0.0
        currentTurnPhase = PHASE.INIT

        -- Calculate MP with Rallying Cry bonus
        local bonusMP = (_G.RallyingCryActive) and 1 or 0
        mpRemaining = config.movementPoints + bonusMP

        -- Decrement cooldowns
        if rallyCryCooldown > 0 then
            rallyCryCooldown = rallyCryCooldown - 1
        end

        print("[KnightCommander " .. entityID .. "] === STARTING TURN === AP=" .. select(1, GetEntityAP(entityID)) .. " MP=" .. mpRemaining .. " RallyCryCooldown=" .. rallyCryCooldown)

        targetPlayerID = FindClosestPlayer()
        if not targetPlayerID then
            FinishAction()
            return
        end
    end

    -- Process turn phase
    local ok, err = pcall(ProcessTurnPhase)
    if not ok then
        print("[KnightCommander " .. entityID .. "] ERROR: " .. tostring(err))
        FinishAction()
    end
end

-- ============================================================================
-- TURN PHASE PROCESSING
-- ============================================================================

function ProcessTurnPhase()
    if currentTurnPhase == PHASE.INIT then
        PhaseInit()
    elseif currentTurnPhase == PHASE.SKILL then
        PhaseSkill()
    elseif currentTurnPhase == PHASE.MOVE then
        PhaseMove()
    elseif currentTurnPhase == PHASE.ATTACK then
        PhaseAttack()
    else
        FinishAction()
    end
end

function PhaseInit()
    -- Priority: Rallying Cry if available
    local currentAP = GetEntityAP(entityID)

    if rallyCryCooldown <= 0 and not _G.RallyingCryPending and not _G.RallyingCryActive then
        if currentAP >= config.rallyCryAPCost then
            currentTurnPhase = PHASE.SKILL
            return
        end
    end

    -- Otherwise move then attack
    currentTurnPhase = PHASE.MOVE
end

function PhaseSkill()
    -- Use Rallying Cry
    local currentAP = GetEntityAP(entityID)
    if currentAP >= config.rallyCryAPCost then
        ConsumeEnemyAP(entityID, config.rallyCryAPCost)
        _G.RallyingCryPending = true
        rallyCryCooldown = config.rallyCryCooldownMax
        print("[KnightCommander " .. entityID .. "] Used RALLYING CRY! All enemies get +1 MP next turn")

        -- Visual: pulse all enemy tiles
        local enemies = GetAllEnemies()
        if enemies then
            for _, eid in ipairs(enemies) do
                local ex, ey = GetEntityGridPosition(eid)
                if ex then PulseTile(ex, ey, 0.4, 0.6, 0.2, 0.8) end
            end
        end
    end

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

    -- Already adjacent
    if CalculateDistance(ex, ey, px, py) <= config.strikeRange then
        currentTurnPhase = PHASE.ATTACK
        return
    end

    -- Pathfind
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

    -- Move one tile
    if pathIndex <= #currentPath and mpRemaining > 0 then
        local nextTile = currentPath[pathIndex]
        if not IsWalkableTile(nextTile.x, nextTile.y) or IsTileOccupied(nextTile.x, nextTile.y) then
            currentPath = {}
            currentTurnPhase = PHASE.ATTACK
            return
        end

        local dx, dy = nextTile.x - ex, nextTile.y - ey
        SetFacingFromDelta(dx, dy)

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
            PulseTile(nextTile.x, nextTile.y, 0.2, 0.6, 0.2, 0.8)
            return -- Wait for glide
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
    if currentAP < config.strikeAPCost then
        FinishAction()
        return
    end

    local ex, ey = GetEntityGridPosition(entityID)
    local px, py = GetEntityGridPosition(targetPlayerID)
    if not ex or not px then
        FinishAction()
        return
    end

    if CalculateDistance(ex, ey, px, py) > config.strikeRange then
        FinishAction()
        return
    end

    -- Strike
    ConsumeEnemyAP(entityID, config.strikeAPCost)
    SetAttackFacing(ex, ey, px, py)
    pendingAttack = true
    pendingAttackTimer = GetAttackAnimDuration()
    pendingAttackTarget = targetPlayerID
    pendingAttackDamage = config.strikeDamage
    print("[KnightCommander " .. entityID .. "] STRIKE on Player " .. targetPlayerID)
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
    pendingAttack = false
    currentTurnPhase = PHASE.DONE
    print("[KnightCommander " .. entityID .. "] Finished turn")
    MarkEnemyActionComplete()
end
