--[[
===============================================================================
File:        UIManager.lua
Author:      Padilla Carl Jameson Z.
Email:       c.Padilla@digipen.edu
Date:        2026-02-04 
Contribution: Padilla Carl Jameson Z. (100%)
-------------------------------------------------------------------------------

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

local TOOLTIP_TEXTURE_DEFAULT = "assets/Menu/Scroll Overlay.png"

-- Default icon path convention:
--   assets/SkillIcons/<SkillID>.png
-- You can override specific skills via config.skillIconMap in UIManager.Init().
local DEFAULT_SKILL_ICON_MAP = {
    Thrust = "assets/SkillIcons/Thrust.png",
    SweepingSlash = "assets/SkillIcons/SweepingSlash.png",
    Guard = "assets/SkillIcons/Guard.png",
    SwiftBlow = "assets/SkillIcons/SwiftBlow.png",
    KnightsOath = "assets/SkillIcons/KnightsOath.png",
    Parry = "assets/SkillIcons/Parry.png",
    ExploitWeakness = "assets/SkillIcons/ExploitWeakness.png",
    Bash = "assets/SkillIcons/Bash.png",
    Fireball = "assets/SkillIcons/Fireball.png",
    PiercingShot = "assets/SkillIcons/PiercingShot.png",
    LightningStrike = "assets/SkillIcons/LightningStrike.png",
    EarthenBind = "assets/SkillIcons/EarthenBind.png",
    ManaDrain = "assets/SkillIcons/ManaDrain.png",
    Overload = "assets/SkillIcons/Overload.png",
    SoulRend = "assets/SkillIcons/SoulRend.png",
    SoulMerge = "assets/SkillIcons/SoulMerge.png",
    Slam = "assets/SkillIcons/Slam.png",
    SiphonCharge = "assets/SkillIcons/SiphonCharge.png",
    FutileResistance = "assets/SkillIcons/FutileResistance.png",
    DarkOmens = "assets/SkillIcons/DarkOmens.png",
    Cannibalism = "assets/SkillIcons/Cannibalism.png",
    Groundshatter = "assets/SkillIcons/Groundshatter.png",
    BloodyWarcry = "assets/SkillIcons/BloodyWarcry.png",
    BladedWhirlwind = "assets/SkillIcons/BladedWhirlwind.png",
}

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
    self.iconScale = self.config.iconScale or (self.circleScale * 0.78)
    self.iconLayer = self.config.iconLayer or (self.circleLayer + 1)
    self.defaultIconTexture = self.config.defaultIconTexture or "assets/UI/skill_circle.png"
    self.skillIconBasePath = self.config.skillIconBasePath or "assets/SkillIcons/"
    self.skillIconMap = self.config.skillIconMap or DEFAULT_SKILL_ICON_MAP

    -- Slot index labels (1-4) rendered near each bubble for input clarity.
    self.showSlotLabels = (self.config.showSlotLabels ~= false)
    self.slotLabelFont = self.config.slotLabelFont or "Jersey20Regular"
    self.slotLabelScale = self.config.slotLabelScale or 0.27
    self.slotLabelColor = self.config.slotLabelColor or { r = 0.08, g = 0.08, b = 0.08 }
    self.slotLabelOffsetXPx = self.config.slotLabelOffsetXPx or -14
    self.slotLabelOffsetYPx = self.config.slotLabelOffsetYPx or -14
    self.useViewportCoordsForText = (self.config.useViewportCoordsForText ~= false)

    -- Tooltip configuration
    self.tooltipTexture = self.config.tooltipTexture or TOOLTIP_TEXTURE_DEFAULT
    self.tooltipOffsetX = self.config.tooltipOffsetX or 0.6
    self.tooltipScaleX = self.config.tooltipScaleX or 0.35
    self.tooltipScaleY = self.config.tooltipScaleY or 0.22
    self.tooltipLayer = self.config.tooltipLayer or (self.iconLayer + 1)
    self.tooltipFont = self.config.tooltipFont or "Jersey20Regular"
    self.tooltipTitleScale = self.config.tooltipTitleScale or 0.35
    self.tooltipCostScale = self.config.tooltipCostScale or 0.33
    self.tooltipBodyScale = self.config.tooltipBodyScale or 0.27
    self.tooltipTextColor = self.config.tooltipTextColor or { r = 0.08, g = 0.08, b = 0.08 }
    self.tooltipPaddingXPx = self.config.tooltipPaddingXPx or 68
    self.tooltipHeaderTopInsetPx = self.config.tooltipHeaderTopInsetPx or 58
    self.tooltipDescriptionStartInsetPx = self.config.tooltipDescriptionStartInsetPx or 104
    self.tooltipDescriptionLineSpacingPx = self.config.tooltipDescriptionLineSpacingPx or 26
    self.tooltipWrapChars = self.config.tooltipWrapChars or 33

    -- State
    self.spriteID = 0
    self.circleIDs = {}       -- Sprite IDs for the 4 circles
    self.circleTints = {}     -- Current tint per circle (to avoid redundant updates)
    self.iconIDs = {}         -- Sprite IDs for per-skill icon art
    self.iconSkillIDs = {}    -- Last skill ID bound to icon slot
    self.tooltipID = 0
    self.tooltipVisible = false
    self.hoveredSlot = nil
    self.hoveredSkillID = nil
    self.hoveredSkillName = nil
    self.hoveredSkillDescription = nil
    self.hoveredSkillCost = nil

    self.skillsDb = {}
    if LoadJSON then
        local data = LoadJSON("assets/JSON/Skills.json")
        if data and data.skills then
            self.skillsDb = data.skills
        end
    end

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
        local iconID = self:SpawnSprite(self.defaultIconTexture, x, circleY, self.iconScale, self.iconScale, self.iconLayer)
        self.circleIDs[i] = circleID
        self.iconIDs[i] = iconID
        self.circleTints[i] = nil  -- Force first update
        self.iconSkillIDs[i] = nil

        -- Start as black (no skill state known yet)
        if circleID and circleID > 0 and SetSpriteColor then
            SetSpriteColor(circleID, TINT_BLACK.r, TINT_BLACK.g, TINT_BLACK.b)
        end
        if iconID and iconID > 0 and SetSpriteColor then
            SetSpriteColor(iconID, 0.2, 0.2, 0.2, 1.0)
        end
    end

    self.tooltipID = self:SpawnSprite(self.tooltipTexture, x + self.tooltipOffsetX, y, self.tooltipScaleX, self.tooltipScaleY, self.tooltipLayer)
    if self.tooltipID > 0 and SetSpriteVisibility then
        SetSpriteVisibility(self.tooltipID, false)
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

    -- Update mouse hover and tooltip display state
    self:UpdateTooltip(cameraPos)
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
        local iconID = self.iconIDs[i]
        local circleY = topY - (i - 1) * self.circleSpacing
        if circleID and circleID > 0 then
            SetSpritePosition(circleID, x, circleY)
        end
        if iconID and iconID > 0 then
            SetSpritePosition(iconID, x, circleY)
        end
    end

    if self.tooltipID and self.tooltipID > 0 then
        SetSpritePosition(self.tooltipID, x + self.tooltipOffsetX, y)
    end
end

function SkillBubbleHolderUI:GetSlotWorldCenter(slotIndex)
    local camX, camY = GetCameraPosition()
    local x = camX + self.offsetX
    local totalHeight = self.circleSpacing * 3
    local topY = (camY + self.offsetY) + totalHeight / 2
    local y = topY - (slotIndex - 1) * self.circleSpacing
    return x, y
end

function SkillBubbleHolderUI:GetMouseFramebufferPosition()
    if not GetMousePosition or not GetFramebufferSize then
        return nil, nil
    end

    local fbW, fbH = GetFramebufferSize()
    if not fbW or not fbH or fbW <= 0 or fbH <= 0 then
        return nil, nil
    end

    local mouseX, mouseY = GetMousePosition()
    return mouseX, (fbH - mouseY)
end

function SkillBubbleHolderUI:GetHoveredSlot(state, activePlayerIndex)
    if not state or not activePlayerIndex then
        return nil
    end

    local skills = state.players and state.players[activePlayerIndex] or nil
    if not skills then
        return nil
    end

    local mouseX, mouseY = self:GetMouseFramebufferPosition()
    if not mouseX or not mouseY or not WorldToScreen then
        return nil
    end

    for i = 1, 4 do
        local slotKey = tostring(i)
        if skills[slotKey] then
            local cx, cy = self:GetSlotWorldCenter(i)
            local leftWorld = cx - (self.circleScale * 0.5)
            local rightWorld = cx + (self.circleScale * 0.5)
            local bottomWorld = cy - (self.circleScale * 0.5)
            local topWorld = cy + (self.circleScale * 0.5)

            local leftScreen, bottomScreen = WorldToScreen(leftWorld, bottomWorld, self.useViewportCoordsForText)
            local rightScreen, topScreen = WorldToScreen(rightWorld, topWorld, self.useViewportCoordsForText)

            if leftScreen and bottomScreen and rightScreen and topScreen then
                local minX = math.min(leftScreen, rightScreen)
                local maxX = math.max(leftScreen, rightScreen)
                local minY = math.min(bottomScreen, topScreen)
                local maxY = math.max(bottomScreen, topScreen)

                if mouseX >= minX and mouseX <= maxX and mouseY >= minY and mouseY <= maxY then
                    return slotKey
                end
            end
        end
    end

    return nil
end

function SkillBubbleHolderUI:WrapText(input, maxChars)
    if not input or input == "" then
        return ""
    end

    local out = {}
    local line = ""
    for word in tostring(input):gmatch("%S+") do
        if line == "" then
            line = word
        elseif (#line + #word + 1) <= maxChars then
            line = line .. " " .. word
        else
            table.insert(out, line)
            line = word
        end
    end
    if line ~= "" then
        table.insert(out, line)
    end

    return table.concat(out, "\n")
end

function SkillBubbleHolderUI:GetEstimatedTextWidthPx(text, textScale, scaleRef)
    local s = tostring(text or "")
    -- Jersey font is fairly wide; this constant gives a good visual centering fit.
    local avgGlyphWidthPx = 24 * textScale * scaleRef
    return #s * avgGlyphWidthPx
end

function SkillBubbleHolderUI:UpdateTooltip(cameraPos)
    local state = _G._skillUIState
    local activeEntityID = GetActiveCharacter and GetActiveCharacter() or nil
    local activePlayerIndex = nil

    if state and activeEntityID and activeEntityID > 0 then
        for idx, eid in pairs(state.entityIDs or {}) do
            if eid == activeEntityID then
                activePlayerIndex = idx
                break
            end
        end
    end

    local hoveredSlot = self:GetHoveredSlot(state, activePlayerIndex)
    self.hoveredSlot = hoveredSlot
    self.hoveredSkillID = nil
    self.hoveredSkillName = nil
    self.hoveredSkillDescription = nil
    self.hoveredSkillCost = nil

    local showTooltip = false

    if hoveredSlot and state and state.players and activePlayerIndex then
        local skills = state.players[activePlayerIndex]
        local skillID = skills and skills[hoveredSlot] or nil
        if skillID then
            local skillData = self.skillsDb and self.skillsDb[skillID] or nil
            local slotCost = state.apCosts and state.apCosts[activePlayerIndex] and state.apCosts[activePlayerIndex][hoveredSlot] or nil
            self.hoveredSkillID = skillID
            self.hoveredSkillName = (skillData and skillData.name) or skillID
            self.hoveredSkillDescription = (skillData and skillData.description) or "No description available."
            local rawCost = slotCost or (skillData and skillData.apCost) or 0
            self.hoveredSkillCost = math.floor((tonumber(rawCost) or 0) + 0.5)
            showTooltip = true
        end
    end

    if self.tooltipID and self.tooltipID > 0 then
        local y = cameraPos.y + self.offsetY
        if hoveredSlot then
            local slotIndex = tonumber(hoveredSlot)
            if slotIndex then
                local _, slotY = self:GetSlotWorldCenter(slotIndex)
                y = slotY
            end
        end
        SetSpritePosition(self.tooltipID, cameraPos.x + self.tooltipOffsetX, y)
    end

    if self.tooltipID and self.tooltipID > 0 and SetSpriteVisibility and self.tooltipVisible ~= showTooltip then
        SetSpriteVisibility(self.tooltipID, showTooltip)
    end
    self.tooltipVisible = showTooltip
end

function SkillBubbleHolderUI:ResolveSkillIconTexture(skillID)
    if not skillID or skillID == "" then
        return self.defaultIconTexture
    end

    local mapped = self.skillIconMap and self.skillIconMap[skillID]
    if mapped and mapped ~= "" then
        local f = io.open(mapped, "rb")
        if f then
            f:close()
            return mapped
        end
    end

    local inferred = self.skillIconBasePath .. skillID .. ".png"
    local f = io.open(inferred, "rb")
    if f then
        f:close()
        return inferred
    end

    return self.defaultIconTexture
end

function SkillBubbleHolderUI:UpdateSlotIcons(skills)
    local camX, camY, camZ = GetCameraPosition()
    local x = camX + self.offsetX
    local topY = (camY + self.offsetY) + (self.circleSpacing * 3) / 2

    for i = 1, 4 do
        local slotKey = tostring(i)
        local skillID = skills and skills[slotKey] or nil
        if self.iconSkillIDs[i] ~= skillID then
            self.iconSkillIDs[i] = skillID
            local texture = self:ResolveSkillIconTexture(skillID)
            local iconID = self.iconIDs[i]
            if iconID and iconID > 0 and SetSpriteTexture then
                SetSpriteTexture(iconID, texture)
            elseif not iconID or iconID <= 0 then
                local circleY = topY - (i - 1) * self.circleSpacing
                self.iconIDs[i] = self:SpawnSprite(texture, x, circleY, self.iconScale, self.iconScale, self.iconLayer)
            end
        end
    end
end

function SkillBubbleHolderUI:UpdateCircleTints()
    if not SetSpriteColor then return end

    -- Read skill UI state from the shared bridge table (set by PlayerScript via CallLevelFunction)
    local state = _G._skillUIState
    if not state then return end

    -- Find the active player's data
    local activeEntityID = GetActiveCharacter and GetActiveCharacter() or nil
    local activePlayerIndex = nil
    if activeEntityID and activeEntityID > 0 then
        for idx, eid in pairs(state.entityIDs or {}) do
            if eid == activeEntityID then
                activePlayerIndex = idx
                break
            end
        end
    end

    local skills = activePlayerIndex and state.players[activePlayerIndex] or nil
    local apCosts = activePlayerIndex and state.apCosts[activePlayerIndex] or nil
    local activeSlot = state.activeSlotKey

    -- Only show active slot highlight if it belongs to the active player
    if state.activePlayerIndex ~= activePlayerIndex then
        activeSlot = nil
    end

    self:UpdateSlotIcons(skills)

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
            -- This slot is currently selected/previewed
            tint = TINT_BLUE
        elseif activeEntityID and apCosts and apCosts[slotKey] then
            -- Check if enough attack AP to use this skill
            local currentAttackAP = GetEntityAttackAP(activeEntityID)
            if currentAttackAP < apCosts[slotKey] then
                tint = TINT_GREY  -- Not enough AP (on cooldown)
            else
                tint = TINT_WHITE -- Available and usable
            end
        else
            tint = TINT_WHITE
        end

        -- Only update if tint changed (avoid redundant GPU calls)
        if tint ~= self.circleTints[i] then
            self.circleTints[i] = tint
            local circleID = self.circleIDs[i]
            if circleID and circleID > 0 then
                SetSpriteColor(circleID, tint.r, tint.g, tint.b)
            end
            local iconID = self.iconIDs[i]
            if iconID and iconID > 0 then
                SetSpriteColor(iconID, tint.r, tint.g, tint.b)
            end
        end
    end
end

-- Draw slot numbers (1-4) near each bubble.
-- DrawText uses framebuffer-space coordinates, so convert from camera-relative
-- UI offsets to screen-space each frame.
function SkillBubbleHolderUI:Draw()
    if not self.enabled or not DrawText then
        return
    end

    local fbW, fbH = nil, nil
    if GetFramebufferSize then
        fbW, fbH = GetFramebufferSize()
    end
    if not fbW or not fbH or fbW <= 0 or fbH <= 0 then
        return
    end

    local camX, camY, camZ = GetCameraPosition()
    local scaleRef = fbW / 1920

    local totalHeight = self.circleSpacing * 3
    local topY = (camY + self.offsetY) + totalHeight / 2
    local x = camX + self.offsetX

    if self.showSlotLabels then
        for i = 1, 4 do
            local circleY = topY - (i - 1) * self.circleSpacing
            local sx = nil
            local sy = nil

            if WorldToScreen then
                sx, sy = WorldToScreen(x, circleY, self.useViewportCoordsForText)
            end

            if sx and sy then
                sx = sx + self.slotLabelOffsetXPx
                sy = sy + self.slotLabelOffsetYPx

                DrawText(
                    self.slotLabelFont,
                    tostring(i),
                    sx,
                    sy,
                    self.slotLabelScale * scaleRef,
                    self.slotLabelColor.r,
                    self.slotLabelColor.g,
                    self.slotLabelColor.b
                )
            end
        end
    end

    if not self.tooltipVisible or not self.hoveredSkillName then
        return
    end
    if IsPaused and IsPaused() then
        return
    end

    if not WorldToScreen then
        return
    end

    local camX, camY = GetCameraPosition()
    local tooltipX = camX + self.tooltipOffsetX
    local tooltipY = camY + self.offsetY

    if self.hoveredSlot then
        local slotIndex = tonumber(self.hoveredSlot)
        if slotIndex then
            local _, slotY = self:GetSlotWorldCenter(slotIndex)
            tooltipY = slotY
        end
    end

    local sx, sy = WorldToScreen(tooltipX, tooltipY, self.useViewportCoordsForText)
    if not sx or not sy then
        return
    end

    local halfW = self.tooltipScaleX * 0.5
    local halfH = self.tooltipScaleY * 0.5
    local leftScreen, bottomScreen = WorldToScreen(tooltipX - halfW, tooltipY - halfH, self.useViewportCoordsForText)
    local rightScreen, topScreen = WorldToScreen(tooltipX + halfW, tooltipY + halfH, self.useViewportCoordsForText)
    if not leftScreen or not rightScreen or not bottomScreen or not topScreen then
        return
    end

    local minX = math.min(leftScreen, rightScreen)
    local maxX = math.max(leftScreen, rightScreen)
    local topY = math.max(bottomScreen, topScreen)

    local scaleRef = fbW / 1920
    local wrappedDescription = self:WrapText(self.hoveredSkillDescription, self.tooltipWrapChars)
    local titleX = minX + (self.tooltipPaddingXPx * scaleRef)
    local titleY = topY - (self.tooltipHeaderTopInsetPx * scaleRef)
    local costText = string.format("AP %d", math.floor((tonumber(self.hoveredSkillCost) or 0) + 0.5))
    local costWidthPx = self:GetEstimatedTextWidthPx(costText, self.tooltipCostScale, scaleRef)
    local costX = maxX - (self.tooltipPaddingXPx * scaleRef) - costWidthPx
    local costY = titleY

    DrawText(
        self.tooltipFont,
        self.hoveredSkillName,
        titleX,
        titleY,
        self.tooltipTitleScale * scaleRef,
        self.tooltipTextColor.r,
        self.tooltipTextColor.g,
        self.tooltipTextColor.b
    )

    DrawText(
        self.tooltipFont,
        costText,
        costX,
        costY,
        self.tooltipCostScale * scaleRef,
        self.tooltipTextColor.r,
        self.tooltipTextColor.g,
        self.tooltipTextColor.b
    )

    local lineIndex = 0
    local descYStart = topY - (self.tooltipDescriptionStartInsetPx * scaleRef)
    for line in wrappedDescription:gmatch("[^\n]+") do
        DrawText(
            self.tooltipFont,
            line,
            titleX,
            descYStart - (lineIndex * self.tooltipDescriptionLineSpacingPx * scaleRef),
            self.tooltipBodyScale * scaleRef,
            self.tooltipTextColor.r,
            self.tooltipTextColor.g,
            self.tooltipTextColor.b
        )
        lineIndex = lineIndex + 1
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
        if self.iconIDs[i] and self.iconIDs[i] > 0 then
            DestroyEntity(self.iconIDs[i])
        end
    end
    if self.tooltipID and self.tooltipID > 0 then
        DestroyEntity(self.tooltipID)
    end
    self.circleIDs = {}
    self.iconIDs = {}
    self.iconSkillIDs = {}
    self.circleTints = {}
    self.tooltipID = 0
    self.tooltipVisible = false

    Log("[SkillBubbleHolderUI] Destroyed")
end

return SkillBubbleHolderUI
