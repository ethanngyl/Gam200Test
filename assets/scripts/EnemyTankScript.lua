--[[
===============================================================================
 File:          EnemyTankScript.lua
 Date:          03/03/2026
 ------------------------------------------------------------------------------

 ENEMY TANK - Heavily Armored Defender

 Stats: HP 7, AP 3, MP 2

 Skills:
    1. Shield Bash  - Adjacent tile, stuns target player, 2 damage, 3 AP
    2. Taunt        - While alive, damage taken by allies is redirected to
                      this unit. Used if any enemy has < 50% health, 2 AP.
                      Always prioritized if condition is true.
                      Lasts indefinitely.

 Passive:
    Heavy Armor     - Takes 1 reduced damage from all sources, always active
                      (Implemented via "damageReduction" status effect)

 AI Priority:
    Taunt (if ally < 50% HP) > Move toward player > Shield Bash

 Copyright (C) 2026 DigiPen Institute of Technology.
===============================================================================
]]--

-- ============================================================================
-- CONFIGURATION
-- ============================================================================

local PHASE = {
    INIT   = "init",
    TAUNT  = "taunt",
    MOVE   = "move",
    ATTACK = "attack",
    DONE   = "done"
}

local entityID = 0
local targetPlayerID = 0

local config = {
    -- Stats
    maxHP = 7,
    maxAP = 3,
    movementPoints = 2,

    -- Shield Bash
    shieldBashRange = 1,
    shieldBashDamage = 2,
    shieldBashAPCost = 3,

    -- Taunt
    tauntAPCost = 2,

    -- Pathfinding
    maxPathLength = 10,
    aggroRange = 99
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

-- Pending attack
local pendingAttack = false
local pendingAttackTimer = 0.0
local pendingAttackTarget = 0
local pendingAttackDamage = 0

-- Taunt state
local tauntActive = false      -- Is Taunt currently active?
local tauntedAllies = {}       -- Track which allies have knightsOath from us

-- Health bar
local healthBarBG = nil
local healthBarFG = nil
local healthBarWidth = 0.12    -- Wider for tank
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

-- Check if any enemy ally has < 50% health
local function AnyAllyLowHealth()
    local enemies = GetAllEnemies()
    if not enemies then return false end
    for _, eid in ipairs(enemies) do
        if eid ~= entityID then
            local hp, maxHP = GetEntityHP(eid)
            if hp and maxHP and maxHP > 0 and hp > 0 then
                if (hp / maxHP) < 0.5 then
                    return true
                end
            end
        end
    end
    return false
end

-- Apply knightsOath to all living enemy allies (damage redirects to Tank)
local function ApplyTauntToAllAllies()
    local enemies = GetAllEnemies()
    if not enemies then return end
    tauntedAllies = {}
    for _, eid in ipairs(enemies) do
        if eid ~= entityID then
            local hp = GetEntityHP(eid)
            if hp and hp > 0 then
                if ApplyStatusEffect then
                    -- knightsOath: sourceEntity=Tank, so damage redirects TO Tank
                    ApplyStatusEffect(eid, "knightsOath", -1, entityID)
                    table.insert(tauntedAllies, eid)
                    print("[EnemyTank " .. entityID .. "] Taunt applied to ally " .. eid)
                end
            end
        end
    end
end

-- Refresh taunt on any new allies that don't have it
local function RefreshTaunt()
    if not tauntActive then return end
    local enemies = GetAllEnemies()
    if not enemies then return end
    for _, eid in ipairs(enemies) do
        if eid ~= entityID then
            local hp = GetEntityHP(eid)
            if hp and hp > 0 then
                if HasStatusEffect and not HasStatusEffect(eid, "knightsOath") then
                    if ApplyStatusEffect then
                        ApplyStatusEffect(eid, "knightsOath", -1, entityID)
                        print("[EnemyTank " .. entityID .. "] Taunt refreshed on ally " .. eid)
                    end
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
    -- Yellow/gold for tank
    SetSpriteColor(healthBarFG, 0.9, 0.75, 0.1, 1.0)
end

-- ============================================================================
-- LIFECYCLE
-- ============================================================================

function OnInit()
    entityID = self
    if not entityID or entityID == 0 then return end
    print("[EnemyTank " .. entityID .. "] Initialized")

    SetEntityHP(entityID, config.maxHP, config.maxHP)
    if SetEntityMaxAP then
        SetEntityMaxAP(entityID, config.maxAP)
    end

    _G.EnemiesDeadThisLevel = _G.EnemiesDeadThisLevel or 0

    -- Apply Heavy Armor passive: permanent damage reduction of 1
    if ApplyStatusEffect then
        ApplyStatusEffect(entityID, "damageReduction", -1, entityID, 0, 1)
        print("[EnemyTank " .. entityID .. "] Heavy Armor applied (1 damage reduction)")
    end

    ApplySheet("idleFront", false)

    -- Tint green to distinguish as tank
    if SetSpriteColor then
        SetSpriteColor(entityID, 0.3, 0.9, 0.3, 1.0)
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
            SetSpriteColor(healthBarFG, 0.9, 0.75, 0.1, 1.0)
        end
    end
end

function OnDestroy()
    -- Remove taunt from all allies when Tank dies
    if tauntActive and RemoveStatusEffect then
        local enemies = GetAllEnemies()
        if enemies then
            for _, eid in ipairs(enemies) do
                if eid ~= entityID then
                    -- Only remove if the knightsOath was from this Tank
                    if HasStatusEffect and HasStatusEffect(eid, "knightsOath") then
                        if GetStatusEffectSource and GetStatusEffectSource(eid, "knightsOath") == entityID then
                            RemoveStatusEffect(eid, "knightsOath")
                            print("[EnemyTank " .. entityID .. "] Taunt removed from ally " .. eid .. " (Tank died)")
                        end
                    end
                end
            end
        end
    end

    _G.EnemiesDeadThisLevel = (_G.EnemiesDeadThisLevel or 0) + 1
    local ex, ey = GetEntityGridPosition(entityID)
    if ex and ey and SetTileOccupant then SetTileOccupant(ex, ey, 0) end
    if healthBarBG and healthBarBG > 0 then DestroyEntity(healthBarBG) end
    if healthBarFG and healthBarFG > 0 then DestroyEntity(healthBarFG) end
    print("[EnemyTank " .. entityID .. "] Destroyed")
end

-- ============================================================================
-- MAIN UPDATE
-- ============================================================================

function OnUpdate(dt)
    UpdateHealthBar()

    -- Refresh taunt on new allies each frame (catches newly summoned enemies)
    RefreshTaunt()

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
        print("[EnemyTank " .. entityID .. "] STUNNED")
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

    -- Handle pending attack
    if pendingAttack then
        pendingAttackTimer = pendingAttackTimer - dt
        if pendingAttackTimer > 0 then return end
        pendingAttack = false

        -- Apply damage with Bolstered Morale bonus
        local bonusDmg = 0
        if _G.KnightCommanderIDs then
            for _, cid in ipairs(_G.KnightCommanderIDs) do
                local chp = GetEntityHP(cid)
                if chp and chp > 0 then
                    bonusDmg = bonusDmg + 1
                    break
                end
            end
        end
        local totalDmg = pendingAttackDamage + bonusDmg
        DamageEntity(pendingAttackTarget, totalDmg, entityID)
        print("[EnemyTank " .. entityID .. "] Shield Bash hit for " .. totalDmg)

        -- Apply stun to target
        if ApplyStatusEffect then
            ApplyStatusEffect(pendingAttackTarget, "stun", 1, entityID)
            print("[EnemyTank " .. entityID .. "] STUNNED Player " .. pendingAttackTarget)
        end

        local px, py = GetEntityGridPosition(pendingAttackTarget)
        if px then PulseTile(px, py, 0.4, 1.0, 0.8, 0.0) end

        local ex, ey = GetEntityGridPosition(entityID)
        if ex and targetPlayerID > 0 then
            local px2, py2 = GetEntityGridPosition(targetPlayerID)
            if px2 then SetFacingFromDelta(px2 - ex, py2 - ey) end
        end

        FinishAction()
        return
    end

    -- First frame
    if not isMyTurnToAct then
        isMyTurnToAct = true
        activeTimer = 0.0
        currentTurnPhase = PHASE.INIT

        local bonusMP = (_G.RallyingCryActive) and 1 or 0
        mpRemaining = config.movementPoints + bonusMP

        print("[EnemyTank " .. entityID .. "] === STARTING TURN === AP=" .. select(1, GetEntityAP(entityID)) .. " MP=" .. mpRemaining .. " TauntActive=" .. tostring(tauntActive))

        targetPlayerID = FindClosestPlayer()
        if not targetPlayerID then
            FinishAction()
            return
        end
    end

    local ok, err = pcall(ProcessTurnPhase)
    if not ok then
        print("[EnemyTank " .. entityID .. "] ERROR: " .. tostring(err))
        FinishAction()
    end
end

-- ============================================================================
-- TURN PHASES
-- ============================================================================

function ProcessTurnPhase()
    if currentTurnPhase == PHASE.INIT then
        PhaseInit()
    elseif currentTurnPhase == PHASE.TAUNT then
        PhaseTaunt()
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

    -- Priority: Taunt if any ally has < 50% HP and Taunt not already active
    if not tauntActive and AnyAllyLowHealth() and currentAP >= config.tauntAPCost then
        currentTurnPhase = PHASE.TAUNT
        return
    end

    currentTurnPhase = PHASE.MOVE
end

function PhaseTaunt()
    local currentAP = GetEntityAP(entityID)
    if currentAP < config.tauntAPCost then
        currentTurnPhase = PHASE.MOVE
        return
    end

    ConsumeEnemyAP(entityID, config.tauntAPCost)
    tauntActive = true
    ApplyTauntToAllAllies()

    print("[EnemyTank " .. entityID .. "] TAUNT ACTIVATED! Redirecting ally damage to self.")

    -- Visual: pulse all allied enemy tiles
    local enemies = GetAllEnemies()
    if enemies then
        for _, eid in ipairs(enemies) do
            local ex, ey = GetEntityGridPosition(eid)
            if ex then PulseTile(ex, ey, 0.5, 0.9, 0.75, 0.1) end
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

    if CalculateDistance(ex, ey, px, py) <= config.shieldBashRange then
        currentTurnPhase = PHASE.ATTACK
        return
    end

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
            PulseTile(nextTile.x, nextTile.y, 0.2, 0.9, 0.75, 0.1)
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
    if currentAP < config.shieldBashAPCost then
        FinishAction()
        return
    end

    local ex, ey = GetEntityGridPosition(entityID)
    local px, py = GetEntityGridPosition(targetPlayerID)
    if not ex or not px then
        FinishAction()
        return
    end

    if CalculateDistance(ex, ey, px, py) > config.shieldBashRange then
        FinishAction()
        return
    end

    -- Shield Bash
    ConsumeEnemyAP(entityID, config.shieldBashAPCost)
    SetAttackFacing(ex, ey, px, py)
    pendingAttack = true
    pendingAttackTimer = GetAttackAnimDuration()
    pendingAttackTarget = targetPlayerID
    pendingAttackDamage = config.shieldBashDamage
    print("[EnemyTank " .. entityID .. "] SHIELD BASH on Player " .. targetPlayerID)
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
    print("[EnemyTank " .. entityID .. "] Finished turn")
    MarkEnemyActionComplete()
end
