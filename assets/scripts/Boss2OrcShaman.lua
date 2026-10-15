--[[
===============================================================================
 File:          Boss2OrcShaman.lua
 Authors:       Ethan Ng
 Email:         n.ethanyongle@digipen.edu
 Date:          03/13/2026
 ------------------------------------------------------------------------------

 ORC SHAMAN BOSS (Boss 2) - Orc Summoning Ritual encounter

 Summary:
    - Boss remains inactive and immune until all living players enter arena.
    - First active turn always casts Preparatory Rites (8-turn countdown).
    - During countdown, boss cannot move.
    - Boss cycles totems in strict order: Unkilling -> Massacre -> Blight.
    - Only one totem may be active at a time; boss passes while totem lives.
    - After all 3 totem types are killed once in the current cycle:
        * Boss is stunned for 1 turn (internal stun),
        * Rites countdown increases by 1,
        * Boss takes bonus backlash damage.
    - When countdown reaches 0, boss becomes empowered:
        * Can move,
        * Spawns 5 Orc Warriors,
        * Enemy-wide buffs: +1 attack, +1 MP, +1 AP.
    - After activation, one Orc Warrior is spawned every 2 enemy rounds.

 Totems:
    - Totem of Unkilling (3 HP): 5x5 zone, fatal damage in zone is prevented (HP set to 1).
    - Totem of Massacre (3 HP): 5x5 zone, invulnerable while enemies are inside zone.
      When an enemy dies in zone, totem takes 1 damage. At 0 HP, all entities take 3 damage.
    - Totem of Blight (5 HP): 5x5 zone. At end of enemy round, all entities take 1 damage.
      If entities inside zone > 4, rites countdown is reduced by 1.

===============================================================================
]]--

local TOTEM_TYPE = {
    NONE = 0,
    UNKILLING = 1,
    MASSACRE = 2,
    BLIGHT = 3
}

local entityID = 0

local bossActivated = false
local localArenaBounds = nil
local spawnWorldCache = nil

local hasActedThisTurn = false
local isMyTurnToAct = false
local lastEnemyTurn = nil

local prepWasCast = false
local ritesCountdown = 0
local empowered = false
local stunnedTurns = 0
local totemOrderIndex = 1
local totemKilledInCycle = {
    [TOTEM_TYPE.UNKILLING] = false,
    [TOTEM_TYPE.MASSACRE] = false,
    [TOTEM_TYPE.BLIGHT] = false
}

local currentTotem = {
    type = TOTEM_TYPE.NONE,
    entity = 0,
    maxHP = 0,
    lastHP = 0,
    centerX = 0,
    centerY = 0,
    lifetimeRound = 0,
    massacreDeathsCounted = 0
}

local trackedEntityState = {}
local enemyRoundCount = 0
local lastKnownTurn = "Player"
local bonusSpawnLastRound = 0

local totemZoneEmitters = {}  -- particle emitter IDs for the active totem's zone
local bossTotemEmitter = nil  -- purple particle emitter on the boss when totem is active

local BOSS_CONFIG = {
    maxHP = 10,
    moveMPCountdown = 0,
    moveMPEmpowered = 2,
    maxMovesPerTurn = 2,
    apCostPerMove = 1,
    prepCountdownStart = 8,
    totemZoneRadius = 2,
    cycleBacklashDamage = 2,
    totemicBonusAttack = 1,
    totemicBonusMP = 1,
    totemicBonusAP = 1,
    extraOrcsOnEmpower = 5,
    periodicSpawnEveryEnemyRounds = 2
}

-- ============================================================================
-- ANIMATION (Warlock Boss sprite sheets)
-- ============================================================================

local BOSS_ANIM = {
    idleFront = { tex = "assets/Enemy/Warlock_Boss_Idle_Front-Sheet.png",  rows = 1, cols = 5, frames = 5, time = 0.12, loop = true  },
    idleBack  = { tex = "assets/Enemy/Warlock_Boss_Idle_Back-Sheet.png",   rows = 1, cols = 5, frames = 5, time = 0.12, loop = true  },
    idleSide  = { tex = "assets/Enemy/Warlock_Boss_Idle_Side-Sheet.png",   rows = 1, cols = 5, frames = 5, time = 0.12, loop = true  },

    walkFront = { tex = "assets/Enemy/Warlock_Boss_Walk_Front-Sheet.png",  rows = 1, cols = 6, frames = 6, time = 0.10, loop = true  },
    walkBack  = { tex = "assets/Enemy/Warlock_Boss_Walk_Back-Sheet.png",   rows = 1, cols = 6, frames = 6, time = 0.10, loop = true  },
    walkSide  = { tex = "assets/Enemy/Warlock_Boss_Walk_Side-Sheet.png",   rows = 1, cols = 6, frames = 6, time = 0.10, loop = true  },

    atkFront  = { tex = "assets/Enemy/Warlock_Boss_Attack_Front-Sheet.png", rows = 1, cols = 8, frames = 8, time = 0.07, loop = false },
    atkBack   = { tex = "assets/Enemy/Warlock_Boss_Attack_Back-Sheet.png",  rows = 1, cols = 8, frames = 8, time = 0.07, loop = false },
    atkSide   = { tex = "assets/Enemy/Warlock_Boss_Attack_Side-Sheet.png",  rows = 1, cols = 8, frames = 8, time = 0.07, loop = false }
}

local lastAnimKey = nil
local lastFlipX = false
local warnedMissingAnimAPI = false

local function ApplySheet(animKey, flipX)
    if not SetSpriteAnimationSheet then
        if not warnedMissingAnimAPI then
            print("[Boss2] SetSpriteAnimationSheet is NIL; animation sheets unavailable")
            warnedMissingAnimAPI = true
        end
        return
    end
    if animKey == lastAnimKey and flipX == lastFlipX then return end

    local a = BOSS_ANIM[animKey]
    if not a then
        print("[Boss2] Missing BOSS_ANIM key: " .. tostring(animKey))
        return
    end

    local ok = SetSpriteAnimationSheet(entityID, a.tex, a.rows, a.cols, a.frames, a.time, a.loop)
    if not ok then
        print("[Boss2] ApplySheet failed for key=" .. tostring(animKey) .. ", tex=" .. tostring(a.tex))
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
        if isMoving then
            animKey = "walkSide"
        else
            animKey = "idleSide"
        end
        flipX = (dx > 0)
    else
        if dy > 0 then
            if isMoving then
                animKey = "walkBack"
            else
                animKey = "idleBack"
            end
        else
            if isMoving then
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
        ApplySheet("atkSide", dx > 0)
    else
        if dy > 0 then ApplySheet("atkBack", false)
        else ApplySheet("atkFront", false) end
    end
end

local function ApplyBossVisuals()
    ApplySheet("idleFront", false)
end

local moveTimer = 0.0
local moveDelay = 0.45
local movesThisTurn = 0
local currentPath = {}
local pathIndex = 1
local targetPlayerID = 0

local glideActive = false
local glideElapsed = 0.0
local glideDuration = 0.35
local glideStartX = 0.0
local glideStartY = 0.0
local glideEndX = 0.0
local glideEndY = 0.0
local pendingFinishAfterGlide = false

-- Health bar sprites
local healthBarBG = nil
local healthBarFG = nil
local healthBarWidth = 0.4
local healthBarHeight = 0.02
local healthBarOffsetY = 0.22
local healthBarLayer = 2

local function GetEntityHPValue(eid)
    local hp = GetEntityHP(eid)
    if type(hp) == "number" then
        return hp
    end
    return nil
end

local function IsAlive(eid)
    if not eid or eid == 0 then return false end
    if IsEntityValid and not IsEntityValid(eid) then return false end
    local hp = GetEntityHPValue(eid)
    return hp and hp > 0
end

local function InArena(x, y)
    local a = localArenaBounds
    if not a then return false end
    return x >= a.minX and x < a.maxX and y >= a.minY and y < a.maxY
end

local function IsInsideTotemZone(totem, x, y)
    if not totem or totem.type == TOTEM_TYPE.NONE then return false end
    if not totem.entity or totem.entity == 0 then return false end
    if not IsAlive(totem.entity) then return false end
    local dx = math.abs(x - totem.centerX)
    local dy = math.abs(y - totem.centerY)
    return dx <= BOSS_CONFIG.totemZoneRadius and dy <= BOSS_CONFIG.totemZoneRadius
end

local function GetAllLivingPlayers()
    local result = {}
    local players = GetAllPlayers()
    if not players then return result end
    for _, pid in ipairs(players) do
        if IsAlive(pid) then
            table.insert(result, pid)
        end
    end
    return result
end

local function GetAllLivingEnemies()
    local result = {}
    local enemies = GetAllEnemies()
    if not enemies then return result end
    for _, eid in ipairs(enemies) do
        if IsAlive(eid) then
            table.insert(result, eid)
        end
    end
    return result
end

local function GetAllLivingCombatants()
    local entities = {}
    for _, pid in ipairs(GetAllLivingPlayers()) do table.insert(entities, pid) end
    for _, eid in ipairs(GetAllLivingEnemies()) do table.insert(entities, eid) end
    return entities
end

local function FindClosestLivingPlayer()
    local bx, by = GetEntityGridPosition(entityID)
    if not bx then return 0 end
    local bestID = 0
    local bestDist = 999999
    for _, pid in ipairs(GetAllLivingPlayers()) do
        local px, py = GetEntityGridPosition(pid)
        if px then
            local d = math.abs(px - bx) + math.abs(py - by)
            if d < bestDist then
                bestDist = d
                bestID = pid
            end
        end
    end
    return bestID
end

local function BuildSpawnWorldCache()
    if spawnWorldCache then return end
    spawnWorldCache = {}
    if not localArenaBounds then return end
    for y = localArenaBounds.minY, localArenaBounds.maxY - 1 do
        for x = localArenaBounds.minX, localArenaBounds.maxX - 1 do
            if IsWalkableTile(x, y) then
                local wx, wy = TileToWorld(x, y)
                if wx and wy then
                    table.insert(spawnWorldCache, { x = x, y = y, wx = wx, wy = wy })
                end
            end
        end
    end
end

local function FindRandomFreeTileInArena()
    BuildSpawnWorldCache()
    if not spawnWorldCache or #spawnWorldCache == 0 then return nil end

    for _ = 1, 80 do
        local c = spawnWorldCache[math.random(#spawnWorldCache)]
        if c and not IsTileOccupied(c.x, c.y) then
            return c
        end
    end
    return nil
end

local function FindRandomAdjacentFreeTile(anchorX, anchorY)
    local dirs = {
        {0, -1}, {0, 1}, {-1, 0}, {1, 0},
        {-1, -1}, {-1, 1}, {1, -1}, {1, 1}
    }

    for i = #dirs, 2, -1 do
        local j = math.random(i)
        dirs[i], dirs[j] = dirs[j], dirs[i]
    end

    for _, d in ipairs(dirs) do
        local tx = anchorX + d[1]
        local ty = anchorY + d[2]
        local skip = localArenaBounds and not InArena(tx, ty)
        if not skip and IsWalkableTile(tx, ty) and not IsTileOccupied(tx, ty) then
            return tx, ty
        end
    end
    return nil, nil
end

local function SpawnOrcWarriorAtTile(tile)
    if not tile then return 0 end
    local eid = SpawnEnemyAt(tile.wx, tile.wy)
    if eid and eid ~= 0 then
        AddScriptComponentToEntity(eid, "assets/scripts/OrcWarriorScript.lua")
        local target = FindClosestLivingPlayer()
        if target and target > 0 then
            SetEnemyTarget(eid, target)
        end
        -- Apply empowered buffs if boss is empowered
        if empowered and SetSharedInt then
            SetSharedInt("orc_empowered_atk_" .. tostring(eid), BOSS_CONFIG.totemicBonusAttack)
            SetSharedInt("orc_empowered_mp_" .. tostring(eid), BOSS_CONFIG.totemicBonusMP)
            if SetEntityMaxAP then
                local ap, maxAp = GetEntityAP(eid)
                if ap and maxAp then
                    SetEntityMaxAP(eid, maxAp + BOSS_CONFIG.totemicBonusAP)
                end
            end
        end
        PulseTile(tile.x, tile.y, 0.25, 0.45, 0.95, 0.35)
        return eid
    end
    return 0
end

local MAX_WARRIORS_ALIVE = 6

local function CountLivingOrcWarriors()
    local enemies = GetAllEnemies()
    if not enemies then return 0 end
    local count = 0
    for _, eid in ipairs(enemies) do
        if eid ~= entityID and eid ~= currentTotem.entity and IsAlive(eid) then
            count = count + 1
        end
    end
    return count
end

-- forced=true bypasses the cap (used for empowerment/totem-break spawns).
-- forced=false (default) caps periodic spawns at MAX_WARRIORS_ALIVE.
local function SpawnNOrcWarriors(count, forced)
    local alive = CountLivingOrcWarriors()
    if not forced then
        local canSpawn = MAX_WARRIORS_ALIVE - alive
        if canSpawn <= 0 then return end
        if count > canSpawn then count = canSpawn end
    end

    local spawned = 0
    for _ = 1, count do
        local tile = FindRandomFreeTileInArena()
        if tile then
            local eid = SpawnOrcWarriorAtTile(tile)
            if eid ~= 0 then
                spawned = spawned + 1
            end
        end
    end
    if spawned > 0 then
        Log("[Boss2] Spawned " .. spawned .. " Orc Warriors (" .. (alive + spawned) .. " alive)")
    end
end

local function SetEnemyWideBuffs()
    local enemies = GetAllEnemies()
    if not enemies then return end
    for _, eid in ipairs(enemies) do
        if IsAlive(eid) then
            local ap, maxAp = GetEntityAP(eid)
            if ap and maxAp then
                local newMax = maxAp + BOSS_CONFIG.totemicBonusAP
                if SetEntityMaxAP then
                    SetEntityMaxAP(eid, newMax)
                end
            end

            if SetSharedInt then
                SetSharedInt("orc_empowered_atk_" .. tostring(eid), BOSS_CONFIG.totemicBonusAttack)
                SetSharedInt("orc_empowered_mp_" .. tostring(eid), BOSS_CONFIG.totemicBonusMP)
            end

            local ex, ey = GetEntityGridPosition(eid)
            if ex then PulseTile(ex, ey, 0.35, 0.9, 0.5, 0.1) end
        end
    end
end

local function ActivateEmpoweredState()
    if empowered then return end
    empowered = true
    Log("[Boss2] ORC SHAMAN EMPOWERED")

    -- Play attack animation for empowerment cast
    ApplySheet("atkFront", false)

    SpawnNOrcWarriors(BOSS_CONFIG.extraOrcsOnEmpower, true)
    SetEnemyWideBuffs()
end

local function CaptureTrackedState()
    local newState = {}
    local players = GetAllPlayers() or {}
    local enemies = GetAllEnemies() or {}

    for _, eid in ipairs(players) do
        local gx, gy = GetEntityGridPosition(eid)
        local hp = GetEntityHPValue(eid)
        if gx and hp then
            newState[eid] = { hp = hp, x = gx, y = gy, isEnemy = false }
        end
    end

    for _, eid in ipairs(enemies) do
        local gx, gy = GetEntityGridPosition(eid)
        local hp = GetEntityHPValue(eid)
        if gx and hp then
            newState[eid] = { hp = hp, x = gx, y = gy, isEnemy = true }
        end
    end
    trackedEntityState = newState
end

local function ApplyFatalProtectionFromUnkilling()
    if currentTotem.type ~= TOTEM_TYPE.UNKILLING then return end
    if not IsAlive(currentTotem.entity) then return end

    local entities = {}
    local players = GetAllPlayers() or {}
    local enemies = GetAllEnemies() or {}
    for _, pid in ipairs(players) do table.insert(entities, pid) end
    for _, eid in ipairs(enemies) do table.insert(entities, eid) end

    for _, eid in ipairs(entities) do
        if IsEntityValid == nil or IsEntityValid(eid) then
            local hp = GetEntityHPValue(eid)
            if hp and hp <= 0 then
                local gx, gy = GetEntityGridPosition(eid)
                if gx and IsInsideTotemZone(currentTotem, gx, gy) then
                    local _, maxHP = GetEntityHP(eid)
                    if maxHP then
                        SetEntityHP(eid, 1, maxHP)
                    else
                        SetEntityHP(eid, 1, 1)
                    end
                    PulseTile(gx, gy, 0.25, 0.1, 1.0, 0.1)
                end
            end
        end
    end
end

local function CountEnemiesInMassacreZone()
    if currentTotem.type ~= TOTEM_TYPE.MASSACRE then return 0 end
    local count = 0
    for _, eid in ipairs(GetAllLivingEnemies()) do
        if eid ~= currentTotem.entity and eid ~= entityID then
            local gx, gy = GetEntityGridPosition(eid)
            if gx and IsInsideTotemZone(currentTotem, gx, gy) then
                count = count + 1
            end
        end
    end
    return count
end

local function HandleMassacreInvulnerability()
    if currentTotem.type ~= TOTEM_TYPE.MASSACRE then return end
    if not IsAlive(currentTotem.entity) then return end

    local hp = GetEntityHPValue(currentTotem.entity)
    if not hp then return end
    if currentTotem.lastHP == 0 then
        currentTotem.lastHP = hp
        return
    end

    local enemiesInZone = CountEnemiesInMassacreZone()
    if enemiesInZone > 0 and hp < currentTotem.lastHP then
        SetEntityHP(currentTotem.entity, currentTotem.lastHP, currentTotem.maxHP)
    else
        currentTotem.lastHP = hp
    end
end

local function ProcessMassacreEnemyDeaths(prevState)
    if currentTotem.type ~= TOTEM_TYPE.MASSACRE then return end
    if not IsAlive(currentTotem.entity) then return end

    local damageTicks = 0

    for eid, prev in pairs(prevState) do
        if prev.isEnemy and eid ~= currentTotem.entity and prev.hp and prev.hp > 0 then
            local nowAlive = IsAlive(eid)
            local nowHP = GetEntityHPValue(eid)
            local diedNow = (not nowAlive) or (nowHP and nowHP <= 0)
            if diedNow and IsInsideTotemZone(currentTotem, prev.x, prev.y) then
                damageTicks = damageTicks + 1
            end
        end
    end

    if damageTicks <= 0 then return end
    currentTotem.massacreDeathsCounted = currentTotem.massacreDeathsCounted + damageTicks
    local hp = GetEntityHPValue(currentTotem.entity)
    if hp and hp > 0 then
        local newHP = hp - damageTicks
        if newHP < 0 then newHP = 0 end
        SetEntityHP(currentTotem.entity, newHP, currentTotem.maxHP)
        currentTotem.lastHP = newHP
    end
end

local function CheckAllPlayersDead()
    local players = GetAllPlayers()
    if not players or #players == 0 then return true end
    for _, pid in ipairs(players) do
        local hp = GetEntityHPValue(pid)
        if hp and hp > 0 then
            return false
        end
    end
    return true
end

-- Queue any newly-dead players into the engine's deferred death system so the
-- PartyTurnManager skips them cleanly and entity IDs aren't recycled too early.
local function QueueDeadPlayerDeaths()
    local players = GetAllPlayers()
    if not players then return end
    for _, pid in ipairs(players) do
        local hp = GetEntityHPValue(pid)
        if hp and hp <= 0 then
            if DeferredDeathQueue then
                table.insert(DeferredDeathQueue, pid)
            end
        end
    end
end

local function HandleEndOfEnemyRoundEffects()
    if currentTotem.type ~= TOTEM_TYPE.BLIGHT then return end
    if not IsAlive(currentTotem.entity) then return end

    local entities = GetAllLivingCombatants()
    local insideCount = 0

    for _, eid in ipairs(entities) do
        local gx, gy = GetEntityGridPosition(eid)
        if gx then
            if eid ~= entityID then
                DamageEntity(eid, 1, currentTotem.entity)
            end
            if IsInsideTotemZone(currentTotem, gx, gy) then
                insideCount = insideCount + 1
            end
        end
    end

    if insideCount > 4 and ritesCountdown > 0 then
        ritesCountdown = ritesCountdown - 1
        Log("[Boss2] Totem of Blight accelerated rites countdown to " .. ritesCountdown)
        if ritesCountdown <= 0 then
            ActivateEmpoweredState()
        end
    end
end

local function ClearTotemParticles()
    for _, eid in ipairs(totemZoneEmitters) do
        if eid and eid ~= 0 then
            DestroyEntity(eid)
        end
    end
    totemZoneEmitters = {}
    if bossTotemEmitter and bossTotemEmitter ~= 0 then
        DestroyEntity(bossTotemEmitter)
        bossTotemEmitter = nil
    end
end

local function OnTotemDestroyed(totemType)
    if totemType ~= TOTEM_TYPE.UNKILLING and totemType ~= TOTEM_TYPE.MASSACRE and totemType ~= TOTEM_TYPE.BLIGHT then
        return
    end

    totemKilledInCycle[totemType] = true

    if totemType == TOTEM_TYPE.MASSACRE then
        for _, eid in ipairs(GetAllLivingCombatants()) do
            if eid ~= entityID then
                DamageEntity(eid, 3, entityID)
            end
        end
        QueueDeadPlayerDeaths()
    end

    ClearTotemParticles()

    currentTotem.type = TOTEM_TYPE.NONE
    currentTotem.entity = 0
    currentTotem.maxHP = 0
    currentTotem.lastHP = 0
    currentTotem.centerX = 0
    currentTotem.centerY = 0
    currentTotem.massacreDeathsCounted = 0

    if CheckAllPlayersDead() and SetNextGameState then
        SetNextGameState("LOSE_SCREEN")
        return
    end

    -- Per-totem-break: boss takes 2 damage and 2 warriors spawn (immune removed so damage lands)
    RemoveStatusEffect(entityID, "immune")
    if IsAlive(entityID) then
        DamageEntity(entityID, BOSS_CONFIG.cycleBacklashDamage, entityID)
    end
    SpawnNOrcWarriors(2, true)
    Log("[Boss2] Totem destroyed — boss takes " .. BOSS_CONFIG.cycleBacklashDamage .. " damage, 2 warriors spawned.")

    if totemKilledInCycle[TOTEM_TYPE.UNKILLING]
        and totemKilledInCycle[TOTEM_TYPE.MASSACRE]
        and totemKilledInCycle[TOTEM_TYPE.BLIGHT] then
        stunnedTurns = 1
        ritesCountdown = ritesCountdown + 1

        totemKilledInCycle[TOTEM_TYPE.UNKILLING] = false
        totemKilledInCycle[TOTEM_TYPE.MASSACRE] = false
        totemKilledInCycle[TOTEM_TYPE.BLIGHT] = false
        totemOrderIndex = 1
        Log("[Boss2] Totem cycle complete. Boss stunned, rites extended to " .. ritesCountdown)
    else
        totemOrderIndex = totemOrderIndex + 1
        if totemOrderIndex > 3 then totemOrderIndex = 1 end
    end
end

local function CheckTotemStateTransitions(prevState)
    if currentTotem.type == TOTEM_TYPE.NONE then return end
    if currentTotem.entity == 0 then return end

    if not IsEntityValid or not IsEntityValid(currentTotem.entity) then
        OnTotemDestroyed(currentTotem.type)
        return
    end

    ProcessMassacreEnemyDeaths(prevState)

    local hp = GetEntityHPValue(currentTotem.entity)
    if not hp or hp <= 0 then
        OnTotemDestroyed(currentTotem.type)
    else
        currentTotem.lastHP = hp
    end
end

local function FindTotemAnchorForType(totemType)
    if totemType == TOTEM_TYPE.UNKILLING then
        local enemies = GetAllLivingEnemies()
        local bx, by = GetEntityGridPosition(entityID)
        local bestID = 0
        local bestDist = 999999
        for _, eid in ipairs(enemies) do
            if eid ~= entityID then
                local ex, ey = GetEntityGridPosition(eid)
                if ex then
                    local d = math.abs(ex - bx) + math.abs(ey - by)
                    if d < bestDist then
                        bestDist = d
                        bestID = eid
                    end
                end
            end
        end
        if bestID ~= 0 then
            local x, y = GetEntityGridPosition(bestID)
            return x, y
        end
    elseif totemType == TOTEM_TYPE.MASSACRE then
        local target = FindClosestLivingPlayer()
        if target ~= 0 then
            local x, y = GetEntityGridPosition(target)
            return x, y
        end
    elseif totemType == TOTEM_TYPE.BLIGHT then
        return GetEntityGridPosition(entityID)
    end
    return nil, nil
end

local function SpawnTotemZoneParticles(totemType, centerTX, centerTY)
    local r, g, b
    if totemType == TOTEM_TYPE.UNKILLING then
        r, g, b = 0.2, 0.5, 1.0
    elseif totemType == TOTEM_TYPE.MASSACRE then
        r, g, b = 1.0, 0.2, 0.2
    else -- BLIGHT
        r, g, b = 0.2, 0.9, 0.3
    end
    local radius = BOSS_CONFIG.totemZoneRadius
    for ty = centerTY - radius, centerTY + radius do
        for tx = centerTX - radius, centerTX + radius do
            local wx, wy = TileToWorld(tx, ty)
            if wx and wy then
                local eid = SpawnParticleEmitterActive(wx, wy, 0.03, 4, 0, r, g, b, 0.7)
                if eid and eid ~= 0 then
                    table.insert(totemZoneEmitters, eid)
                end
            end
        end
    end
end

local function SpawnBossTotemParticle()
    local bx, by = GetEntityWorldPosition(entityID)
    if not bx then return end
    bossTotemEmitter = SpawnParticleEmitterActive(bx, by, 0.04, 6, 0, 0.6, 0.1, 0.9, 0.8, entityID)
end

local function SpawnTotem(totemType)
    if currentTotem.type ~= TOTEM_TYPE.NONE then return false end

    local ax, ay = FindTotemAnchorForType(totemType)
    if not ax then return false end

    local tx, ty = FindRandomAdjacentFreeTile(ax, ay)
    if not tx then return false end

    local wx, wy = TileToWorld(tx, ty)
    if not wx then return false end

    local totemID = SpawnEnemyAt(wx, wy)
    if not totemID or totemID == 0 then return false end

    -- Set totem texture immediately (before script attachment, so it's visible right away)
    local totemTextures = {
        [TOTEM_TYPE.UNKILLING] = "assets/enemy/totem_of_unkilling.png",
        [TOTEM_TYPE.MASSACRE]  = "assets/enemy/totem_of_massacre.png",
        [TOTEM_TYPE.BLIGHT]    = "assets/enemy/totem_of_blight.png"
    }
    local texPath = totemTextures[totemType]
    if texPath then
        if SetSpriteAnimationSheet then
            SetSpriteAnimationSheet(totemID, texPath, 1, 1, 1, 1.0, false)
        end
        if SetSpriteTexture then
            SetSpriteTexture(totemID, texPath)
        end
    end

    AddScriptComponentToEntity(totemID, "assets/scripts/Boss2TotemScript.lua")

    local hp = 3
    if totemType == TOTEM_TYPE.BLIGHT then
        hp = 5
    end

    SetEntityHP(totemID, hp, hp)
    if SetEntityMaxAP then SetEntityMaxAP(totemID, 0) end
    SetEnemyTarget(totemID, FindClosestLivingPlayer())

    SetSharedInt("boss2_totem_type_" .. tostring(math.floor(totemID)), totemType)

    currentTotem.type = totemType
    currentTotem.entity = totemID
    currentTotem.maxHP = hp
    currentTotem.lastHP = hp
    currentTotem.centerX = tx
    currentTotem.centerY = ty
    currentTotem.lifetimeRound = enemyRoundCount
    currentTotem.massacreDeathsCounted = 0

    local r, g, b = 0.2, 0.8, 0.2
    if totemType == TOTEM_TYPE.MASSACRE then
        r, g, b = 0.95, 0.2, 0.2
    elseif totemType == TOTEM_TYPE.BLIGHT then
        r, g, b = 0.5, 0.2, 0.8
    end
    PulseTile(tx, ty, 0.55, r, g, b)

    SpawnTotemZoneParticles(totemType, tx, ty)
    SpawnBossTotemParticle()
    return true
end

local function TryUseNextTotem()
    local order = {
        TOTEM_TYPE.UNKILLING,
        TOTEM_TYPE.MASSACRE,
        TOTEM_TYPE.BLIGHT
    }
    local idx = totemOrderIndex
    if idx < 1 or idx > 3 then idx = 1 end
    local t = order[idx]
    return SpawnTotem(t)
end

local function ApplyTotemicBlessing()
    local totemAlive = currentTotem.type ~= TOTEM_TYPE.NONE and IsAlive(currentTotem.entity)
    if totemAlive then
        ApplyStatusEffect(entityID, "immune", -1, entityID)
    else
        RemoveStatusEffect(entityID, "immune")
    end
end

local function DecrementRitesCountdownAtBossTurnStart()
    if not prepWasCast or empowered then return end
    if ritesCountdown <= 0 then return end
    ritesCountdown = ritesCountdown - 1
    if ritesCountdown <= 0 then
        ActivateEmpoweredState()
    end
end

local function StartBossTurn()
    isMyTurnToAct = true
    hasActedThisTurn = false
    movesThisTurn = 0
    currentPath = {}
    pathIndex = 1

    DecrementRitesCountdownAtBossTurnStart()
end

local function FinishBossTurn()
    if hasActedThisTurn then return end
    hasActedThisTurn = true
    isMyTurnToAct = false

    -- Return to idle facing closest player
    local target = FindClosestLivingPlayer()
    if target ~= 0 then
        local bx, by = GetEntityGridPosition(entityID)
        local px, py = GetEntityGridPosition(target)
        if bx and px then
            SetFacingFromDelta(px - bx, py - by, false)
        end
    end

    MarkEnemyActionComplete()
end

local function TryMoveTowardClosestPlayer()
    local mp = empowered and BOSS_CONFIG.moveMPEmpowered or BOSS_CONFIG.moveMPCountdown
    if mp <= 0 then return end
    if movesThisTurn >= BOSS_CONFIG.maxMovesPerTurn then return end

    local target = FindClosestLivingPlayer()
    if target == 0 then return end
    targetPlayerID = target

    local bx, by = GetEntityGridPosition(entityID)
    local px, py = GetEntityGridPosition(target)
    if not bx or not px then return end

    currentPath = FindPathToTarget(bx, by, px, py)
    if not currentPath or #currentPath == 0 then return end

    local nextTile = currentPath[1]
    if not nextTile then return end
    if not InArena(nextTile.x, nextTile.y) then return end
    if not IsWalkableTile(nextTile.x, nextTile.y) then return end
    if IsTileOccupied(nextTile.x, nextTile.y) then return end

    local sx, sy = GetEntityWorldPosition(entityID)
    local moved = MoveEntityToTile(entityID, nextTile.x, nextTile.y)
    if not moved then return end

    local wx, wy = GetEntityWorldPosition(entityID)
    if sx and sy and wx and wy then
        glideActive = true
        glideElapsed = 0.0
        glideStartX = sx
        glideStartY = sy
        glideEndX = wx
        glideEndY = wy
        SetSpritePosition(entityID, sx, sy)
    end

    SetFacingFromDelta(nextTile.x - bx, nextTile.y - by, true)
    ConsumeEnemyAP(entityID, BOSS_CONFIG.apCostPerMove)
    ShowTileBorder(nextTile.x, nextTile.y, 0.25)
    PulseTile(nextTile.x, nextTile.y, 0.2, 0.9, 0.45, 0.1)
    movesThisTurn = movesThisTurn + 1
end

local function BossDecision()
    if not prepWasCast then
        prepWasCast = true
        ritesCountdown = BOSS_CONFIG.prepCountdownStart
        -- Play attack animation for casting Preparatory Rites
        ApplySheet("atkFront", false)
        Log("[Boss2] Preparatory Rites cast. Countdown: " .. ritesCountdown)
        FinishBossTurn()
        return
    end

    if stunnedTurns > 0 then
        stunnedTurns = stunnedTurns - 1
        Log("[Boss2] Boss stunned. Remaining stunned turns: " .. stunnedTurns)
        FinishBossTurn()
        return
    end

    if currentTotem.type ~= TOTEM_TYPE.NONE and IsAlive(currentTotem.entity) then
        -- Empowered boss moves toward players even while totem is active
        if empowered then
            TryMoveTowardClosestPlayer()
            if glideActive then
                pendingFinishAfterGlide = true
                return
            end
        end
        -- Face toward active totem while waiting
        local bx, by = GetEntityGridPosition(entityID)
        if bx and currentTotem.centerX then
            SetFacingFromDelta(currentTotem.centerX - bx, currentTotem.centerY - by, false)
        end
        FinishBossTurn()
        return
    end

    if TryUseNextTotem() then
        -- Play attack animation facing toward spawned totem
        local bx, by = GetEntityGridPosition(entityID)
        if bx and currentTotem.centerX then
            SetAttackFacing(bx, by, currentTotem.centerX, currentTotem.centerY)
        end
        FinishBossTurn()
        return
    end

    -- Empowered boss moves when no totem to spawn
    if empowered then
        TryMoveTowardClosestPlayer()
    end

    if glideActive then
        pendingFinishAfterGlide = true
        return
    end

    FinishBossTurn()
end

function OnInit()
    entityID = self
    if not entityID or entityID == 0 then return end

    ApplyBossVisuals()

    local aMinX = GetSharedInt("arenaMinX", -1)
    local aMinY = GetSharedInt("arenaMinY", -1)
    local aMaxX = GetSharedInt("arenaMaxX", -1)
    local aMaxY = GetSharedInt("arenaMaxY", -1)
    if aMinX >= 0 and aMinY >= 0 and aMaxX > aMinX and aMaxY > aMinY then
        localArenaBounds = {
            minX = aMinX,
            minY = aMinY,
            maxX = aMaxX,
            maxY = aMaxY
        }
    end

    SetEntityHP(entityID, BOSS_CONFIG.maxHP, BOSS_CONFIG.maxHP)
    if SetEntityMaxAP then SetEntityMaxAP(entityID, 3) end

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
            SetSpriteColor(healthBarFG, 0.85, 0.0, 0.0, 1.0)
        end
    end

    CaptureTrackedState()
    Log("[Boss2] Orc Shaman initialized (entity " .. entityID .. ")")
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

    ClearTotemParticles()

    if currentTotem.entity and currentTotem.entity ~= 0 and IsEntityValid and IsEntityValid(currentTotem.entity) then
        DestroyEntity(currentTotem.entity)
    end
end

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

    local r, g, b = 0.85, 0.0, 0.0
    if ratio <= 0.25 then
        r, g, b = 1.0, 0.0, 0.0
    elseif ratio <= 0.5 then
        r, g, b = 0.9, 0.15, 0.0
    end
    SetSpriteColor(healthBarFG, r, g, b, 1.0)
end

function OnUpdate(dt)
    UpdateHealthBar()

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
            if pendingFinishAfterGlide then
                pendingFinishAfterGlide = false
                if GetCurrentTurn() == "Enemy" then
                    FinishBossTurn()
                end
                return
            end
        end
        if glideActive then return end
    end

    local prevTracked = trackedEntityState

    if moveTimer > 0 then
        moveTimer = moveTimer - dt
        if moveTimer < 0 then moveTimer = 0 end
    end

    if not bossActivated then
        bossActivated = true
        Log("[Boss2] Orc Shaman activated.")
        SpawnNOrcWarriors(4, true)
        local currentTurnNow = GetCurrentTurn()
        if currentTurnNow == "Enemy" and IsActiveEnemy(entityID) and IsEnemyActionReady() then
            FinishBossTurn()
        end
        CaptureTrackedState()
        return
    end

    local turn = GetCurrentTurn()
    if lastKnownTurn == "Enemy" and turn ~= "Enemy" then
        lastKnownTurn = turn
        enemyRoundCount = enemyRoundCount + 1
        HandleEndOfEnemyRoundEffects()
        QueueDeadPlayerDeaths()
        if CheckAllPlayersDead() and SetNextGameState then
            SetNextGameState("LOSE_SCREEN")
            return
        end
        if enemyRoundCount - bonusSpawnLastRound >= BOSS_CONFIG.periodicSpawnEveryEnemyRounds then
            bonusSpawnLastRound = enemyRoundCount
            SpawnNOrcWarriors(1)
        end
    else
        lastKnownTurn = turn
    end

    -- These run every frame: safe status/HP corrections with no damage side-effects
    ApplyFatalProtectionFromUnkilling()
    HandleMassacreInvulnerability()
    ApplyTotemicBlessing()

    -- Detect totem death every frame so particles are cleaned up immediately
    if currentTotem.type ~= TOTEM_TYPE.NONE and currentTotem.entity ~= 0 then
        local totemGone = false
        if IsEntityValid and not IsEntityValid(currentTotem.entity) then
            totemGone = true
        elseif GetEntityHPValue then
            local hp = GetEntityHPValue(currentTotem.entity)
            if hp and hp <= 0 then totemGone = true end
        end
        if totemGone then
            OnTotemDestroyed(currentTotem.type)
        end
    end

    if turn ~= "Enemy" then
        if lastEnemyTurn == "Enemy" then
            hasActedThisTurn = false
            isMyTurnToAct = false
            moveTimer = 0.0
            movesThisTurn = 0
            pendingFinishAfterGlide = false
            if glideActive then
                SetSpritePosition(entityID, glideEndX, glideEndY)
                glideActive = false
            end
        end
        lastEnemyTurn = turn
        CaptureTrackedState()
        return
    end
    lastEnemyTurn = "Enemy"

    -- Only during enemy turn: can trigger OnTotemDestroyed which deals AoE damage
    CheckTotemStateTransitions(prevTracked)

    if not IsActiveEnemy(entityID) then
        CaptureTrackedState()
        return
    end

    if not IsEnemyActionReady() then
        CaptureTrackedState()
        return
    end

    if not isMyTurnToAct then
        StartBossTurn()
    end

    if hasActedThisTurn then
        CaptureTrackedState()
        return
    end

    BossDecision()
    CaptureTrackedState()
end
