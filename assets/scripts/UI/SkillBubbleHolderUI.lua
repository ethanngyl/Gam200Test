--[[
===============================================================================
 File:          SkillBubbleHolderUI.lua
 Date:          2026-03-06
 ------------------------------------------------------------------------------

 SKILL BUBBLE HOLDER UI - Skill Slot Display Component

 Brief:
    Displays the skill bubble holder on the far right of the screen.
    Shows 4 white circle indicators vertically that reflect skill slot state:
      - White: skill assigned, available to use
      - Blue:  skill currently selected/previewed
      - Grey:  skill on cooldown (insufficient AP)
      - Black: no skill assigned to this slot

 Features:
    - Camera-relative positioning (far right side)
    - Background sprite using skill_bubble_holder.png
    - 4 circle indicators for skill slots 1-4
    - Dynamic tint based on skill state

 Usage:
    local SkillBubbleHolderUI = require("UI/SkillBubbleHolderUI")

    local skillHolder = SkillBubbleHolderUI:New()
    skillHolder:Init({
        offsetX = 0.82,
        offsetY = -0.15,
        scaleX = 0.15,
        scaleY = 0.55,
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
-- TINT COLORS
-- ============================================================================

local TINT_WHITE = { r = 1.0, g = 1.0, b = 1.0 }   -- Skill assigned, usable
local TINT_BLUE  = { r = 0.2, g = 0.4, b = 1.0 }   -- Currently selected
local TINT_GREY  = { r = 0.4, g = 0.4, b = 0.4 }   -- On cooldown / not enough AP
local TINT_BLACK = { r = 0.05, g = 0.05, b = 0.05 } -- No skill assigned

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
    self.offsetY = self.config.offsetY or -0.15
    self.scaleX = self.config.scaleX or 0.15
    self.scaleY = self.config.scaleY or 0.55
    self.layer = self.config.layer or 4
    self.texture = self.config.texture or "assets/new assets/skill_bubble_holder.png"

    -- Circle configuration
    self.circleTexture = self.config.circleTexture or "assets/UI/skill_circle.png"
    self.circleScale = self.config.circleScale or 0.06
    self.circleSpacing = self.config.circleSpacing or 0.11
    self.circleLayer = (self.layer or 4) + 1  -- Render on top of holder

    -- State
    self.spriteID = 0
    self.circleIDs = {}       -- Sprite IDs for the 4 circles
    self.circleTints = {}     -- Current tint per circle (to avoid redundant updates)

    -- Get camera position for initial placement
    local camX, camY, camZ = GetCameraPosition()

    local x = camX + self.offsetX
    local y = camY + self.offsetY

    -- Spawn background holder sprite
    self.spriteID = self:SpawnSprite(self.texture, x, y, self.scaleX, self.scaleY, self.layer)

    -- Spawn 4 circle indicators vertically (top to bottom = slot 1 to 4)
    local totalHeight = self.circleSpacing * 3  -- Space between first and last circle
    local topY = y + totalHeight / 2

    for i = 1, 4 do
        local circleY = topY - (i - 1) * self.circleSpacing
        local circleID = self:SpawnSprite(self.circleTexture, x, circleY, self.circleScale, self.circleScale, self.circleLayer)
        self.circleIDs[i] = circleID
        self.circleTints[i] = nil  -- Force first update

        -- Start as black (no skill state known yet)
        if circleID and circleID > 0 and SetSpriteColor then
            SetSpriteColor(circleID, TINT_BLACK.r, TINT_BLACK.g, TINT_BLACK.b)
        end
    end

    Log("[SkillBubbleHolderUI] Initialized at offset (" .. self.offsetX .. ", " .. self.offsetY .. ") with 4 skill circles")
end

-- ============================================================================
-- UPDATE
-- ============================================================================

function SkillBubbleHolderUI:Update(dt, cameraPos)
    if not self.enabled then return end

    -- Update sprite positions if camera moved
    if self:ShouldUpdatePosition(cameraPos) then
        self:UpdatePositions(cameraPos)
    end

    -- Update circle tints based on skill state
    self:UpdateCircleTints()
end

-- ============================================================================
-- HELPER METHODS
-- ============================================================================

function SkillBubbleHolderUI:UpdatePositions(cameraPos)
    local x = cameraPos.x + self.offsetX
    local y = cameraPos.y + self.offsetY

    -- Update holder background
    if self.spriteID and self.spriteID > 0 then
        SetSpritePosition(self.spriteID, x, y)
    end

    -- Update circle positions
    local totalHeight = self.circleSpacing * 3
    local topY = y + totalHeight / 2

    for i = 1, 4 do
        local circleID = self.circleIDs[i]
        if circleID and circleID > 0 then
            local circleY = topY - (i - 1) * self.circleSpacing
            SetSpritePosition(circleID, x, circleY)
        end
    end
end

function SkillBubbleHolderUI:UpdateCircleTints()
    if not SetSpriteColor then return end

    -- Get the active character's skills
    local skills = nil
    if GetActiveCharacterSkills then
        skills = GetActiveCharacterSkills()
    end

    -- Get which slot is currently selected
    local activeSlot = nil
    if GetActiveSkillSlotKey then
        activeSlot = GetActiveSkillSlotKey()
    end

    for i = 1, 4 do
        local slotKey = tostring(i)
        local tint = nil

        if skills == nil then
            -- No active character data, keep black
            tint = TINT_BLACK
        elseif not skills[slotKey] then
            -- No skill assigned to this slot
            tint = TINT_BLACK
        elseif activeSlot == slotKey then
            -- This slot is currently selected
            tint = TINT_BLUE
        elseif CanUseSkill and not CanUseSkill(slotKey) then
            -- Skill assigned but not enough AP (cooldown/unavailable)
            tint = TINT_GREY
        else
            -- Skill assigned and usable
            tint = TINT_WHITE
        end

        -- Only update if tint changed (avoid redundant GPU calls)
        if tint ~= self.circleTints[i] then
            self.circleTints[i] = tint
            local circleID = self.circleIDs[i]
            if circleID and circleID > 0 then
                SetSpriteColor(circleID, tint.r, tint.g, tint.b)
            end
        end
    end
end

function SkillBubbleHolderUI:Destroy()
    -- Circles are tracked in self.entities via SpawnSprite, so UIComponent:Destroy handles them
    if self.spriteID and self.spriteID > 0 then
        DestroyEntity(self.spriteID)
    end
    self.spriteID = 0

    for i = 1, 4 do
        if self.circleIDs[i] and self.circleIDs[i] > 0 then
            DestroyEntity(self.circleIDs[i])
        end
    end
    self.circleIDs = {}
    self.circleTints = {}

    Log("[SkillBubbleHolderUI] Destroyed")
end

return SkillBubbleHolderUI
