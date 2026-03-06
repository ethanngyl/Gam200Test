--[[
===============================================================================
 File:          EnemyGeneric.lua
 Authors:       Ethan Ng Yong Le
 Date:          03/05/2026
 ------------------------------------------------------------------------------

 UNIFIED ENEMY SCRIPT - Data-Driven Enemy AI

 Brief:
    Single script that handles ALL enemy types. Reads configuration from
    JSON config files (assets/config/enemies/*.json) to determine stats,
    skills, AI behavior, animations, and visuals.

 Usage:
    -- Pass config type as 3rd parameter to AddScriptComponentToEntity
    AddScriptComponentToEntity(enemyID, "assets/scripts/EnemyGeneric.lua", "knight")
    -- Supported: "knight", "mage", "tank", "knight_commander"

 Supported Enemy Types (via JSON configs):
    knight           - Durable frontline with Summon Reinforcements
    mage             - Ranged support with Arcane Bolt + Barrier
    tank             - Heavy armor with Shield Bash + Taunt
    knight_commander - Tactical support with Strike + Rallying Cry

 Copyright (C) 2026 DigiPen Institute of Technology.
 Reproduction or disclosure of this file or its contents
 without the prior written consent of DigiPen Institute of
 Technology is prohibited.
===============================================================================
]]--

-- ============================================================================
-- TURN PHASES (shared by all enemy types)
-- ============================================================================

local PHASE = {
    INIT    = "init",
    SPECIAL = "special",
    MOVE    = "move",
    ATTACK  = "attack",
    DONE    = "done"
}

-- ============================================================================
-- ENTITY STATE
-- ============================================================================

local entityID = 0
local targetPlayerID = 0
local config = nil         -- Loaded from JSON
local enemyType = "knight" -- Default type

-- ============================================================================
-- INTERNAL STATE (shared across all enemy types)
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

-- Pending attack (melee)
local pendingAttack = false
local pendingAttackTimer = 0.0
local pendingAttackTarget = 0
local pendingAttackDamage = 0

-- Pending projectile (ranged)
local pendingProjectile = false
local pendingProjectileTimer = 0.0
local facingDirX = 0
local facingDirY = -1

-- Cooldowns
local specialCooldown = 0

-- Health bar
local healthBarBG = nil
local healthBarFG = nil
local healthBarHeight = 0.02
local healthBarOffsetY = 0.05
local healthBarLayer = 4

-- Animation
local ENEMY_ANIM = {}
local lastAnimKey = nil
local lastFlipX = false

-- Type-specific state
local tauntActive = false
local skipNextTurn = false
local summonReady = false
local lastTurnPhase = nil

-- ============================================================================
-- CONFIG LOADING
-- ============================================================================

-- Config type is now passed via the C++ ScriptSystem as a Lua global "configType"
-- (set as the 3rd parameter to AddScriptComponentToEntity)

-- Default animations (used when config doesn't specify or as fallback)
local DEFAULT_ANIM = {
    idleFront = { tex = "assets/Enemy/Enemy_Knight_Idle_Front-Sheet.png",  rows = 1, cols = 12, frames = 12, time = 0.08, loop = true  },
    idleBack  = { tex = "assets/Enemy/Enemy_Knight_Idle_Back-Sheet.png",   rows = 1, cols = 4,  frames = 4,  time = 0.10, loop = true  },
    idleSide  = { tex = "assets/Enemy/Enemy_Knight_Idle_Side-Sheet.png",   rows = 1, cols = 12, frames = 12, time = 0.08, loop = true  },
    atkFront  = { tex = "assets/Enemy/Enemy_Knight_Attack_Front-Sheet.png", rows = 1, cols = 8, frames = 8, time = 0.07, loop = false },
    atkBack   = { tex = "assets/Enemy/Enemy_Knight_Attack_Back-Sheet.png",  rows = 1, cols = 8, frames = 8, time = 0.07, loop = false },
    atkLeft   = { tex = "assets/Enemy/Enemy_Knight_Attack_Left-Sheet.png",  rows = 1, cols = 7, frames = 7, time = 0.07, loop = false },
}

local function LoadConfig()
    -- Read config type from the configType global set by C++ ScriptSystem
    if configType and configType ~= "" then
        enemyType = configType
    end

    -- Load JSON config using existing LoadJSON API
    local configPath = "assets/config/enemies/" .. enemyType .. ".json"
    local ok, loaded = pcall(LoadJSON, configPath)
    if ok and loaded then
        config = loaded
        print("[EnemyGeneric " .. entityID .. "] Loaded config: " .. configPath)
    end

    -- Fallback to default knight config if loading fails
    if not config then
        print("[EnemyGeneric " .. entityID .. "] WARNING: Failed to load config '" .. configPath .. "', using defaults")
        config = {
            type = "knight",
            displayName = "Knight",
            logTag = "EnemyKnight",
            stats = { maxHP = 5, maxAP = 2, movementPoints = 3 },
            tint = nil,
            healthBarColor = nil,
            healthBarWidth = 0.1,
            skills = {
                attack = { name = "Strike", type = "melee", range = 1, damage = 1, apCost = 1, multiAttack = true },
                special = nil
            },
            ai = { behavior = "aggressive", aggroRange = 99, maxPathLength = 10, priorityList = {"move", "attack"} },
            passives = {},
        }
    end

    -- Load animations from config or use defaults
    if config.animations then
        ENEMY_ANIM = config.animations
    else
        ENEMY_ANIM = DEFAULT_ANIM
    end
end

-- ============================================================================
-- ANIMATION
-- ============================================================================

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

local function FindRandomOpenTile()
    local ex, ey = GetEntityGridPosition(entityID)
    if not ex then return nil, nil end
    for _ = 1, 30 do
        local rx = ex + math.random(-5, 5)
        local ry = ey + math.random(-5, 5)
        if IsWalkableTile(rx, ry) and not IsTileOccupied(rx, ry) then
            return rx, ry
        end
    end
    return nil, nil
end

local function IsInLineOfSight(ex, ey, px, py)
    if ex ~= px and ey ~= py then return false end
    if ex == px then
        local step = (py > ey) and 1 or -1
        for y = ey + step, py - step, step do
            if IsTileWall and IsTileWall(ex, y) then return false end
        end
    else
        local step = (px > ex) and 1 or -1
        for x = ex + step, px - step, step do
            if IsTileWall and IsTileWall(x, ey) then return false end
        end
    end
    return true
end

local function AnyAllyLowHealth(threshold)
    local enemies = GetAllEnemies()
    if not enemies then return false end
    for _, eid in ipairs(enemies) do
        if eid ~= entityID then
            local hp, maxHP = GetEntityHP(eid)
            if hp and maxHP and maxHP > 0 and hp > 0 then
                if (hp / maxHP) < threshold then return true end
            end
        end
    end
    return false
end

local function GetLogTag()
    if config and config.logTag then return config.logTag end
    return "Enemy"
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
    local barWidth = (config and config.healthBarWidth) or 0.1
    local fgWidth = barWidth * ratio
    local fgX = wx - (barWidth - fgWidth) * 0.5
    SetSpritePosition(healthBarFG, fgX, barY)
    SetScale(healthBarFG, fgWidth, healthBarHeight)

    -- Use config color or default HP-based coloring
    if config and config.healthBarColor then
        local c = config.healthBarColor
        SetSpriteColor(healthBarFG, c[1], c[2], c[3], c[4])
    else
        local r, g, b = 0.0, 0.85, 0.0
        if ratio <= 0.25 then r, g, b = 0.9, 0.1, 0.1
        elseif ratio <= 0.5 then r, g, b = 0.95, 0.65, 0.0
        elseif ratio <= 0.75 then r, g, b = 0.95, 0.95, 0.0 end
        SetSpriteColor(healthBarFG, r, g, b, 1.0)
    end
end

-- ============================================================================
-- GLIDE SYSTEM
-- ============================================================================

local function StartGlide(fromX, fromY, toX, toY)
    glideActive = true
    glideElapsed = 0.0
    glideStartX, glideStartY = fromX, fromY
    glideEndX, glideEndY = toX, toY
    SetSpritePosition(entityID, fromX, fromY)
end

local function UpdateGlide(dt)
    if not glideActive then return false end
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
    return glideActive
end

-- ============================================================================
-- MOVEMENT SYSTEM (shared)
-- ============================================================================

local function MoveOneStep()
    if mpRemaining <= 0 then return false, "no_mp" end

    local ex, ey = GetEntityGridPosition(entityID)
    local px, py = GetEntityGridPosition(targetPlayerID)
    if not ex or not px then return false, "no_pos" end

    local attackSkill = config.skills.attack
    local attackRange = (attackSkill and attackSkill.range) or 1

    -- Already in attack range for melee
    if attackSkill and attackSkill.type == "melee" then
        if CalculateDistance(ex, ey, px, py) <= attackRange then
            return false, "in_range"
        end
    end

    -- For ranged: check if in line of sight at good range
    if attackSkill and attackSkill.type == "projectile" then
        local preferredDist = (attackSkill.preferredDistance or config.ai.preferredDistance or 3)
        local dist = CalculateDistance(ex, ey, px, py)
        if attackSkill.requiresLineOfSight then
            if IsInLineOfSight(ex, ey, px, py) and dist >= 2 and dist <= attackRange then
                return false, "in_range"
            end
        end

        -- Flee if too close (ranged behavior)
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
                        StartGlide(sx, sy, endX, endY)
                    end
                    mpRemaining = mpRemaining - 1
                    return true, "moved"
                end
            end
        end
    end

    -- Pathfind toward target
    if #currentPath == 0 or pathIndex > #currentPath or pathTargetPlayerID ~= targetPlayerID
       or lastKnownPlayerX ~= px or lastKnownPlayerY ~= py then
        currentPath = FindPathToTarget(ex, ey, px, py)
        pathIndex = 1
        pathTargetPlayerID = targetPlayerID
        lastKnownPlayerX = px
        lastKnownPlayerY = py
        if not currentPath or #currentPath == 0 then
            return false, "no_path"
        end
    end

    if pathIndex <= #currentPath and mpRemaining > 0 then
        local nextTile = currentPath[pathIndex]
        if not IsWalkableTile(nextTile.x, nextTile.y) or IsTileOccupied(nextTile.x, nextTile.y) then
            currentPath = {}
            return false, "blocked"
        end

        SetFacingFromDelta(nextTile.x - ex, nextTile.y - ey)
        local sx, sy = GetEntityWorldPosition(entityID)
        local success = MoveEntityToTile(entityID, nextTile.x, nextTile.y)
        if success then
            local endX, endY = GetEntityWorldPosition(entityID)
            if sx and sy and endX and endY then
                StartGlide(sx, sy, endX, endY)
            end
            mpRemaining = mpRemaining - 1
            pathIndex = pathIndex + 1
            ShowTileBorder(nextTile.x, nextTile.y, 0.3)
            -- Pulse color based on enemy type
            local tint = config.tint
            if tint then
                PulseTile(nextTile.x, nextTile.y, 0.2, tint[1], tint[2], tint[3])
            else
                PulseTile(nextTile.x, nextTile.y, 0.2, 1.0, 0.5, 0.0)
            end
            return true, "moved"
        else
            currentPath = {}
            return false, "move_failed"
        end
    end

    return false, "path_end"
end

-- ============================================================================
-- SPECIAL SKILL HANDLERS
-- ============================================================================

local function ExecuteSpecial_Barrier()
    local special = config.skills.special
    local currentAP = GetEntityAP(entityID)
    if currentAP < special.apCost then return false end

    local barrierTarget = FindRandomAlly()
    if not barrierTarget and special.targetSelfIfNoAlly then
        barrierTarget = entityID
    end
    if not barrierTarget then return false end

    if HasStatusEffect and HasStatusEffect(barrierTarget, special.statusEffect) then
        return false
    end

    ConsumeEnemyAP(entityID, special.apCost)
    specialCooldown = special.cooldownMax

    if ApplyStatusEffect then
        ApplyStatusEffect(barrierTarget, special.statusEffect, special.statusDuration or -1, entityID)
    end

    print("[" .. GetLogTag() .. " " .. entityID .. "] " .. special.name .. " on Entity " .. barrierTarget)

    local tx, ty = GetEntityGridPosition(barrierTarget)
    if tx then PulseTile(tx, ty, 0.5, 0.2, 0.4, 1.0) end
    local ex, ey = GetEntityGridPosition(entityID)
    if ex then PulseTile(ex, ey, 0.3, 0.2, 0.4, 1.0) end

    return true
end

local function ExecuteSpecial_Summon()
    local special = config.skills.special
    local currentAP = GetEntityAP(entityID)
    if currentAP < special.apCost then return false end

    -- Check death threshold
    local deaths = _G.EnemiesDeadThisLevel or 0
    if deaths < (special.summonDeathThreshold or 2) then return false end

    ConsumeEnemyAP(entityID, special.apCost)
    skipNextTurn = true
    specialCooldown = special.cooldownMax

    print("[" .. GetLogTag() .. " " .. entityID .. "] " .. special.name .. " initiated! Will skip next turn.")
    local ex, ey = GetEntityGridPosition(entityID)
    if ex then PulseTile(ex, ey, 0.6, 0.3, 0.8, 0.3) end

    return true
end

local function ResolveSummon()
    local special = config.skills.special
    local tileX, tileY = FindRandomOpenTile()
    if tileX and tileY then
        local worldX, worldY = TileToWorld(tileX, tileY)
        if worldX and worldY then
            local newID = SpawnEnemyAt(worldX, worldY)
            if newID and newID ~= 0 then
                -- Use the configured summon config type, or same type as self
                local summonType = special.summonConfig or config.type
                AddScriptComponentToEntity(newID, special.summonScript or "assets/scripts/EnemyGeneric.lua", summonType)
                local playerID = targetPlayerID or (FindClosestPlayer() or 0)
                if playerID > 0 then
                    SetEnemyTarget(newID, playerID)
                end
                print("[" .. GetLogTag() .. " " .. entityID .. "] SUMMONED new " .. summonType .. " (Entity " .. newID .. ") at tile (" .. tileX .. "," .. tileY .. ")")
                PulseTile(tileX, tileY, 0.5, 0.3, 1.0, 0.3)
            end
        end
    else
        print("[" .. GetLogTag() .. " " .. entityID .. "] Summon failed - no open tile found!")
    end
end

local function ExecuteSpecial_RallyingCry()
    local special = config.skills.special
    local currentAP = GetEntityAP(entityID)
    if currentAP < special.apCost then return false end

    if _G.RallyingCryPending or _G.RallyingCryActive then return false end

    ConsumeEnemyAP(entityID, special.apCost)
    _G.RallyingCryPending = true
    specialCooldown = special.cooldownMax

    print("[" .. GetLogTag() .. " " .. entityID .. "] Used " .. special.name .. "! All enemies get +1 MP next turn")

    local enemies = GetAllEnemies()
    if enemies then
        for _, eid in ipairs(enemies) do
            local ex, ey = GetEntityGridPosition(eid)
            if ex then PulseTile(ex, ey, 0.4, 0.6, 0.2, 0.8) end
        end
    end

    return true
end

local function ExecuteSpecial_Taunt()
    local special = config.skills.special
    local currentAP = GetEntityAP(entityID)
    if currentAP < special.apCost then return false end
    if tauntActive then return false end

    -- Check condition
    if special.condition == "ally_low_health" then
        if not AnyAllyLowHealth(special.conditionThreshold or 0.5) then
            return false
        end
    end

    ConsumeEnemyAP(entityID, special.apCost)
    tauntActive = true

    -- Apply knightsOath to all allies
    local enemies = GetAllEnemies()
    if enemies then
        for _, eid in ipairs(enemies) do
            if eid ~= entityID then
                local hp = GetEntityHP(eid)
                if hp and hp > 0 and ApplyStatusEffect then
                    ApplyStatusEffect(eid, special.statusEffect, special.statusDuration or -1, entityID)
                    print("[" .. GetLogTag() .. " " .. entityID .. "] Taunt applied to ally " .. eid)
                end
            end
        end
    end

    print("[" .. GetLogTag() .. " " .. entityID .. "] TAUNT ACTIVATED! Redirecting ally damage to self.")

    if enemies then
        for _, eid in ipairs(enemies) do
            local ex, ey = GetEntityGridPosition(eid)
            if ex then PulseTile(ex, ey, 0.5, 0.9, 0.75, 0.1) end
        end
    end

    return true
end

-- Dispatch special skill based on config type
local function TryExecuteSpecial()
    local special = config.skills.special
    if not special then return false end

    local currentAP = GetEntityAP(entityID)
    if currentAP < special.apCost then return false end
    if specialCooldown > 0 then return false end

    if special.type == "buff_ally" then return ExecuteSpecial_Barrier()
    elseif special.type == "summon" then return ExecuteSpecial_Summon()
    elseif special.type == "rallying_cry" then return ExecuteSpecial_RallyingCry()
    elseif special.type == "taunt" then return ExecuteSpecial_Taunt()
    end

    return false
end

-- ============================================================================
-- ATTACK HANDLERS
-- ============================================================================

local function TryExecuteAttack_Melee()
    local attack = config.skills.attack
    local currentAP = GetEntityAP(entityID)
    if currentAP < attack.apCost then return false end

    local ex, ey = GetEntityGridPosition(entityID)
    local px, py = GetEntityGridPosition(targetPlayerID)
    if not ex or not px then return false end

    if CalculateDistance(ex, ey, px, py) > attack.range then return false end

    ConsumeEnemyAP(entityID, attack.apCost)
    SetAttackFacing(ex, ey, px, py)
    pendingAttack = true
    pendingAttackTimer = GetAttackAnimDuration()
    pendingAttackTarget = targetPlayerID
    pendingAttackDamage = attack.damage

    print("[" .. GetLogTag() .. " " .. entityID .. "] " .. attack.name .. " on Player " .. targetPlayerID)
    return true
end

local function TryExecuteAttack_Projectile()
    local attack = config.skills.attack
    local currentAP = GetEntityAP(entityID)
    if currentAP < attack.apCost then return false end

    local ex, ey = GetEntityGridPosition(entityID)
    local px, py = GetEntityGridPosition(targetPlayerID)
    if not ex or not px then return false end

    if attack.requiresLineOfSight and not IsInLineOfSight(ex, ey, px, py) then
        return false
    end

    ConsumeEnemyAP(entityID, attack.apCost)
    SetAttackFacing(ex, ey, px, py)
    pendingProjectile = true
    pendingProjectileTimer = GetAttackAnimDuration()

    print("[" .. GetLogTag() .. " " .. entityID .. "] " .. attack.name .. " toward Player " .. targetPlayerID .. " dir=(" .. facingDirX .. "," .. facingDirY .. ")")
    return true
end

local function TryExecuteAttack()
    local attack = config.skills.attack
    if not attack then return false end

    if attack.type == "melee" then return TryExecuteAttack_Melee()
    elseif attack.type == "projectile" then return TryExecuteAttack_Projectile()
    end

    return false
end

-- ============================================================================
-- PENDING ATTACK RESOLUTION
-- ============================================================================

local function ResolvePendingMeleeAttack()
    pendingAttack = false
    local attack = config.skills.attack

    DamageEntity(pendingAttackTarget, pendingAttackDamage, entityID)
    print("[" .. GetLogTag() .. " " .. entityID .. "] " .. attack.name .. " hit for " .. pendingAttackDamage)

    -- Apply status effect if configured
    if attack.statusEffect and ApplyStatusEffect then
        ApplyStatusEffect(pendingAttackTarget, attack.statusEffect, attack.statusDuration or 1, entityID)
        print("[" .. GetLogTag() .. " " .. entityID .. "] Applied " .. attack.statusEffect .. " to Player " .. pendingAttackTarget)
    end

    local px, py = GetEntityGridPosition(pendingAttackTarget)
    if px then PulseTile(px, py, 0.3, 1.0, 0.0, 0.0) end

    -- Return to idle facing
    local ex, ey = GetEntityGridPosition(entityID)
    if ex and targetPlayerID > 0 then
        local px2, py2 = GetEntityGridPosition(targetPlayerID)
        if px2 then SetFacingFromDelta(px2 - ex, py2 - ey) end
    end

    -- Multi-attack: check if we can attack again
    if attack.multiAttack then
        local remainingAP = GetEntityAP(entityID)
        if remainingAP >= attack.apCost then
            print("[" .. GetLogTag() .. " " .. entityID .. "] AP remaining=" .. remainingAP .. ", trying another attack")
            currentTurnPhase = PHASE.ATTACK
            moveTimer = 0.3
            return
        end
    end

    FinishAction()
end

local function ResolvePendingProjectile()
    pendingProjectile = false
    local attack = config.skills.attack

    local wx, wy = GetEntityWorldPosition(entityID)
    if wx and wy and SpawnSkillProjectile then
        local tint = attack.projectileTint or {0.4, 0.2, 1.0, 1.0}
        local spawnOffsetX = facingDirX * 0.05
        local spawnOffsetY = facingDirY * 0.05
        SpawnSkillProjectile(
            wx + spawnOffsetX, wy + spawnOffsetY,
            facingDirX, facingDirY,
            attack.projectileSpeed or 3.0,
            attack.damage,
            false,
            tint[1], tint[2], tint[3], tint[4],
            "",
            true,
            entityID
        )
        print("[" .. GetLogTag() .. " " .. entityID .. "] " .. attack.name .. " fired! dmg=" .. attack.damage)
    end

    -- Multi-attack: check if we can fire again
    if attack.multiAttack then
        local remainingAP = GetEntityAP(entityID)
        if remainingAP >= attack.apCost then
            print("[" .. GetLogTag() .. " " .. entityID .. "] AP remaining=" .. remainingAP .. ", checking for another shot")
            currentTurnPhase = PHASE.ATTACK
            moveTimer = 0.3
            return
        end
    end

    -- Return to idle facing
    local ex, ey = GetEntityGridPosition(entityID)
    if ex and targetPlayerID > 0 then
        local px, py = GetEntityGridPosition(targetPlayerID)
        if px then SetFacingFromDelta(px - ex, py - ey) end
    end

    FinishAction()
end

-- ============================================================================
-- PASSIVE HANDLERS
-- ============================================================================

local function ApplyPassives_OnInit()
    if not config.passives then return end
    for _, passive in ipairs(config.passives) do
        if passive.type == "damageReduction" then
            if ApplyStatusEffect then
                ApplyStatusEffect(entityID, passive.statusEffect, -1, entityID, 0, passive.value or 1)
                print("[" .. GetLogTag() .. " " .. entityID .. "] " .. passive.name .. " applied (" .. (passive.value or 1) .. " damage reduction)")
            end
        elseif passive.type == "bolstered_morale" then
            -- Handled by onInit hook
        end
    end
end

local function ApplyBolsteredMorale()
    local enemies = GetAllEnemies()
    if not enemies then return end
    for _, eid in ipairs(enemies) do
        if eid ~= entityID then
            local current = GetDamageModifier(eid) or 0
            SetDamageModifier(eid, current + 1)
            print("[" .. GetLogTag() .. " " .. entityID .. "] Bolstered Morale: +1 damageModifier on enemy " .. eid)
        end
    end
end

local function RemoveBolsteredMorale()
    local enemies = GetAllEnemies()
    if not enemies then return end
    for _, eid in ipairs(enemies) do
        if eid ~= entityID then
            local current = GetDamageModifier(eid) or 0
            local newVal = current - 1
            if newVal < 0 then newVal = 0 end
            SetDamageModifier(eid, newVal)
        end
    end
end

local function RefreshTaunt()
    if not tauntActive then return end
    local special = config.skills.special
    if not special or special.type ~= "taunt" then return end
    local enemies = GetAllEnemies()
    if not enemies then return end
    for _, eid in ipairs(enemies) do
        if eid ~= entityID then
            local hp = GetEntityHP(eid)
            if hp and hp > 0 then
                if HasStatusEffect and not HasStatusEffect(eid, special.statusEffect) then
                    if ApplyStatusEffect then
                        ApplyStatusEffect(eid, special.statusEffect, special.statusDuration or -1, entityID)
                    end
                end
            end
        end
    end
end

-- Rallying Cry lifecycle (knight commander only)
local function ManageRallyingCryLifecycle(currentTurn)
    if not config.skills.special or config.skills.special.type ~= "rallying_cry" then return end
    if currentTurn == "Enemy" and lastTurnPhase ~= "Enemy" then
        if _G.RallyingCryPending then
            _G.RallyingCryActive = true
            _G.RallyingCryPending = false
            print("[" .. GetLogTag() .. " " .. entityID .. "] Rallying Cry ACTIVATED for this turn!")
        end
    elseif currentTurn ~= "Enemy" and lastTurnPhase == "Enemy" then
        if _G.RallyingCryActive then
            _G.RallyingCryActive = false
            print("[" .. GetLogTag() .. " " .. entityID .. "] Rallying Cry expired")
        end
    end
    lastTurnPhase = currentTurn
end

-- ============================================================================
-- LIFECYCLE
-- ============================================================================

function OnInit()
    entityID = self
    if not entityID or entityID == 0 then return end

    LoadConfig()

    print("[" .. GetLogTag() .. " " .. entityID .. "] Initialized (type: " .. config.type .. ")")

    -- Set stats
    SetEntityHP(entityID, config.stats.maxHP, config.stats.maxHP)
    if SetEntityMaxAP then
        SetEntityMaxAP(entityID, config.stats.maxAP)
    end

    _G.EnemiesDeadThisLevel = _G.EnemiesDeadThisLevel or 0

    ApplySheet("idleFront", false)

    -- Apply tint
    if config.tint and SetSpriteColor then
        SetSpriteColor(entityID, config.tint[1], config.tint[2], config.tint[3], config.tint[4])
    end

    -- Set tile occupancy
    local ex, ey = GetEntityGridPosition(entityID)
    if ex and ey and SetTileOccupant then
        SetTileOccupant(ex, ey, entityID)
    end

    targetPlayerID = FindClosestPlayer() or 0

    -- Apply passives
    ApplyPassives_OnInit()

    -- onInit hooks
    if config.onInit == "bolstered_morale_apply" then
        _G.KnightCommanderIDs = _G.KnightCommanderIDs or {}
        table.insert(_G.KnightCommanderIDs, entityID)
        ApplyBolsteredMorale()
    end

    -- Spawn health bar
    local barWidth = config.healthBarWidth or 0.1
    local wx, wy = GetEntityWorldPosition(entityID)
    if wx and wy then
        local barY = wy + healthBarOffsetY
        healthBarBG = SpawnSprite("", wx, barY, barWidth, healthBarHeight, healthBarLayer)
        if healthBarBG and healthBarBG > 0 then
            SetSpriteColor(healthBarBG, 0.15, 0.15, 0.15, 0.85)
        end
        healthBarFG = SpawnSprite("", wx, barY, barWidth, healthBarHeight, healthBarLayer + 1)
        if healthBarFG and healthBarFG > 0 then
            if config.healthBarColor then
                local c = config.healthBarColor
                SetSpriteColor(healthBarFG, c[1], c[2], c[3], c[4])
            else
                SetSpriteColor(healthBarFG, 0.0, 0.85, 0.0, 1.0)
            end
        end
    end
end

function OnDestroy()
    -- onDestroy hooks
    if config then
        if config.onDestroy == "bolstered_morale_remove" then
            RemoveBolsteredMorale()
            if _G.KnightCommanderIDs then
                for i, id in ipairs(_G.KnightCommanderIDs) do
                    if id == entityID then
                        table.remove(_G.KnightCommanderIDs, i)
                        break
                    end
                end
            end
        elseif config.onDestroy == "remove_taunt" then
            if tauntActive and RemoveStatusEffect then
                local enemies = GetAllEnemies()
                if enemies then
                    for _, eid in ipairs(enemies) do
                        if eid ~= entityID then
                            if HasStatusEffect and HasStatusEffect(eid, "knightsOath") then
                                if GetStatusEffectSource and GetStatusEffectSource(eid, "knightsOath") == entityID then
                                    RemoveStatusEffect(eid, "knightsOath")
                                    print("[" .. GetLogTag() .. " " .. entityID .. "] Taunt removed from ally " .. eid .. " (died)")
                                end
                            end
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
    print("[" .. GetLogTag() .. " " .. entityID .. "] Destroyed")
end

-- ============================================================================
-- MAIN UPDATE
-- ============================================================================

function OnUpdate(dt)
    UpdateHealthBar()

    -- Type-specific per-frame updates
    if config then
        if config.skills.special and config.skills.special.type == "taunt" then
            RefreshTaunt()
        end
        ManageRallyingCryLifecycle(GetCurrentTurn())
    end

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

    -- Handle glide animation
    if UpdateGlide(dt) then return end

    -- Sequential turn check
    local ok1, isActive = pcall(IsActiveEnemy, entityID)
    if not ok1 or not isActive then return end
    local ok2, actionReady = pcall(IsEnemyActionReady)
    if not ok2 or not actionReady then return end
    if hasActedThisTurn then return end

    -- Stun check
    if HasStatusEffect and HasStatusEffect(entityID, "stun") then
        print("[" .. GetLogTag() .. " " .. entityID .. "] STUNNED")
        DecrementStatusEffects(entityID)
        FinishAction()
        return
    end

    if DecrementStatusEffects then DecrementStatusEffects(entityID) end

    -- Move timer
    if moveTimer > 0 then
        moveTimer = moveTimer - dt
        if moveTimer < 0 then moveTimer = 0 end
        return
    end

    -- Safety timeout
    activeTimer = activeTimer + dt
    if activeTimer >= maxActiveTime then
        FinishAction()
        return
    end

    -- Handle pending melee attack
    if pendingAttack then
        pendingAttackTimer = pendingAttackTimer - dt
        if pendingAttackTimer > 0 then return end
        ResolvePendingMeleeAttack()
        return
    end

    -- Handle pending projectile
    if pendingProjectile then
        pendingProjectileTimer = pendingProjectileTimer - dt
        if pendingProjectileTimer > 0 then return end
        ResolvePendingProjectile()
        return
    end

    -- First frame of turn
    if not isMyTurnToAct then
        isMyTurnToAct = true
        activeTimer = 0.0
        currentTurnPhase = PHASE.INIT

        local bonusMP = (_G.RallyingCryActive) and 1 or 0
        mpRemaining = config.stats.movementPoints + bonusMP

        if specialCooldown > 0 then specialCooldown = specialCooldown - 1 end

        print("[" .. GetLogTag() .. " " .. entityID .. "] === STARTING TURN === AP=" .. select(1, GetEntityAP(entityID)) .. " MP=" .. mpRemaining)

        targetPlayerID = FindClosestPlayer()
        if not targetPlayerID then
            FinishAction()
            return
        end
    end

    -- Process turn phases
    local ok, err = pcall(ProcessTurnPhase)
    if not ok then
        print("[" .. GetLogTag() .. " " .. entityID .. "] ERROR: " .. tostring(err))
        FinishAction()
    end
end

-- ============================================================================
-- TURN PHASE PROCESSING
-- ============================================================================

function ProcessTurnPhase()
    if currentTurnPhase == PHASE.INIT then
        PhaseInit()
    elseif currentTurnPhase == PHASE.SPECIAL then
        PhaseSpecial()
    elseif currentTurnPhase == PHASE.MOVE then
        PhaseMove()
    elseif currentTurnPhase == PHASE.ATTACK then
        PhaseAttack()
    else
        FinishAction()
    end
end

function PhaseInit()
    local special = config.skills.special

    -- Handle summon resolution (knight)
    if special and special.type == "summon" then
        if summonReady then
            summonReady = false
            ResolveSummon()
            currentTurnPhase = PHASE.MOVE
            return
        end
        if skipNextTurn then
            skipNextTurn = false
            summonReady = true
            print("[" .. GetLogTag() .. " " .. entityID .. "] Skipping turn (summoning)...")
            local ex, ey = GetEntityGridPosition(entityID)
            if ex then PulseTile(ex, ey, 0.5, 0.3, 0.8, 0.3) end
            FinishAction()
            return
        end
    end

    -- Walk priority list from config
    local priorities = config.ai.priorityList or {"move", "attack"}
    for _, action in ipairs(priorities) do
        if action == "special" and special then
            local currentAP = GetEntityAP(entityID)
            if specialCooldown <= 0 and currentAP >= special.apCost then
                -- Additional condition checks
                local shouldUse = true
                if special.type == "taunt" and tauntActive then shouldUse = false end
                if special.type == "rallying_cry" and (_G.RallyingCryPending or _G.RallyingCryActive) then shouldUse = false end
                if special.type == "summon" then
                    local deaths = _G.EnemiesDeadThisLevel or 0
                    if deaths < (special.summonDeathThreshold or 2) then shouldUse = false end
                end

                if shouldUse then
                    currentTurnPhase = PHASE.SPECIAL
                    return
                end
            end
            -- If special not available, continue to next priority
        elseif action == "move" then
            -- Check if attack is possible without moving (for ranged)
            local attack = config.skills.attack
            if attack and attack.type == "projectile" and attack.requiresLineOfSight then
                local currentAP = GetEntityAP(entityID)
                if currentAP >= attack.apCost and targetPlayerID then
                    local ex, ey = GetEntityGridPosition(entityID)
                    local px, py = GetEntityGridPosition(targetPlayerID)
                    if ex and px then
                        local dist = CalculateDistance(ex, ey, px, py)
                        if IsInLineOfSight(ex, ey, px, py) and dist >= 2 and dist <= attack.range then
                            currentTurnPhase = PHASE.ATTACK
                            return
                        end
                    end
                end
            end
            currentTurnPhase = PHASE.MOVE
            return
        elseif action == "attack" then
            currentTurnPhase = PHASE.ATTACK
            return
        end
    end

    currentTurnPhase = PHASE.MOVE
end

function PhaseSpecial()
    TryExecuteSpecial()
    currentTurnPhase = PHASE.MOVE
end

function PhaseMove()
    local moved, reason = MoveOneStep()
    if moved then
        return  -- Wait for glide
    end

    if reason == "in_range" or reason == "no_mp" or reason == "no_path" or reason == "blocked" or reason == "move_failed" or reason == "path_end" or reason == "no_pos" then
        currentTurnPhase = PHASE.ATTACK
    end
end

function PhaseAttack()
    if not TryExecuteAttack() then
        FinishAction()
    end
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
    pendingProjectile = false
    currentTurnPhase = PHASE.DONE
    print("[" .. GetLogTag() .. " " .. entityID .. "] Finished turn")
    MarkEnemyActionComplete()
end
