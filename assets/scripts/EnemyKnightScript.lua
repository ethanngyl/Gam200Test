--[[
===============================================================================
 File:          EnemyKnightScript.lua
 Date:          03/03/2026
 ------------------------------------------------------------------------------

 ENEMY KNIGHT - Durable Frontline Enemy with Summoning

 Stats: HP 5, AP 2, MP 3

 Skills:
    1. Strike                - Adjacent tiles, 1 damage, 1 AP
    2. Summon Reinforcements - Skips next turn, then spawns another Enemy Knight
                               on a random open tile if still alive. 2 AP.
                               2-turn cooldown. Only used when 2+ enemies have
                               died this level.

 AI Priority:
    Summon (if conditions met) > Move toward player > Strike

 Copyright (C) 2026 DigiPen Institute of Technology.
===============================================================================
]]--

-- ============================================================================
-- CONFIGURATION
-- ============================================================================

local PHASE = {
    INIT   = "init",
    SKILL  = "skill",
    MOVE   = "move",
    ATTACK = "attack",
    DONE   = "done"
}

local entityID = 0
local targetPlayerID = 0

local config = {
    -- Stats
    maxHP = 5,
    maxAP = 2,
    movementPoints = 3,

    -- Strike
    strikeRange = 1,
    strikeDamage = 1,
    strikeAPCost = 1,

    -- Summon Reinforcements
    summonAPCost = 2,
    summonCooldownMax = 2,
    summonDeathThreshold = 2,  -- Only summon if 2+ enemies died

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

-- Summon Reinforcements
local summonCooldown = 0
local skipNextTurn = false     -- When true, skip the next turn (summoning)
local summonReady = false      -- When true, spawn the knight this turn

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

-- Find a random walkable, unoccupied tile on the map
local function FindRandomOpenTile()
    local ex, ey = GetEntityGridPosition(entityID)
    if not ex then return nil, nil end

    -- Try random offsets from current position
    local attempts = 30
    for _ = 1, attempts do
        local rx = ex + math.random(-5, 5)
        local ry = ey + math.random(-5, 5)
        if IsWalkableTile(rx, ry) and not IsTileOccupied(rx, ry) then
            return rx, ry
        end
    end
    return nil, nil
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
    -- Green color for knight (tanky)
    local r, g, b = 0.0, 0.85, 0.0
    if ratio <= 0.25 then r, g, b = 0.9, 0.1, 0.1
    elseif ratio <= 0.5 then r, g, b = 0.95, 0.65, 0.0
    elseif ratio <= 0.75 then r, g, b = 0.95, 0.95, 0.0 end
    SetSpriteColor(healthBarFG, r, g, b, 1.0)
end

-- ============================================================================
-- LIFECYCLE
-- ============================================================================

function OnInit()
    entityID = self
    if not entityID or entityID == 0 then return end
    print("[EnemyKnight " .. entityID .. "] Initialized")

    SetEntityHP(entityID, config.maxHP, config.maxHP)
    if SetEntityMaxAP then
        SetEntityMaxAP(entityID, config.maxAP)
    end

    _G.EnemiesDeadThisLevel = _G.EnemiesDeadThisLevel or 0

    ApplySheet("idleFront", false)

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
            SetSpriteColor(healthBarFG, 0.0, 0.85, 0.0, 1.0)
        end
    end
end

function OnDestroy()
    _G.EnemiesDeadThisLevel = (_G.EnemiesDeadThisLevel or 0) + 1
    local ex, ey = GetEntityGridPosition(entityID)
    if ex and ey and SetTileOccupant then SetTileOccupant(ex, ey, 0) end
    if healthBarBG and healthBarBG > 0 then DestroyEntity(healthBarBG) end
    if healthBarFG and healthBarFG > 0 then DestroyEntity(healthBarFG) end
    print("[EnemyKnight " .. entityID .. "] Destroyed")
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
        print("[EnemyKnight " .. entityID .. "] STUNNED")
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

        DamageEntity(pendingAttackTarget, pendingAttackDamage, entityID)
        print("[EnemyKnight " .. entityID .. "] Strike hit for " .. pendingAttackDamage)

        local px, py = GetEntityGridPosition(pendingAttackTarget)
        if px then PulseTile(px, py, 0.3, 1.0, 0.0, 0.0) end

        local ex, ey = GetEntityGridPosition(entityID)
        if ex and targetPlayerID > 0 then
            local px2, py2 = GetEntityGridPosition(targetPlayerID)
            if px2 then SetFacingFromDelta(px2 - ex, py2 - ey) end
        end

        -- Check if we have AP remaining to attack again
        local remainingAP = GetEntityAP(entityID)
        if remainingAP >= config.strikeAPCost then
            print("[EnemyKnight " .. entityID .. "] AP remaining=" .. remainingAP .. ", trying another attack")
            currentTurnPhase = PHASE.ATTACK
            moveTimer = 0.3  -- Brief delay between attacks for visual clarity
        else
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

        if summonCooldown > 0 then summonCooldown = summonCooldown - 1 end

        print("[EnemyKnight " .. entityID .. "] === STARTING TURN === AP=" .. select(1, GetEntityAP(entityID)) .. " MP=" .. mpRemaining .. " skipNext=" .. tostring(skipNextTurn) .. " summonReady=" .. tostring(summonReady))

        targetPlayerID = FindClosestPlayer()
        if not targetPlayerID then
            FinishAction()
            return
        end
    end

    local ok, err = pcall(ProcessTurnPhase)
    if not ok then
        print("[EnemyKnight " .. entityID .. "] ERROR: " .. tostring(err))
        FinishAction()
    end
end

-- ============================================================================
-- TURN PHASES
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
    -- Handle summon resolution: spawn the knight
    if summonReady then
        summonReady = false
        ResolveSummon()
        return
    end

    -- Handle skip turn (summoning in progress)
    if skipNextTurn then
        skipNextTurn = false
        summonReady = true
        print("[EnemyKnight " .. entityID .. "] Skipping turn (summoning)...")
        local ex, ey = GetEntityGridPosition(entityID)
        if ex then PulseTile(ex, ey, 0.5, 0.3, 0.8, 0.3) end
        FinishAction()
        return
    end

    -- Check Summon conditions: 2+ deaths, off cooldown, enough AP
    local currentAP = GetEntityAP(entityID)
    local deaths = _G.EnemiesDeadThisLevel or 0
    if deaths >= config.summonDeathThreshold and summonCooldown <= 0 and currentAP >= config.summonAPCost then
        currentTurnPhase = PHASE.SKILL
        return
    end

    currentTurnPhase = PHASE.MOVE
end

function PhaseSkill()
    -- Use Summon Reinforcements
    local currentAP = GetEntityAP(entityID)
    if currentAP >= config.summonAPCost then
        ConsumeEnemyAP(entityID, config.summonAPCost)
        skipNextTurn = true
        summonCooldown = config.summonCooldownMax
        print("[EnemyKnight " .. entityID .. "] SUMMON REINFORCEMENTS initiated! Will skip next turn.")

        local ex, ey = GetEntityGridPosition(entityID)
        if ex then PulseTile(ex, ey, 0.6, 0.3, 0.8, 0.3) end
    end

    -- Can still move after initiating summon
    currentTurnPhase = PHASE.MOVE
end

function ResolveSummon()
    -- Spawn a new Enemy Knight on a random open tile
    local tileX, tileY = FindRandomOpenTile()
    if tileX and tileY then
        -- Convert grid to world coords
        local worldX, worldY = TileToWorld(tileX, tileY)
        if worldX and worldY then
            local newKnightID = SpawnEnemyAt(worldX, worldY)
            if newKnightID and newKnightID ~= 0 then
                AddScriptComponentToEntity(newKnightID, "assets/scripts/EnemyKnightScript.lua")
                local playerID = targetPlayerID or (FindClosestPlayer() or 0)
                if playerID > 0 then
                    SetEnemyTarget(newKnightID, playerID)
                end
                print("[EnemyKnight " .. entityID .. "] SUMMONED new Knight (Entity " .. newKnightID .. ") at tile (" .. tileX .. "," .. tileY .. ")")
                PulseTile(tileX, tileY, 0.5, 0.3, 1.0, 0.3)
            end
        end
    else
        print("[EnemyKnight " .. entityID .. "] Summon failed - no open tile found!")
    end

    -- Continue with normal turn after summon resolves
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

    if CalculateDistance(ex, ey, px, py) <= config.strikeRange then
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
            PulseTile(nextTile.x, nextTile.y, 0.2, 1.0, 0.5, 0.0)
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

    ConsumeEnemyAP(entityID, config.strikeAPCost)
    SetAttackFacing(ex, ey, px, py)
    pendingAttack = true
    pendingAttackTimer = GetAttackAnimDuration()
    pendingAttackTarget = targetPlayerID
    pendingAttackDamage = config.strikeDamage
    print("[EnemyKnight " .. entityID .. "] STRIKE on Player " .. targetPlayerID)
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
    print("[EnemyKnight " .. entityID .. "] Finished turn")
    MarkEnemyActionComplete()
end
