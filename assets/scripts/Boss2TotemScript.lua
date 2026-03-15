-- Boss 2 helper script: passive totem actor.
-- Totem behavior is orchestrated by Boss2OrcShaman.lua.

local entityID = 0
local hasActedThisTurn = false
local lastTurn = nil

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
    SetEntityAP(entityID, 0, 0)
end

function OnUpdate(_dt)
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
