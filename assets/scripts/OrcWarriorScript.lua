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

local healthBarBG = nil
local healthBarFG = nil
local healthBarWidth = 0.1
local healthBarHeight = 0.015
local healthBarOffsetY = 0.08
local healthBarLayer = 2

local glideActive = false
local glideElapsed = 0.0
local glideDuration = 0.35
local glideStartX = 0.0
local glideStartY = 0.0
local glideEndX = 0.0
local glideEndY = 0.0
local pendingFinishAfterGlide = false

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

    local sx, sy = GetEntityWorldPosition(entityID)
    local moved = MoveEntityToTile(entityID, nextTile.x, nextTile.y)
    if not moved then return false end

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

    ConsumeEnemyAP(entityID, CONFIG.moveCost)
    mpRemaining = mpRemaining - 1

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

    local r, g, b = 0.0, 0.85, 0.0
    if ratio <= 0.25 then
        r, g, b = 0.9, 0.1, 0.1
    elseif ratio <= 0.5 then
        r, g, b = 0.85, 0.75, 0.0
    end
    SetSpriteColor(healthBarFG, r, g, b, 1.0)
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

    targetPlayerID = FindClosestPlayer()
end

function OnDestroy()
    if healthBarBG and healthBarBG > 0 then
        DestroyEntity(healthBarBG)
        healthBarBG = nil
    end
    if healthBarFG and healthBarFG > 0 then
        DestroyEntity(healthBarFG)
        healthBarFG = nil
    end
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
        end
        if glideActive then return end
    end

    if moveTimer > 0 then
        moveTimer = moveTimer - dt
        if moveTimer < 0 then moveTimer = 0 end
    end

    local turn = GetCurrentTurn()
    if turn ~= "Enemy" then
        if lastEnemyTurn == "Enemy" then
            hasActedThisTurn = false
            isMyTurnToAct = false
            if glideActive then
                SetSpritePosition(entityID, glideEndX, glideEndY)
                glideActive = false
            end
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
