--[[
===============================================================================
 File:          SkillBubbleHolderUI.lua
 Date:          2026-03-06
 ------------------------------------------------------------------------------

 SKILL BUBBLE HOLDER UI - Skill Slot Display Component

 Brief:
    Displays the skill bubble holder on the far right of the screen.
    Acts as a visual placeholder showing which skill slots the player has.
    Skill icons can be placed on top of this holder in the future.

 Features:
    - Camera-relative positioning (far right side)
    - Single sprite display using skill_bubble_holder.png
    - Enable/disable support (hidden during enemy turn)

 Usage:
    local SkillBubbleHolderUI = require("UI/SkillBubbleHolderUI")

    local skillHolder = SkillBubbleHolderUI:New()
    skillHolder:Init({
        offsetX = 0.82,
        offsetY = -0.35,
        scaleX = 0.15,
        scaleY = 0.15,
        layer = 4,
        texture = "assets/new assets/skill_bubble_holder.png"
    })

    -- In update loop
    skillHolder:Update(dt, cameraPos)

    -- Cleanup
    skillHolder:Destroy()

 Copyright (C) 2026 DigiPen Institute of Technology.
 Reproduction or disclosure of this file or its contents
 without the prior written consent of DigiPen Institute of
 Technology is prohibited.
===============================================================================
]]--

local UIComponent = require("UI/UIComponent")
local SkillBubbleHolderUI = {}
setmetatable(SkillBubbleHolderUI, {__index = UIComponent})
SkillBubbleHolderUI.__index = SkillBubbleHolderUI

-- ============================================================================
-- CONSTRUCTOR
-- ============================================================================

function SkillBubbleHolderUI:New()
    local instance = UIComponent:New()
    setmetatable(instance, self)
    return instance
end

-- ============================================================================
-- INITIALIZATION
-- ============================================================================

function SkillBubbleHolderUI:Init(config)
    self.config = config or {}

    -- Configuration
    self.offsetX = self.config.offsetX or 0.82
    self.offsetY = self.config.offsetY or -0.35
    self.scaleX = self.config.scaleX or 0.15
    self.scaleY = self.config.scaleY or 0.15
    self.layer = self.config.layer or 4
    self.texture = self.config.texture or "assets/new assets/skill_bubble_holder.png"

    -- State
    self.spriteID = 0

    -- Get camera position for initial placement
    local camX, camY, camZ = GetCameraPosition()

    local x = camX + self.offsetX
    local y = camY + self.offsetY

    self.spriteID = self:SpawnSprite(self.texture, x, y, self.scaleX, self.scaleY, self.layer)

    Log("[SkillBubbleHolderUI] Initialized at offset (" .. self.offsetX .. ", " .. self.offsetY .. ")")
end

-- ============================================================================
-- UPDATE
-- ============================================================================

function SkillBubbleHolderUI:Update(dt, cameraPos)
    if not self.enabled then return end

    -- Update sprite position if camera moved (keeps UI fixed on screen)
    if self:ShouldUpdatePosition(cameraPos) then
        self:UpdatePositions(cameraPos)
    end
end

-- ============================================================================
-- HELPER METHODS
-- ============================================================================

function SkillBubbleHolderUI:UpdatePositions(cameraPos)
    local x = cameraPos.x + self.offsetX
    local y = cameraPos.y + self.offsetY

    if self.spriteID and self.spriteID > 0 then
        SetSpritePosition(self.spriteID, x, y)
    end
end

function SkillBubbleHolderUI:Destroy()
    if self.spriteID and self.spriteID > 0 then
        DestroyEntity(self.spriteID)
    end

    self.spriteID = 0
    Log("[SkillBubbleHolderUI] Destroyed")
end

return SkillBubbleHolderUI
