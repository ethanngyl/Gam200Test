--[[
===============================================================================
File:        TurnOrderUI.lua
Date:        2026-03-12
-------------------------------------------------------------------------------

TURN ORDER UI - Displays party turn order with active character highlight

Brief:
   Shows animated character portraits in a row with arrow icons between them.
   Slots 1-3 are the three party members; slot 4 is a shared enemy portrait.
   The active portrait is shown at full brightness and scaled up;
   inactive portraits are dimmed and at normal size.
   Inherits from UIComponent for camera-relative positioning.

Usage:
   Managed by UIManager — Init/Update/Destroy are called automatically.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
===============================================================================
]]--

local UIComponent = require("UI/UIComponent")

local TurnOrderUI = {}
setmetatable(TurnOrderUI, {__index = UIComponent})
TurnOrderUI.__index = TurnOrderUI

function TurnOrderUI:New()
    local instance = UIComponent:New()
    setmetatable(instance, self)
    return instance
end

function TurnOrderUI:Init(config)
    config = config or {}

    self.offsetX = config.offsetX or 0.0
    self.offsetY = config.offsetY or 0.35
    self.layer = config.layer or 8

    self.portraitSize = config.portraitSize or 0.08
    self.arrowSize = config.arrowSize or 0.04
    self.spacing = config.spacing or 0.14

    self.charConfigs = config.charConfigs or {
        {
            texture = "assets/Warrior/FrontView/WarriorTopDownView.png",
            rows = 1, columns = 12, frameCount = 12, frameTime = 0.55
        },
        {
            texture = "assets/Mage/FrontView/Mage_Idle_Front-Sheet.png",
            rows = 1, columns = 12, frameCount = 12, frameTime = 0.55
        },
        {
            texture = "assets/Berserker/FrontView/Berserker_Idle_Front-Sheet.png",
            rows = 1, columns = 12, frameCount = 6, frameTime = 0.55
        }
    }

    self.enemyConfig = config.enemyConfig or {
        texture = "assets/Enemy/Enemy_Knight_Idle_Front-Sheet.png",
        rows = 1, columns = 12, frameCount = 12, frameTime = 0.08
    }

    self.arrowTexture = config.arrowTexture or "assets/UI/AP_Crystal(old).png"

    -- Background panel (rotated 90° from vertical to horizontal)
    self.bgTexture = config.bgTexture or "assets/new assets/skill_bubble_holder.png"
    self.bgScaleX = config.bgScaleX or 0.15
    self.bgScaleY = config.bgScaleY or 0.55
    self.bgRotation = config.bgRotation or 90
    self.bgOffsetY = config.bgOffsetY or 0.0

    -- Active = full brightness + scaled up; Inactive = dimmed + normal size
    self.activeTint = config.activeTint or { r = 1.0, g = 1.0, b = 1.0, a = 1.0 }
    self.inactiveTint = config.inactiveTint or { r = 0.35, g = 0.35, b = 0.35, a = 1.0 }
    self.activeScale = config.activeScale or 1.3

    -- Total slot count = party members + 1 enemy
    self.totalSlots = #self.charConfigs + 1
    self.enemySlot = self.totalSlots

    self.bgID = 0
    self.portraitIDs = {}
    self.arrowIDs = {}
    self.lastHighlight = ""

    local camX, camY = GetCameraPosition()
    self:CreateSprites(camX, camY)
    self:UpdateHighlight()
end

-- ============================================================================
-- SPRITE CREATION
-- ============================================================================

function TurnOrderUI:GetSlotOffsetX(slotIndex)
    return (slotIndex - (self.totalSlots + 1) * 0.5) * self.spacing
end

function TurnOrderUI:CreateSprites(camX, camY)
    local baseX = camX + self.offsetX
    local baseY = camY + self.offsetY

    -- Spawn background panel (one layer behind portraits, rotated 90°)
    local bgID = SpawnSprite(
        self.bgTexture,
        baseX, baseY + self.bgOffsetY,
        self.bgScaleX, self.bgScaleY,
        self.layer - 1,
        self.bgRotation
    )
    if bgID and bgID > 0 then
        self.bgID = bgID
        table.insert(self.entities, bgID)
        if SetSpriteFilterMode then
            SetSpriteFilterMode(bgID, true)
        end
    end

    -- Spawn party portraits (slots 1..N-1)
    for i = 1, #self.charConfigs do
        local cfg = self.charConfigs[i]
        local id = self:SpawnAnimatedSprite(
            cfg.texture,
            baseX + self:GetSlotOffsetX(i), baseY,
            self.portraitSize, self.portraitSize,
            self.layer,
            cfg.rows, cfg.columns, cfg.frameCount, cfg.frameTime, true
        )
        self.portraitIDs[i] = id
    end

    -- Spawn enemy portrait (last slot)
    local eCfg = self.enemyConfig
    local eID = self:SpawnAnimatedSprite(
        eCfg.texture,
        baseX + self:GetSlotOffsetX(self.enemySlot), baseY,
        self.portraitSize, self.portraitSize,
        self.layer,
        eCfg.rows, eCfg.columns, eCfg.frameCount, eCfg.frameTime, true
    )
    self.portraitIDs[self.enemySlot] = eID

    -- Spawn arrows between each pair
    for i = 1, self.totalSlots - 1 do
        local arrowX = baseX + (self:GetSlotOffsetX(i) + self:GetSlotOffsetX(i + 1)) * 0.5
        local arrowID = self:SpawnSprite(
            self.arrowTexture,
            arrowX, baseY,
            self.arrowSize, self.arrowSize,
            self.layer
        )
        self.arrowIDs[i] = arrowID
    end
end

-- ============================================================================
-- TINT UPDATE
-- ============================================================================

function TurnOrderUI:UpdateHighlight()
    local currentTurn = GetCurrentTurn and GetCurrentTurn() or "Player"
    local isEnemyTurn = (currentTurn == "Enemy")

    local highlightSlot = 0
    if isEnemyTurn then
        highlightSlot = self.enemySlot
    elseif ActiveCharacterIndex then
        highlightSlot = ActiveCharacterIndex
    end

    local key = tostring(highlightSlot)
    if key == self.lastHighlight then
        return
    end
    self.lastHighlight = key

    local baseSize = self.portraitSize
    local bigSize = baseSize * self.activeScale

    for i, id in ipairs(self.portraitIDs) do
        if id and id > 0 then
            if i == highlightSlot then
                SetSpriteColor(id,
                    self.activeTint.r, self.activeTint.g,
                    self.activeTint.b, self.activeTint.a)
                SetScale(id, bigSize, bigSize)
            else
                SetSpriteColor(id,
                    self.inactiveTint.r, self.inactiveTint.g,
                    self.inactiveTint.b, self.inactiveTint.a)
                SetScale(id, baseSize, baseSize)
            end
        end
    end
end

-- ============================================================================
-- UPDATE
-- ============================================================================

function TurnOrderUI:Update(dt, cameraPos)
    if not self.enabled then return end

    if self:ShouldUpdatePosition(cameraPos) then
        self:UpdatePositions(cameraPos)
    end

    self:UpdateHighlight()
end

function TurnOrderUI:UpdatePositions(cameraPos)
    local baseX = cameraPos.x + self.offsetX
    local baseY = cameraPos.y + self.offsetY

    -- Move background
    if self.bgID and self.bgID > 0 then
        SetSpritePosition(self.bgID, baseX, baseY + self.bgOffsetY)
    end

    for i, id in ipairs(self.portraitIDs) do
        if id and id > 0 then
            SetSpritePosition(id, baseX + self:GetSlotOffsetX(i), baseY)
        end
    end

    for i, id in ipairs(self.arrowIDs) do
        if id and id > 0 then
            local arrowX = (self:GetSlotOffsetX(i) + self:GetSlotOffsetX(i + 1)) * 0.5
            SetSpritePosition(id, baseX + arrowX, baseY)
        end
    end
end

-- ============================================================================
-- CLEANUP
-- ============================================================================

function TurnOrderUI:Destroy()
    self.bgID = 0
    self.portraitIDs = {}
    self.arrowIDs = {}
    self.lastHighlight = ""
    UIComponent.Destroy(self)
end

return TurnOrderUI
