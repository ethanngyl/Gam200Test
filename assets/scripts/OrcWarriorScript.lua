--[[
 Orc Warrior
 - 6 HP, 2 MP, 2 AP
 - Strong Swing: 2 AP, self-damage 2 HP, deal 2 damage in a 3-tile sweep.
 - Regenerate: 2 AP, heal 1 HP, cooldown 2 turns, used at HP <= 3.
]]--

local entityID = 0
local targetPlayerID = 0

local hasActedThisTurn = false
local isMyTurnToAct = false
local lastEnemyTurn = nil

local moveTimer = 0.0
local moveDelay = 0.45
local mpRemaining = 0

local regenCooldown = 0
local facingDX = 0
local facingDY = -1

local CONFIG = {
    maxHP = 6,
    maxAP = 2,
    moveMP = 2,
    moveCost = 1,
    regenAmount = 1,
    regenThreshold = 3,
    regenCooldown = 2,
    strongSwingDamage = 2,
    strongSwingSelfDamage = 2,
    strongSwingAPCost = 2
}

local function IsAlive(eid)
    if not eid or eid == 0 then return false end
    if IsEntityValid and not IsEntityValid(eid) then return false end
    local hp = GetEntityHP(eid)
    return hp and hp > 0
end

local function FindClosestPlayer()
    local ex, ey = GetEntityGridPosition(entityID)
    if not ex then return 0 end

    local bestID = 0
    local bestDist = 99999
    local players = GetAllPlayers()
    if not players then return 0 end

    for _, pid in ipairs(players) do
        if IsAlive(pid) then
            local px, py = GetEntityGridPosition(pid)
            if px then
                local d = math.abs(px - ex) + math.abs(py - ey)
                if d < bestDist then
                    bestDist = d
                    bestID = pid
                end
            end
        end
    end

    return bestID
end

local function UpdateFacingToward(tx, ty)
    local ex, ey = GetEntityGridPosition(entityID)
    if not ex then return end
    local dx = tx - ex
    local dy = ty - ey
    if math.abs(dx) > math.abs(dy) then
        facingDX = dx > 0 and 1 or -1
        facingDY = 0
    else
        facingDX = 0
        facingDY = dy > 0 and 1 or -1
    end
end

local function StrongSwingTiles(ex, ey)
    -- 3-tile sweep perpendicular to current facing direction.
    local tiles = {}
    if facingDY ~= 0 then
        table.insert(tiles, { x = ex - 1, y = ey + facingDY })
        table.insert(tiles, { x = ex,     y = ey + facingDY })
        table.insert(tiles, { x = ex + 1, y = ey + facingDY })
    else
        table.insert(tiles, { x = ex + facingDX, y = ey - 1 })
        table.insert(tiles, { x = ex + facingDX, y = ey })
        table.insert(tiles, { x = ex + facingDX, y = ey + 1 })
    end
    return tiles
end

local function CanUseStrongSwingOnAnyPlayer()
    local ex, ey = GetEntityGridPosition(entityID)
    if not ex then return false end
    local tiles = StrongSwingTiles(ex, ey)
    local players = GetAllPlayers()
    if not players then return false end

    for _, pid in ipairs(players) do
        if IsAlive(pid) then
            local px, py = GetEntityGridPosition(pid)
            if px then
                for _, t in ipairs(tiles) do
                    if t.x == px and t.y == py then
                        return true
                    end
                end
            end
        end
    end
    return false
end

local function UseRegenerate()
    local hp, maxHP = GetEntityHP(entityID)
    if not hp or not maxHP then return false end
    local ap = GetEntityAP(entityID)
    if not ap or ap < 2 then return false end
    if regenCooldown > 0 then return false end
    if hp > CONFIG.regenThreshold then return false end

    ConsumeEnemyAP(entityID, 2)
    SetEntityHP(entityID, math.min(hp + CONFIG.regenAmount, maxHP), maxHP)
    regenCooldown = CONFIG.regenCooldown

    local ex, ey = GetEntityGridPosition(entityID)
    if ex then PulseTile(ex, ey, 0.3, 0.2, 0.9, 0.2) end
    return true
end

local function UseStrongSwing()
    local ap = GetEntityAP(entityID)
    if not ap or ap < CONFIG.strongSwingAPCost then return false end

    local ex, ey = GetEntityGridPosition(entityID)
    if not ex then return false end

    local tiles = StrongSwingTiles(ex, ey)
    local players = GetAllPlayers()
    if not players then return false end

    local hitAny = false
    for _, pid in ipairs(players) do
        if IsAlive(pid) then
            local px, py = GetEntityGridPosition(pid)
            if px then
                for _, t in ipairs(tiles) do
                    if t.x == px and t.y == py then
                        DamageEntity(pid, CONFIG.strongSwingDamage, entityID)
                        hitAny = true
                    end
                end
            end
        end
    end

    if not hitAny then
        return false
    end

    ConsumeEnemyAP(entityID, CONFIG.strongSwingAPCost)
    DamageEntity(entityID, CONFIG.strongSwingSelfDamage, entityID)

    for _, t in ipairs(tiles) do
        PulseTile(t.x, t.y, 0.28, 0.95, 0.25, 0.1)
    end
    return true
end

local function TryMoveTowardTarget()
    if mpRemaining <= 0 then return false end

    local ex, ey = GetEntityGridPosition(entityID)
    local px, py = GetEntityGridPosition(targetPlayerID)
    if not ex or not px then return false end

    local path = FindPathToTarget(ex, ey, px, py)
    if not path or #path == 0 then return false end

    local nextTile = path[1]
    if not nextTile then return false end
    if not IsWalkableTile(nextTile.x, nextTile.y) then return false end
    if IsTileOccupied(nextTile.x, nextTile.y) then return false end

    local moved = MoveEntityToTile(entityID, nextTile.x, nextTile.y)
    if not moved then return false end

    ConsumeEnemyAP(entityID, CONFIG.moveCost)
    mpRemaining = mpRemaining - 1
    moveTimer = moveDelay

    UpdateFacingToward(px, py)
    ShowTileBorder(nextTile.x, nextTile.y, 0.25)
    PulseTile(nextTile.x, nextTile.y, 0.2, 0.9, 0.5, 0.2)
    return true
end

local function StartTurn()
    isMyTurnToAct = true
    hasActedThisTurn = false
    mpRemaining = CONFIG.moveMP

    if regenCooldown > 0 then
        regenCooldown = regenCooldown - 1
    end

    targetPlayerID = FindClosestPlayer()
end

local function FinishTurn()
    if hasActedThisTurn then return end
    hasActedThisTurn = true
    isMyTurnToAct = false
    MarkEnemyActionComplete()
end

function OnInit()
    entityID = self
    if not entityID or entityID == 0 then return end

    SetEntityHP(entityID, CONFIG.maxHP, CONFIG.maxHP)
    if SetEntityMaxAP then SetEntityMaxAP(entityID, CONFIG.maxAP) end
    SetEntityAP(entityID, CONFIG.maxAP, CONFIG.maxAP)

    targetPlayerID = FindClosestPlayer()
end

function OnUpdate(dt)
    if moveTimer > 0 then
        moveTimer = moveTimer - dt
        if moveTimer < 0 then moveTimer = 0 end
    end

    local turn = GetCurrentTurn()
    if turn ~= "Enemy" then
        if lastEnemyTurn == "Enemy" then
            hasActedThisTurn = false
            isMyTurnToAct = false
        end
        lastEnemyTurn = turn
        return
    end
    lastEnemyTurn = "Enemy"

    if not IsActiveEnemy(entityID) then return end
    if not IsEnemyActionReady() then return end

    if not isMyTurnToAct then
        StartTurn()
    end
    if hasActedThisTurn then return end

    if not targetPlayerID or targetPlayerID == 0 or not IsAlive(targetPlayerID) then
        targetPlayerID = FindClosestPlayer()
    end
    if not targetPlayerID or targetPlayerID == 0 then
        FinishTurn()
        return
    end

    local tx, ty = GetEntityGridPosition(targetPlayerID)
    if tx then
        UpdateFacingToward(tx, ty)
    end

    if UseRegenerate() then
        FinishTurn()
        return
    end

    if CanUseStrongSwingOnAnyPlayer() and UseStrongSwing() then
        FinishTurn()
        return
    end

    if moveTimer <= 0 then
        TryMoveTowardTarget()
    end

    if CanUseStrongSwingOnAnyPlayer() and UseStrongSwing() then
        FinishTurn()
        return
    end

    FinishTurn()
end
