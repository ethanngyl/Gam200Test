--[[
===============================================================================
 File:          BossScript.lua
 Authors:       Ethan Ng
 Email:         n.ethanyongle@digipen.edu
 Date:          02/25/2026
 ------------------------------------------------------------------------------

 BOSS SCRIPT - Enhanced Knight Boss AI for Grid-Based Turn-Based Combat

 Brief:
    A boss variant of the enemy knight with a 4-skill system.
    Uses the same EnemyTurnManager integration as regular enemies (tagged "Enemy").
    Has the same HP/AP as regular enemies. Skills do NOT consume AP;
    the boss may use up to 2 skills per turn. Movement still costs AP.

 Skills:
    1. Heavy Strike  - Melee attack dealing attackDamage + 1 (range 1)
    2. Cross Slash    - Hits all tiles in a cross pattern, 3 tiles each direction
    3. Mark Target    - Marks highest-HP player; boss pathfinds only to them
                        for 3 turns. Both deal +1 damage to each other.
    4. Execute        - At <= 50% HP, once per fight. Charges 1 turn, then
                        teleports adjacent to lowest-HP player and kills them.

 Skill Priority (each turn):
    Skill 4 > Skill 3 (if no mark active) > random(Skill 1, Skill 2)

 Turn Flow:
    1. Resolve charge/execute states from previous turn
    2. Check Skill 4 conditions  (uses turn immediately)
    3. Check Skill 3 conditions  (consumes 1 skill slot)
    4. Move toward target        (consumes AP, same as regular enemy)
    5. Use remaining skill slots  (random Skill 1 or 2 if in range)

 Copyright (C) 2026 DigiPen Institute of Technology.
 Reproduction or disclosure of this file or its contents
 without the prior written consent of DigiPen Institute of
 Technology is prohibited.
===============================================================================
]]--

-- ============================================================================
-- SKILL DEFINITIONS
-- ============================================================================

local SKILL = {
    HEAVY_STRIKE = 1,  -- Melee, attackDamage + 1
    CROSS_SLASH  = 2,  -- AoE cross, 3 tiles each direction
    MARK_TARGET  = 3,  -- Mark highest-HP player for 3 turns
    EXECUTE      = 4   -- Charge → teleport → instant kill
}

-- ============================================================================
-- TURN PHASES (within a single boss turn)
-- ============================================================================

local PHASE = {
    INIT  = "init",   -- Resolve special states, decide skills
    MOVE  = "move",   -- Pathfind and move toward target
    SKILL = "skill",  -- Use combat skills (1 and/or 2)
    DONE  = "done"    -- Turn finished
}

-- ============================================================================
-- ENTITY STATE
-- ============================================================================

local entityID = 0
local targetPlayerID = 0

-- AI config (same defaults as regular enemy)
local config = {
    apCostPerMove   = 1,
    maxMovesPerTurn = 3,
    attackRange     = 1,      -- Melee range for Skill 1
    attackDamage    = 1,      -- Base damage (regular enemy value)
    crossSlashRange = 3,      -- Tiles in each cardinal direction for Skill 2
    crossSlashDamage = 1,     -- Damage per tile hit for Skill 2
    maxSkillsPerTurn = 2,
    markDuration     = 3,     -- Turns that Skill 3 mark lasts
    aggroRange       = 99,    -- Boss always chases (like AGGRESSIVE)
    maxPathLength    = 15
}

-- ============================================================================
-- INTERNAL STATE
-- ============================================================================

-- Turn management
local hasActedThisTurn = false
local lastEnemyTurn = nil
local isMyTurnToAct = false
local currentTurnPhase = PHASE.INIT
local skillsRemaining = 0

-- Movement (same system as EnemyScript)
local moveTimer = 0.0
local moveDelay = 0.55
local movesThisTurn = 0
local currentPath = {}
local pathIndex = 1
local pathTargetPlayerID = 0
local lastKnownPlayerX = nil
local lastKnownPlayerY = nil

-- Safety timer
local activeTimer = 0.0
local maxActiveTime = 8.0

-- Smooth glide state
local glideActive = false
local glideElapsed = 0.0
local glideDuration = 0.5
local glideStartX = 0.0
local glideStartY = 0.0
local glideEndX = 0.0
local glideEndY = 0.0

-- Pending skill animation
local pendingSkillActive = false
local pendingSkillTimer = 0.0
local pendingSkillID = 0
local pendingSkillTargets = {}   -- list of {entityID, damage}

-- Skill 3 (Mark Target) state
local markedTargetID = 0         -- Entity ID of marked player (0 = none)
local markTurnsRemaining = 0     -- Turns remaining on mark

-- Skill 4 (Execute) state
local executeUsed = false        -- Can only be used once per fight
local chargingActive = false     -- Currently in charge state
local skipNextTurn = false       -- Next turn is the skip turn
local executeReady = false       -- Ready to teleport and execute

-- Health bar state
local healthBarBG = nil
local healthBarFG = nil
local healthBarWidth = 0.4       -- Wider bar for boss
local healthBarHeight = 0.02     -- Slightly taller
local healthBarOffsetY = 0.22
local healthBarLayer = 2

-- Boss idle state: immune and does nothing until all 3 players enter the arena
local bossActivated = false
local localArenaBounds = nil  -- Set in OnInit from shared C++ store

-- ============================================================================
-- ANIMATION (reuses enemy knight sheets)
-- ============================================================================

local BOSS_ANIM = {
    idleFront = { tex = "assets/Enemy/Boss1_Idle_Front-Sheet.png",  rows = 4, cols = 3, frames = 12, time = 0.08, loop = true  },
    idleBack  = { tex = "assets/Enemy/Boss1_Idle_Back-Sheet.png",   rows = 3, cols = 3, frames = 7,  time = 0.10, loop = true  },
    idleSide  = { tex = "assets/Enemy/Boss1_Idle_Side-Sheet.png",   rows = 4, cols = 3, frames = 12, time = 0.08, loop = true  },

    walkFront = { tex = "assets/Enemy/Boss1_Walk_Front2-Sheet.png", rows = 3, cols = 2, frames = 6,  time = 0.08, loop = true  },
    walkBack  = { tex = "assets/Enemy/Boss1_Walk_Back2.png",        rows = 3, cols = 2, frames = 6,  time = 0.08, loop = true  },
    walkSide  = { tex = "assets/Enemy/Boss1_Walk_Side-Sheet.png",   rows = 3, cols = 2, frames = 6,  time = 0.08, loop = true  },

    atkFront  = { tex = "assets/Enemy/Boss1_Attack_Front-Sheet.png", rows = 3, cols = 3, frames = 8, time = 0.07, loop = false },
    atkBack   = { tex = "assets/Enemy/Boss1_Attack_Back-Sheet.png",  rows = 3, cols = 3, frames = 8, time = 0.07, loop = false },
    atkLeft   = { tex = "assets/Enemy/Boss1_Attack_Left-Sheet.png",  rows = 3, cols = 3, frames = 7, time = 0.07, loop = false }
}

local lastAnimKey = nil
local lastFlipX = false
local warnedMissingAnimAPI = false

local function ApplySheet(animKey, flipX)
    if not SetSpriteAnimationSheet then
        if not warnedMissingAnimAPI then
            print("[BossScript] SetSpriteAnimationSheet is NIL; boss animation sheets unavailable")
            warnedMissingAnimAPI = true
        end
        return
    end
    if animKey == lastAnimKey and flipX == lastFlipX then return end

    local a = BOSS_ANIM[animKey]
    if not a then
        print("[BossScript] Missing BOSS_ANIM key: " .. tostring(animKey))
        return
    end

    local ok = SetSpriteAnimationSheet(entityID, a.tex, a.rows, a.cols, a.frames, a.time, a.loop)
    if not ok then
        print("[BossScript] ApplySheet failed for key=" .. tostring(animKey) .. ", tex=" .. tostring(a.tex))
        if SetSpriteTexture then
            SetSpriteTexture(entityID, a.tex)
        end
    end
    if SetAnimationFlipX then
        SetAnimationFlipX(entityID, flipX and true or false)
    end
    lastAnimKey = animKey
    lastFlipX = flipX and true or false
end

local function SetFacingFromDelta(dx, dy, isMoving)
    local animKey = nil
    local flipX = false

    if math.abs(dx) > math.abs(dy) then
        if isMoving and BOSS_ANIM["walkSide"] then
            animKey = "walkSide"
        else
            animKey = "idleSide"
        end

        flipX = (dx > 0)
    else
        if dy > 0 then
            if isMoving and BOSS_ANIM["walkBack"] then
                animKey = "walkBack"
            else
                animKey = "idleBack"
            end
        else
            if isMoving and BOSS_ANIM["walkFront"] then
                animKey = "walkFront"
            else
                animKey = "idleFront"
            end
        end
    end

    ApplySheet(animKey, flipX)
end

local function SetAttackFacing(ex, ey, px, py)
    local dx = px - ex
    local dy = py - ey
    if math.abs(dx) > math.abs(dy) then
        ApplySheet("atkLeft", dx > 0)
    else
        if dy > 0 then ApplySheet("atkBack", false)
        else ApplySheet("atkFront", false) end
    end
end

-- ============================================================================
-- HELPER FUNCTIONS
-- ============================================================================

local function CalculateDistance(x1, y1, x2, y2)
    return math.abs(x2 - x1) + math.abs(y2 - y1)
end

-- Find the closest living player
local function FindClosestPlayer()
    local enemyX, enemyY = GetEntityGridPosition(entityID)
    if not enemyX then return nil end

    local players = GetAllPlayers()
    if not players or #players == 0 then return nil end

    local closestID = nil
    local closestDist = 999999
    for _, pid in ipairs(players) do
        local hp, _ = GetEntityHP(pid)
        if hp and hp > 0 then
            local px, py = GetEntityGridPosition(pid)
            if px then
                local dist = CalculateDistance(enemyX, enemyY, px, py)
                if dist < closestDist then
                    closestDist = dist
                    closestID = pid
                end
            end
        end
    end
    return closestID, closestDist
end

-- Find the player with the highest current HP
local function FindHighestHPPlayer()
    local players = GetAllPlayers()
    if not players or #players == 0 then return nil end

    local bestID = nil
    local bestHP = -1
    for _, pid in ipairs(players) do
        local hp, _ = GetEntityHP(pid)
        if hp and hp > 0 and hp > bestHP then
            bestHP = hp
            bestID = pid
        end
    end
    return bestID
end

-- Find the player with the lowest current HP (alive)
local function FindLowestHPPlayer()
    local players = GetAllPlayers()
    if not players or #players == 0 then return nil end

    local bestID = nil
    local bestHP = 999999
    for _, pid in ipairs(players) do
        local hp, _ = GetEntityHP(pid)
        if hp and hp > 0 and hp < bestHP then
            bestHP = hp
            bestID = pid
        end
    end
    return bestID
end

-- Check if a given entity ID is a living player
local function IsPlayerEntity(eid)
    if not eid or eid == 0 then return false end
    local players = GetAllPlayers()
    if not players then return false end
    for _, pid in ipairs(players) do
        if pid == eid then
            local hp, _ = GetEntityHP(pid)
            return hp and hp > 0
        end
    end
    return false
end

-- Get the attack animation duration for pending skill timing
local function GetAttackAnimDuration()
    local key = lastAnimKey
    local a = BOSS_ANIM[key]
    if a then return (a.frames * a.time) + 0.02 end
    return 0.20
end

-- Determine the pathfind target based on mark state
local function GetPathfindTarget()
    -- If Skill 3 is active, pathfind only to marked target
    if markedTargetID > 0 and markTurnsRemaining > 0 then
        local hp, _ = GetEntityHP(markedTargetID)
        if hp and hp > 0 then
            return markedTargetID
        end
        -- Marked target is dead, clear mark
        markedTargetID = 0
        markTurnsRemaining = 0
    end
    -- Default: closest player
    local closest, _ = FindClosestPlayer()
    return closest
end

-- Calculate bonus damage from mark
local function GetMarkBonus(targetID)
    if markedTargetID > 0 and markTurnsRemaining > 0 and targetID == markedTargetID then
        return 1
    end
    return 0
end

-- ============================================================================
-- ARENA CHECK (must be defined before OnInit so OnInit can call it as a local)
-- ============================================================================

-- Check if all living players are inside the arena bounds
local function AreAllPlayersInArena()
    local arena = localArenaBounds
    if not arena then return false end

    local players = GetAllPlayers()
    if not players or #players == 0 then return false end

    local count = 0
    local alive = 0
    for _, pid in ipairs(players) do
        local hp = GetEntityHP(pid)
        if hp and hp > 0 then
            alive = alive + 1
            local px, py = GetEntityGridPosition(pid)
            if px and py
               and px >= arena.minX and px < arena.maxX
               and py >= arena.minY and py < arena.maxY then
                count = count + 1
            end
        end
    end

    return alive > 0 and count == alive
end

-- ============================================================================
-- LIFECYCLE CALLBACKS
-- ============================================================================

function OnInit()
    entityID = self
    if not entityID or entityID == 0 then
        Log("[BossScript] ERROR: Invalid entity ID")
        return
    end
    Log("[BossScript] Boss " .. entityID .. " initialized")

    currentPath = {}
    pathIndex = 1
    ApplySheet("idleFront", false)

    -- Set initial tile occupancy
    local bx, by = GetEntityGridPosition(entityID)
    if bx and by and SetTileOccupant then
        SetTileOccupant(bx, by, entityID)
    end

    targetPlayerID = FindClosestPlayer() or 0

    -- Boss-specific base stats (override generic enemy spawn defaults).
    SetEntityHP(entityID, 10, 10)

    -- Read arena bounds from shared C++ store (set by ProceduralMapLevel.lua)
    local aMinX = GetSharedInt("arenaMinX", -1)
    local aMinY = GetSharedInt("arenaMinY", -1)
    local aMaxX = GetSharedInt("arenaMaxX", -1)
    local aMaxY = GetSharedInt("arenaMaxY", -1)
    if aMinX >= 0 and aMinY >= 0 then
        localArenaBounds = {
            minX = aMinX,
            minY = aMinY,
            maxX = aMaxX,
            maxY = aMaxY
        }
        print("[Boss " .. entityID .. "] Arena bounds from shared store: (" .. aMinX .. "," .. aMinY
            .. ") to (" .. aMaxX .. "," .. aMaxY .. ")")
    else
        print("[Boss " .. entityID .. "] WARNING: No arena bounds in shared store!")
    end

    -- Boss is spawned by ProceduralMapLevel only when AreAllSurvivingPlayersInArena() is true.
    -- Activate immediately if players are already in the arena (covers the case where a player
    -- died elsewhere to satisfy the "all surviving players in arena" condition, after which the
    -- remaining players may move out before the first enemy turn, causing AreAllPlayersInArena()
    -- in OnUpdate to return false and permanently block boss activation).
    if AreAllPlayersInArena() then
        bossActivated = true
        print("[Boss " .. entityID .. "] Immediately activated (all surviving players confirmed in arena at spawn time)")
    else
        -- Players not yet in arena (safety fallback - apply immune and wait)
        if ApplyStatusEffect then
            ApplyStatusEffect(entityID, "immune", -1, entityID)
            print("[Boss " .. entityID .. "] Immune until all players enter arena")
        end
    end

    -- Spawn health bar sprites above boss
    local wx, wy = GetEntityWorldPosition(entityID)
    if wx and wy then
        local barY = wy + healthBarOffsetY
        healthBarBG = SpawnSprite("", wx, barY, healthBarWidth, healthBarHeight, healthBarLayer)
        if healthBarBG and healthBarBG > 0 then
            SetSpriteColor(healthBarBG, 0.15, 0.15, 0.15, 0.85)
        end
        healthBarFG = SpawnSprite("", wx, barY, healthBarWidth, healthBarHeight, healthBarLayer + 1)
        if healthBarFG and healthBarFG > 0 then
            SetSpriteColor(healthBarFG, 0.85, 0.0, 0.0, 1.0)  -- red for boss
        end
    end
end

function OnDestroy()
    -- Clean up health bar sprites
    if healthBarBG and healthBarBG > 0 then
        DestroyEntity(healthBarBG)
        healthBarBG = nil
    end
    if healthBarFG and healthBarFG > 0 then
        DestroyEntity(healthBarFG)
        healthBarFG = nil
    end

    local bx, by = GetEntityGridPosition(entityID)
    if bx and by and SetTileOccupant then
        SetTileOccupant(bx, by, 0)
    end
    Log("[BossScript] Boss " .. entityID .. " destroyed")
end

-- ============================================================================
-- MAIN UPDATE
-- ============================================================================

-- Update boss health bar position and fill
local function UpdateHealthBar()
    if not healthBarBG or not healthBarFG then return end
    if healthBarBG <= 0 or healthBarFG <= 0 then return end

    local wx, wy = GetEntityWorldPosition(entityID)
    if not wx or not wy then return end

    local barY = wy + healthBarOffsetY
    SetSpritePosition(healthBarBG, wx, barY)

    local currentHP, maxHP = GetEntityHP(entityID)
    if not currentHP or not maxHP or maxHP <= 0 then return end

    local ratio = currentHP / maxHP
    if ratio < 0 then ratio = 0 end
    if ratio > 1 then ratio = 1 end

    local fgWidth = healthBarWidth * ratio
    local fgX = wx - (healthBarWidth - fgWidth) * 0.5
    SetSpritePosition(healthBarFG, fgX, barY)
    SetScale(healthBarFG, fgWidth, healthBarHeight)

    -- Boss bar: always red, but brighter when lower HP
    local r, g, b = 0.85, 0.0, 0.0
    if ratio <= 0.25 then
        r, g, b = 1.0, 0.0, 0.0
    elseif ratio <= 0.5 then
        r, g, b = 0.9, 0.15, 0.0
    end
    SetSpriteColor(healthBarFG, r, g, b, 1.0)
end

function OnUpdate(dt)
    -- Update health bar every frame
    UpdateHealthBar()

    -- Boss stays idle until all players enter the arena
    if not bossActivated then
        if AreAllPlayersInArena() then
            bossActivated = true
            if RemoveStatusEffect then
                RemoveStatusEffect(entityID, "immune")
            end
            print("[Boss " .. entityID .. "] All players in arena - BOSS ACTIVATED!")
        else
            -- Players not yet in arena.  If the enemy turn system has made us the
            -- active enemy we MUST still finish our action, otherwise the whole
            -- enemy turn hangs permanently.  Skip any real action but release the turn.
            local currentTurn = GetCurrentTurn and GetCurrentTurn()
            if currentTurn == "Enemy" then
                local ok1, isActive = pcall(IsActiveEnemy, entityID)
                if ok1 and isActive then
                    local ok2, actionReady = pcall(IsEnemyActionReady)
                    if ok2 and actionReady and not hasActedThisTurn then
                        print("[Boss " .. entityID .. "] Not yet activated — passing turn to unblock enemy phase")
                        FinishBossAction()
                    end
                end
            end
            return
        end
    end

    local currentTurn = GetCurrentTurn()

    -- Reset state when leaving enemy turn
    if currentTurn ~= "Enemy" then
        if lastEnemyTurn == "Enemy" then
            hasActedThisTurn = false
            isMyTurnToAct = false
            moveTimer = 0.0
            movesThisTurn = 0
            activeTimer = 0.0
            currentTurnPhase = PHASE.INIT
            if glideActive then
                SetSpritePosition(entityID, glideEndX, glideEndY)
                glideActive = false
            end
        end
        lastEnemyTurn = currentTurn
        return
    end
    lastEnemyTurn = "Enemy"

    -- Handle glide animation
    if glideActive then
        glideElapsed = glideElapsed + dt
        local t = glideElapsed / glideDuration
        if t > 1.0 then t = 1.0 end
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

    -- Sequential turn checks
    local ok1, isActive = pcall(IsActiveEnemy, entityID)
    if not ok1 or not isActive then return end

    local ok2, actionReady = pcall(IsEnemyActionReady)
    if not ok2 or not actionReady then return end

    if hasActedThisTurn then return end

    -- Check if boss is stunned (status effects are decremented centrally in ResetPartyTurn)
    if HasStatusEffect and HasStatusEffect(entityID, "stun") then
        print("[Boss " .. entityID .. "] STUNNED - skipping turn")
        FinishBossAction()
        return
    end

    if moveTimer > 0 then
        moveTimer = moveTimer - dt
        if moveTimer < 0 then moveTimer = 0 end
        return
    end

    -- Safety timeout
    activeTimer = activeTimer + dt
    if activeTimer >= maxActiveTime then
        print("[Boss " .. entityID .. "] SAFETY: force-finishing turn")
        FinishBossAction()
        return
    end

    -- First frame of this turn
    if not isMyTurnToAct then
        isMyTurnToAct = true
        movesThisTurn = 0
        activeTimer = 0.0
        skillsRemaining = config.maxSkillsPerTurn
        currentTurnPhase = PHASE.INIT
        print("[Boss " .. entityID .. "] ========== STARTING TURN ==========")

        targetPlayerID = GetPathfindTarget()
        if not targetPlayerID or targetPlayerID == 0 then
            print("[Boss " .. entityID .. "] No player found!")
            FinishBossAction()
            return
        end
    end

    -- Handle pending skill animation
    if pendingSkillActive then
        pendingSkillTimer = pendingSkillTimer - dt
        if pendingSkillTimer > 0 then return end

        -- Apply damage to all pending targets
        pendingSkillActive = false
        for _, t in ipairs(pendingSkillTargets) do
            print("[Boss " .. entityID .. "] Skill " .. pendingSkillID .. " hits Entity " .. t.entityID .. " for " .. t.damage)
            DamageEntity(t.entityID, t.damage, entityID)
            local px, py = GetEntityGridPosition(t.entityID)
            if px then
                PulseTile(px, py, 0.3, 1.0, 0.0, 0.0)
            end
        end
        pendingSkillTargets = {}

        -- Return to idle facing
        local ex, ey = GetEntityGridPosition(entityID)
        if ex and targetPlayerID > 0 then
            local px, py = GetEntityGridPosition(targetPlayerID)
            if px then SetFacingFromDelta(px - ex, py - ey, false) end
        end

        -- Continue to next skill or finish
        if skillsRemaining > 0 and currentTurnPhase == PHASE.SKILL then
            -- Try another skill next frame
            return
        end
        FinishBossAction()
        return
    end

    -- Process current turn phase
    local phaseOk, phaseErr = pcall(ProcessTurnPhase)
    if not phaseOk then
        print("[Boss " .. entityID .. "] ERROR: " .. tostring(phaseErr))
        FinishBossAction()
    end
end

-- ============================================================================
-- TURN PHASE PROCESSING
-- ============================================================================

function ProcessTurnPhase()
    if currentTurnPhase == PHASE.INIT then
        PhaseInit()
    elseif currentTurnPhase == PHASE.MOVE then
        PhaseMove()
    elseif currentTurnPhase == PHASE.SKILL then
        PhaseSkill()
    else
        FinishBossAction()
    end
end

-- ============================================================================
-- PHASE: INIT - Resolve special states and decide pre-movement skills
-- ============================================================================

function PhaseInit()
    print("[Boss " .. entityID .. "] Phase: INIT")

    -- 1. Resolve Execute (teleport + kill from previous charge)
    if executeReady then
        executeReady = false
        ResolveExecute()
        return
    end

    -- 2. Handle charge skip turn
    if skipNextTurn then
        skipNextTurn = false
        executeReady = true
        print("[Boss " .. entityID .. "] Charging... skipping this turn")
        -- Pulse boss tile to show charging state
        local bx, by = GetEntityGridPosition(entityID)
        if bx then PulseTile(bx, by, 0.5, 0.8, 0.0, 0.8) end  -- Purple pulse
        FinishBossAction()
        return
    end

    -- 3. Check Skill 4 priority (highest): HP <= 50% and not used yet
    if not executeUsed then
        local hp, maxHP = GetEntityHP(entityID)
        if hp and maxHP and maxHP > 0 and (hp / maxHP) <= 0.5 then
            print("[Boss " .. entityID .. "] HP at " .. hp .. "/" .. maxHP .. " - using EXECUTE (Skill 4)")
            UseSkillExecute()
            return
        end
    end

    -- 4. Check Skill 3 priority: no marked target
    if markedTargetID == 0 or markTurnsRemaining <= 0 then
        -- Clear expired mark
        markedTargetID = 0
        markTurnsRemaining = 0

        local highestHP = FindHighestHPPlayer()
        if highestHP then
            UseSkillMarkTarget(highestHP)
            skillsRemaining = skillsRemaining - 1
        end
    else
        -- Decrement mark duration
        markTurnsRemaining = markTurnsRemaining - 1
        if markTurnsRemaining <= 0 then
            print("[Boss " .. entityID .. "] Mark on Player " .. markedTargetID .. " expired")
            markedTargetID = 0
        else
            print("[Boss " .. entityID .. "] Mark on Player " .. markedTargetID .. " (" .. markTurnsRemaining .. " turns left)")
        end
    end

    -- Update pathfind target based on mark
    targetPlayerID = GetPathfindTarget()
    if not targetPlayerID or targetPlayerID == 0 then
        FinishBossAction()
        return
    end

    -- Transition to movement phase
    currentTurnPhase = PHASE.MOVE
    PhaseMove()
end

-- ============================================================================
-- PHASE: MOVE - Pathfind and move toward target (same as EnemyScript chase)
-- ============================================================================

function PhaseMove()
    local currentAP, _ = GetEntityAP(entityID)
    if not currentAP or currentAP < config.apCostPerMove then
        -- No AP for movement, skip to skill phase
        currentTurnPhase = PHASE.SKILL
        return
    end

    local enemyX, enemyY = GetEntityGridPosition(entityID)
    local playerX, playerY = GetEntityGridPosition(targetPlayerID)
    if not enemyX or not playerX then
        currentTurnPhase = PHASE.SKILL
        return
    end

    -- Check if we need a new path
    local needsNewPath = false
    if #currentPath == 0 or pathIndex > #currentPath then
        needsNewPath = true
    elseif pathTargetPlayerID ~= targetPlayerID then
        needsNewPath = true
    elseif lastKnownPlayerX ~= playerX or lastKnownPlayerY ~= playerY then
        needsNewPath = true
    end

    if needsNewPath then
        currentPath = FindPathToTarget(enemyX, enemyY, playerX, playerY)
        pathIndex = 1
        pathTargetPlayerID = targetPlayerID
        lastKnownPlayerX = playerX
        lastKnownPlayerY = playerY

        if not currentPath or #currentPath == 0 then
            print("[Boss " .. entityID .. "] No path to target")
            currentTurnPhase = PHASE.SKILL
            return
        end
    end

    -- Check if already in melee range — skip to skill phase
    local distToTarget = CalculateDistance(enemyX, enemyY, playerX, playerY)
    if distToTarget <= config.attackRange then
        currentTurnPhase = PHASE.SKILL
        return
    end

    -- Calculate max moves (reserve nothing — skills don't cost AP)
    local maxMoves = config.maxMovesPerTurn

    -- Move one tile per frame
    if currentAP >= config.apCostPerMove and pathIndex <= #currentPath and movesThisTurn < maxMoves then
        local nextTile = currentPath[pathIndex]

        if not IsWalkableTile(nextTile.x, nextTile.y) then
            currentPath = {}
            currentTurnPhase = PHASE.SKILL
            return
        end

        if IsTileOccupied(nextTile.x, nextTile.y) then
            currentPath = {}
            currentTurnPhase = PHASE.SKILL
            return
        end

        -- Update facing
        local dx = nextTile.x - enemyX
        local dy = nextTile.y - enemyY
        SetFacingFromDelta(dx, dy, true)

        -- Smooth glide
        local sx, sy = GetEntityWorldPosition(entityID)
        local success = MoveEntityToTile(entityID, nextTile.x, nextTile.y)
        if success then
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
            movesThisTurn = movesThisTurn + 1
            pathIndex = pathIndex + 1
            ShowTileBorder(nextTile.x, nextTile.y, 0.3)
            PulseTile(nextTile.x, nextTile.y, 0.2, 0.8, 0.0, 0.0)  -- Dark red pulse for boss
            return  -- Wait for glide to finish
        else
            currentPath = {}
            currentTurnPhase = PHASE.SKILL
            return
        end
    end

    -- Can't move more — transition to skill phase
    currentTurnPhase = PHASE.SKILL
end

-- ============================================================================
-- PHASE: SKILL - Use remaining skill slots with Skill 1 or 2
-- ============================================================================

function PhaseSkill()
    if skillsRemaining <= 0 then
        FinishBossAction()
        return
    end

    local bossX, bossY = GetEntityGridPosition(entityID)
    if not bossX then
        FinishBossAction()
        return
    end

    -- Determine which skills are usable based on player positions
    local canUseSkill1 = false  -- Need adjacent player
    local canUseSkill2 = false  -- Need player in cross pattern
    local skill1Target = nil
    local skill2Targets = {}

    local players = GetAllPlayers()
    if players then
        for _, pid in ipairs(players) do
            local hp, _ = GetEntityHP(pid)
            if hp and hp > 0 then
                local px, py = GetEntityGridPosition(pid)
                if px then
                    local dist = CalculateDistance(bossX, bossY, px, py)
                    -- Skill 1: adjacent (range 1)
                    if dist <= config.attackRange then
                        canUseSkill1 = true
                        skill1Target = pid
                    end
                    -- Skill 2: in cross pattern (same row or column, within range)
                    if (px == bossX and math.abs(py - bossY) <= config.crossSlashRange) or
                       (py == bossY and math.abs(px - bossX) <= config.crossSlashRange) then
                        canUseSkill2 = true
                        table.insert(skill2Targets, { entityID = pid, x = px, y = py })
                    end
                end
            end
        end
    end

    if not canUseSkill1 and not canUseSkill2 then
        -- No players in range for any skill
        FinishBossAction()
        return
    end

    -- Pick a random available skill
    local availableSkills = {}
    if canUseSkill1 then table.insert(availableSkills, SKILL.HEAVY_STRIKE) end
    if canUseSkill2 then table.insert(availableSkills, SKILL.CROSS_SLASH) end
    local chosen = availableSkills[math.random(#availableSkills)]

    if chosen == SKILL.HEAVY_STRIKE then
        UseSkillHeavyStrike(skill1Target)
    elseif chosen == SKILL.CROSS_SLASH then
        UseSkillCrossSlash(bossX, bossY, skill2Targets)
    end
    skillsRemaining = skillsRemaining - 1
    -- pendingSkillActive is set by the skill functions; OnUpdate will resolve it
end

-- ============================================================================
-- SKILL IMPLEMENTATIONS
-- ============================================================================

-- Skill 1: Heavy Strike — melee attack, attackDamage + 1, plus mark bonus
function UseSkillHeavyStrike(targetID)
    local bossX, bossY = GetEntityGridPosition(entityID)
    local px, py = GetEntityGridPosition(targetID)
    if not bossX or not px then return end

    local damage = config.attackDamage + 1 + GetMarkBonus(targetID)
    print("[Boss " .. entityID .. "] Skill 1: Heavy Strike on Player " .. targetID .. " for " .. damage .. " dmg")

    SetAttackFacing(bossX, bossY, px, py)

    pendingSkillActive = true
    pendingSkillID = SKILL.HEAVY_STRIKE
    pendingSkillTimer = GetAttackAnimDuration()
    pendingSkillTargets = { { entityID = targetID, damage = damage } }
end

-- Skill 2: Cross Slash — AoE, 3 tiles each cardinal direction, plus mark bonus
function UseSkillCrossSlash(bossX, bossY, playersInCross)
    print("[Boss " .. entityID .. "] Skill 2: Cross Slash from (" .. bossX .. "," .. bossY .. ")")

    -- Visual: pulse all cross tiles
    local directions = { {0,-1}, {0,1}, {-1,0}, {1,0} }
    for _, dir in ipairs(directions) do
        for i = 1, config.crossSlashRange do
            local tx = bossX + dir[1] * i
            local ty = bossY + dir[2] * i
            PulseTile(tx, ty, 0.4, 1.0, 0.3, 0.0)  -- Orange pulse for cross slash
        end
    end

    -- Build damage list from players in the cross pattern
    local targets = {}
    for _, p in ipairs(playersInCross) do
        local damage = config.crossSlashDamage + GetMarkBonus(p.entityID)
        table.insert(targets, { entityID = p.entityID, damage = damage })
        print("[Boss " .. entityID .. "]   -> Will hit Player " .. p.entityID .. " at (" .. p.x .. "," .. p.y .. ") for " .. damage)
    end

    -- Play attack animation facing the first target (or front if none)
    if #playersInCross > 0 then
        SetAttackFacing(bossX, bossY, playersInCross[1].x, playersInCross[1].y)
    else
        ApplySheet("atkFront", false)
    end

    pendingSkillActive = true
    pendingSkillID = SKILL.CROSS_SLASH
    pendingSkillTimer = GetAttackAnimDuration()
    pendingSkillTargets = targets
end

-- Skill 3: Mark Target — mark highest-HP player for 3 turns
function UseSkillMarkTarget(targetID)
    markedTargetID = targetID
    markTurnsRemaining = config.markDuration
    print("[Boss " .. entityID .. "] Skill 3: Marked Player " .. targetID .. " for " .. config.markDuration .. " turns")

    -- Visual: pulse the marked player's tile
    local px, py = GetEntityGridPosition(targetID)
    if px then
        PulseTile(px, py, 0.6, 0.8, 0.0, 0.8)  -- Purple pulse for mark
    end

    -- Pulse boss tile too
    local bx, by = GetEntityGridPosition(entityID)
    if bx then
        PulseTile(bx, by, 0.6, 0.8, 0.0, 0.8)
    end
end

-- Skill 4: Execute — enter charging state, end turn immediately
function UseSkillExecute()
    executeUsed = true
    chargingActive = true
    skipNextTurn = true
    print("[Boss " .. entityID .. "] Skill 4: EXECUTE — entering charge state!")

    -- Visual: pulse boss tile to show charging
    local bx, by = GetEntityGridPosition(entityID)
    if bx then
        PulseTile(bx, by, 0.8, 1.0, 0.0, 0.0)  -- Red pulse for charge
    end

    FinishBossAction()
end

-- Execute resolution: teleport adjacent to lowest-HP player and kill them
function ResolveExecute()
    chargingActive = false
    local targetID = FindLowestHPPlayer()
    if not targetID then
        print("[Boss " .. entityID .. "] Execute: No valid target found")
        currentTurnPhase = PHASE.MOVE
        PhaseMove()
        return
    end

    local px, py = GetEntityGridPosition(targetID)
    if not px then
        print("[Boss " .. entityID .. "] Execute: Cannot find target position")
        currentTurnPhase = PHASE.MOVE
        PhaseMove()
        return
    end

    -- Find an adjacent walkable, unoccupied tile next to the target
    local adjacentDirs = { {0,-1}, {0,1}, {-1,0}, {1,0} }
    local teleportX, teleportY = nil, nil
    for _, dir in ipairs(adjacentDirs) do
        local tx = px + dir[1]
        local ty = py + dir[2]
        if IsWalkableTile(tx, ty) and not IsTileOccupied(tx, ty) then
            teleportX = tx
            teleportY = ty
            break
        end
    end

    if not teleportX then
        print("[Boss " .. entityID .. "] Execute: No adjacent tile available!")
        -- Fall back to normal turn
        currentTurnPhase = PHASE.MOVE
        PhaseMove()
        return
    end

    -- Teleport to the tile
    print("[Boss " .. entityID .. "] Execute: Teleporting to (" .. teleportX .. "," .. teleportY .. ")")
    MoveEntityToTile(entityID, teleportX, teleportY)

    -- Visual: red pulse on boss and target tiles
    PulseTile(teleportX, teleportY, 0.5, 1.0, 0.0, 0.0)
    PulseTile(px, py, 0.5, 1.0, 0.0, 0.0)

    -- Execute the target (instant kill)
    local currentHP, _ = GetEntityHP(targetID)
    if currentHP and currentHP > 0 then
        print("[Boss " .. entityID .. "] EXECUTING Player " .. targetID .. " (dealing " .. currentHP .. " damage)")
        SetAttackFacing(teleportX, teleportY, px, py)

        pendingSkillActive = true
        pendingSkillID = SKILL.EXECUTE
        pendingSkillTimer = GetAttackAnimDuration()
        pendingSkillTargets = { { entityID = targetID, damage = currentHP } }
    else
        FinishBossAction()
    end
end

-- ============================================================================
-- TURN COORDINATION
-- ============================================================================

function FinishBossAction()
    hasActedThisTurn = true
    movesThisTurn = 0
    moveTimer = 0.0
    glideActive = false
    activeTimer = 0.0
    currentPath = {}
    pathIndex = 1
    pendingSkillActive = false
    pendingSkillTargets = {}
    currentTurnPhase = PHASE.DONE
    Log("[Boss " .. entityID .. "] Finished turn")
    MarkEnemyActionComplete()
end

-- ============================================================================
-- GLOBAL QUERY: allows player scripts to check if they are marked
-- ============================================================================

-- Other scripts can call: IsBossMarkActive(playerID) to check mark status
function IsBossMarkActive(playerID)
    return markedTargetID == playerID and markTurnsRemaining > 0
end
_G.IsBossMarkActive = IsBossMarkActive
