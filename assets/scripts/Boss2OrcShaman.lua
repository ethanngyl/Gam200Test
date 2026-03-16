--[[
===============================================================================
 File:          Boss2OrcShaman.lua
 Authors:       GitHub Copilot
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

-- SpawnEnemyAt starts with a default enemy look; force boss visuals on attach.
local BOSS_VISUAL = {
    tex = "assets/Enemy/Boss1_Idle_Front-Sheet.png",
    rows = 4,
    cols = 3,
    frames = 12,
    time = 0.08,
    loop = true
}

local function ApplyBossVisuals()
    if not entityID or entityID == 0 then return end
    if not SetSpriteAnimationSheet then return end
    SetSpriteAnimationSheet(
        entityID,
        BOSS_VISUAL.tex,
        BOSS_VISUAL.rows,
        BOSS_VISUAL.cols,
        BOSS_VISUAL.frames,
        BOSS_VISUAL.time,
        BOSS_VISUAL.loop
    )
    if SetAnimationFlipX then
        SetAnimationFlipX(entityID, false)
    end
end

local moveTimer = 0.0
local moveDelay = 0.45
local movesThisTurn = 0
local currentPath = {}
local pathIndex = 1
local targetPlayerID = 0

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

local function AreAllPlayersInArena()
    local players = GetAllPlayers()
    if not players or #players == 0 then return false end

    local aliveCount = 0
    local inCount = 0
    for _, pid in ipairs(players) do
        if IsAlive(pid) then
            aliveCount = aliveCount + 1
            local px, py = GetEntityGridPosition(pid)
            if px and InArena(px, py) then
                inCount = inCount + 1
            end
        end
    end
    return aliveCount > 0 and aliveCount == inCount
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
        if localArenaBounds and not InArena(tx, ty) then
            goto continue
        end
        if IsWalkableTile(tx, ty) and not IsTileOccupied(tx, ty) then
            return tx, ty
        end
        ::continue::
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
        PulseTile(tile.x, tile.y, 0.25, 0.45, 0.95, 0.35)
        return eid
    end
    return 0
end

local function SpawnNOrcWarriors(count)
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
        Log("[Boss2] Spawned " .. spawned .. " Orc Warriors")
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
                SetEntityAP(eid, math.min(ap + BOSS_CONFIG.totemicBonusAP, newMax), newMax)
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

    SpawnNOrcWarriors(BOSS_CONFIG.extraOrcsOnEmpower)
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
                    SetEntityHP(eid, 1)
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
        if eid ~= currentTotem.entity then
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

local function HandleEndOfEnemyRoundEffects()
    if currentTotem.type ~= TOTEM_TYPE.BLIGHT then return end
    if not IsAlive(currentTotem.entity) then return end

    local entities = GetAllLivingCombatants()
    local insideCount = 0

    for _, eid in ipairs(entities) do
        local gx, gy = GetEntityGridPosition(eid)
        if gx then
            DamageEntity(eid, 1, currentTotem.entity)
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

local function OnTotemDestroyed(totemType)
    if totemType ~= TOTEM_TYPE.UNKILLING and totemType ~= TOTEM_TYPE.MASSACRE and totemType ~= TOTEM_TYPE.BLIGHT then
        return
    end

    totemKilledInCycle[totemType] = true

    if totemType == TOTEM_TYPE.MASSACRE then
        for _, eid in ipairs(GetAllLivingCombatants()) do
            DamageEntity(eid, 3, entityID)
        end
    end

    currentTotem.type = TOTEM_TYPE.NONE
    currentTotem.entity = 0
    currentTotem.maxHP = 0
    currentTotem.lastHP = 0
    currentTotem.centerX = 0
    currentTotem.centerY = 0
    currentTotem.massacreDeathsCounted = 0

    if totemKilledInCycle[TOTEM_TYPE.UNKILLING]
        and totemKilledInCycle[TOTEM_TYPE.MASSACRE]
        and totemKilledInCycle[TOTEM_TYPE.BLIGHT] then
        stunnedTurns = 1
        ritesCountdown = ritesCountdown + 1
        DamageEntity(entityID, BOSS_CONFIG.cycleBacklashDamage, entityID)

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

    AddScriptComponentToEntity(totemID, "assets/scripts/Boss2TotemScript.lua")

    local hp = 3
    if totemType == TOTEM_TYPE.BLIGHT then
        hp = 5
    end

    SetEntityHP(totemID, hp, hp)
    if SetEntityMaxAP then SetEntityMaxAP(totemID, 0) end
    SetEntityAP(totemID, 0, 0)
    SetEnemyTarget(totemID, FindClosestLivingPlayer())

    SetSharedInt("boss2_totem_type_" .. tostring(totemID), totemType)

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
    MarkEnemyActionComplete()
end

local function TryMoveTowardClosestPlayer()
    local mp = empowered and BOSS_CONFIG.moveMPEmpowered or BOSS_CONFIG.moveMPCountdown
    if mp <= 0 then return end

    local target = FindClosestLivingPlayer()
    if target == 0 then return end
    targetPlayerID = target

    local bx, by = GetEntityGridPosition(entityID)
    local px, py = GetEntityGridPosition(target)
    if not bx or not px then return end

    currentPath = FindPathToTarget(bx, by, px, py)
    pathIndex = 1

    while mp > 0 and movesThisTurn < BOSS_CONFIG.maxMovesPerTurn and currentPath and pathIndex <= #currentPath do
        local nextTile = currentPath[pathIndex]
        if not nextTile then break end
        if not InArena(nextTile.x, nextTile.y) then break end
        if not IsWalkableTile(nextTile.x, nextTile.y) then break end
        if IsTileOccupied(nextTile.x, nextTile.y) then break end
        if moveTimer > 0 then break end

        local moved = MoveEntityToTile(entityID, nextTile.x, nextTile.y)
        if not moved then break end

        ConsumeEnemyAP(entityID, BOSS_CONFIG.apCostPerMove)
        ShowTileBorder(nextTile.x, nextTile.y, 0.25)
        PulseTile(nextTile.x, nextTile.y, 0.2, 0.9, 0.45, 0.1)

        mp = mp - 1
        movesThisTurn = movesThisTurn + 1
        pathIndex = pathIndex + 1
        moveTimer = moveDelay
    end
end

local function BossDecision()
    if not prepWasCast then
        prepWasCast = true
        ritesCountdown = BOSS_CONFIG.prepCountdownStart
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
        FinishBossTurn()
        return
    end

    if TryUseNextTotem() then
        FinishBossTurn()
        return
    end

    if empowered then
        TryMoveTowardClosestPlayer()
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
    SetEntityAP(entityID, 3, 3)

    if ApplyStatusEffect then
        ApplyStatusEffect(entityID, "immune", -1, entityID)
    end

    CaptureTrackedState()
    Log("[Boss2] Orc Shaman initialized")
end

function OnDestroy()
    if currentTotem.entity and currentTotem.entity ~= 0 and IsEntityValid and IsEntityValid(currentTotem.entity) then
        DestroyEntity(currentTotem.entity)
    end
end

function OnUpdate(dt)
    local prevTracked = trackedEntityState

    if moveTimer > 0 then
        moveTimer = moveTimer - dt
        if moveTimer < 0 then moveTimer = 0 end
    end

    if not bossActivated then
        if AreAllPlayersInArena() then
            bossActivated = true
            RemoveStatusEffect(entityID, "immune")
            SpawnNOrcWarriors(4)
            Log("[Boss2] Arena condition met. Orc Shaman activated.")
        else
            if GetCurrentTurn() == "Enemy" and IsActiveEnemy(entityID) and IsEnemyActionReady() then
                FinishBossTurn()
            end
            CaptureTrackedState()
            return
        end
    end

    local turn = GetCurrentTurn()
    if lastKnownTurn == "Enemy" and turn ~= "Enemy" then
        enemyRoundCount = enemyRoundCount + 1
        HandleEndOfEnemyRoundEffects()
        if enemyRoundCount - bonusSpawnLastRound >= BOSS_CONFIG.periodicSpawnEveryEnemyRounds then
            bonusSpawnLastRound = enemyRoundCount
            SpawnNOrcWarriors(1)
        end
    end
    lastKnownTurn = turn

    CheckTotemStateTransitions(prevTracked)
    ApplyFatalProtectionFromUnkilling()
    HandleMassacreInvulnerability()
    ApplyTotemicBlessing()

    if turn ~= "Enemy" then
        if lastEnemyTurn == "Enemy" then
            hasActedThisTurn = false
            isMyTurnToAct = false
            moveTimer = 0.0
            movesThisTurn = 0
        end
        lastEnemyTurn = turn
        CaptureTrackedState()
        return
    end
    lastEnemyTurn = "Enemy"

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
