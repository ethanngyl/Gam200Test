-- Boss 2 helper script: passive totem actor.
-- Totem behavior is orchestrated by Boss2OrcShaman.lua.

local entityID = 0
local hasActedThisTurn = false
local lastTurn = nil

local healthBarBG = nil
local healthBarFG = nil
local healthBarWidth = 0.09
local healthBarHeight = 0.015
local healthBarOffsetY = 0.08
local healthBarLayer = 2

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

    SetSpriteColor(healthBarFG, 0.95, 0.5, 0.0, 1.0)
end

function OnInit()
    entityID = self
    if not entityID or entityID == 0 then return end

    local totemType = 0
    if GetSharedInt then
        totemType = GetSharedInt("boss2_totem_type_" .. tostring(entityID), 0)
    end

    if totemType == 3 then
        SetEntityHP(entityID, 5, 5)
    else
        SetEntityHP(entityID, 3, 3)
    end

    if SetEntityMaxAP then SetEntityMaxAP(entityID, 0) end

    -- Set totem texture based on type
    local totemTextures = {
        [1] = "assets/enemy/totem_of_unkilling.png",
        [2] = "assets/enemy/totem_of_massacre.png",
        [3] = "assets/enemy/totem_of_blight.png"
    }
    local texPath = totemTextures[totemType]
    if texPath then
        -- Use a 1-frame animation sheet to override the default enemy sprite.
        -- SetSpriteTexture alone is overridden by the SpriteAnimation component each frame.
        if SetSpriteAnimationSheet then
            SetSpriteAnimationSheet(entityID, texPath, 1, 1, 1, 1.0, false)
        end
        if SetSpriteTexture then
            SetSpriteTexture(entityID, texPath)
        end
    end

    local wx, wy = GetEntityWorldPosition(entityID)
    if wx and wy then
        local barY = wy + healthBarOffsetY
        healthBarBG = SpawnSprite("", wx, barY, healthBarWidth, healthBarHeight, healthBarLayer)
        if healthBarBG and healthBarBG > 0 then
            SetSpriteColor(healthBarBG, 0.15, 0.15, 0.15, 0.85)
        end
        healthBarFG = SpawnSprite("", wx, barY, healthBarWidth, healthBarHeight, healthBarLayer + 1)
        if healthBarFG and healthBarFG > 0 then
            SetSpriteColor(healthBarFG, 0.95, 0.5, 0.0, 1.0)
        end
    end
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

function OnUpdate(_dt)
    UpdateHealthBar()

    local turn = GetCurrentTurn()

    if turn ~= "Enemy" then
        hasActedThisTurn = false
        lastTurn = turn
        return
    end

    if not IsActiveEnemy(entityID) then
        lastTurn = turn
        return
    end

    if not IsEnemyActionReady() then
        lastTurn = turn
        return
    end

    if hasActedThisTurn then
        lastTurn = turn
        return
    end

    hasActedThisTurn = true
    MarkEnemyActionComplete()
    lastTurn = turn
end
