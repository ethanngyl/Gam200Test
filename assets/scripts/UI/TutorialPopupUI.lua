--[[
===============================================================================
 File:          TutorialPopupUI.lua
 Authors:       Ethan Ng
 Email:         n.ethanyongle@digipen.edu
 Date:          02/25/2026
 ------------------------------------------------------------------------------

 TUTORIAL POPUP UI - Displays tutorial popups during gameplay

 Brief:
    A UI component for displaying tutorial popups with text and images.
    Can be configured with multiple steps, each containing a title, body,
    and optional hint. Supports text wrapping and viewport-relative positioning.
    Players can navigate through steps with key presses, and the popup will
    follow the camera with optimized updates.

 Copyright (C) 2026 DigiPen Institute of Technology.
 Reproduction or disclosure of this file or its contents
 without the prior written consent of DigiPen Institute of
 Technology is prohibited.
===============================================================================
]]--

local UIComponent = require("UI/UIComponent")

local TutorialPopupUI = {}
setmetatable(TutorialPopupUI, {__index = UIComponent})
TutorialPopupUI.__index = TutorialPopupUI

function TutorialPopupUI:New()
    local instance = UIComponent:New()
    setmetatable(instance, self)
    return instance
end

function TutorialPopupUI:Init(config)
    self.config = config or {}

    self.offsetX = self.config.offsetX or -0.60
    self.offsetY = self.config.offsetY or -0.08
    self.scaleX = self.config.scaleX or 0.52
    self.scaleY = self.config.scaleY or 0.30
    self.layer = self.config.layer or 7
    self.texture = self.config.texture or "assets/Menu/Scroll Overlay.png"

    self.font = self.config.font or "Jersey20Regular"
    self.titleScale = self.config.titleScale or 0.50
    self.bodyScale = self.config.bodyScale or 0.33
    self.hintScale = self.config.hintScale or 0.26
    self.textColor = self.config.textColor or { r = 0.08, g = 0.08, b = 0.08 }

    self.paddingXPx = self.config.paddingXPx or 62
    self.titleInsetTopPx = self.config.titleInsetTopPx or 50
    self.bodyInsetTopPx = self.config.bodyInsetTopPx or 92
    self.lineSpacingPx = self.config.lineSpacingPx or 26
    self.hintInsetBottomPx = self.config.hintInsetBottomPx or 28
    self.wrapChars = self.config.wrapChars or 40
    self.useViewportCoordsForText = (self.config.useViewportCoordsForText ~= false)

    self.steps = self.config.steps or {}
    self.active = (#self.steps > 0)
    self.currentStep = 1
    self.lastXDown = false
    self.lastYDown = false

    local camX, camY = GetCameraPosition()
    self.spriteID = self:SpawnSprite(
        self.texture,
        camX + self.offsetX,
        camY + self.offsetY,
        self.scaleX,
        self.scaleY,
        self.layer
    )

    if self.spriteID and self.spriteID > 0 and SetSpriteVisibility then
        SetSpriteVisibility(self.spriteID, self.active)
    end
end

function TutorialPopupUI:WrapText(input, maxChars)
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

function TutorialPopupUI:Update(dt, cameraPos)
    if not self.enabled then return end

    if self:ShouldUpdatePosition(cameraPos) and self.spriteID and self.spriteID > 0 then
        SetSpritePosition(self.spriteID, cameraPos.x + self.offsetX, cameraPos.y + self.offsetY)
    end

    if not self.active then
        return
    end

    local xDown = IsKeyDown and IsKeyDown("X") or false
    local yDown = IsKeyDown and IsKeyDown("Y") or false

    local xPressed = xDown and not self.lastXDown
    local yPressed = yDown and not self.lastYDown

    if xPressed then
        self.active = false
        if self.spriteID and self.spriteID > 0 and SetSpriteVisibility then
            SetSpriteVisibility(self.spriteID, false)
        end
    elseif yPressed then
        self.currentStep = self.currentStep + 1
        if self.currentStep > #self.steps then
            self.active = false
            if self.spriteID and self.spriteID > 0 and SetSpriteVisibility then
                SetSpriteVisibility(self.spriteID, false)
            end
        end
    end

    self.lastXDown = xDown
    self.lastYDown = yDown
end

function TutorialPopupUI:Draw()
    if not self.enabled or not self.active or not DrawText or not WorldToScreen then
        return
    end

    local step = self.steps[self.currentStep]
    if not step then return end

    local fbW, fbH = nil, nil
    if GetFramebufferSize then
        fbW, fbH = GetFramebufferSize()
    end
    if not fbW or fbW <= 0 then return end

    local camX, camY = GetCameraPosition()
    local px = camX + self.offsetX
    local py = camY + self.offsetY

    local halfW = self.scaleX * 0.5
    local halfH = self.scaleY * 0.5

    local leftScreen, bottomScreen = WorldToScreen(px - halfW, py - halfH, self.useViewportCoordsForText)
    local rightScreen, topScreen = WorldToScreen(px + halfW, py + halfH, self.useViewportCoordsForText)
    if not leftScreen or not rightScreen or not bottomScreen or not topScreen then
        return
    end

    local minX = math.min(leftScreen, rightScreen)
    local maxY = math.max(bottomScreen, topScreen)
    local minY = math.min(bottomScreen, topScreen)
    local scaleRef = fbW / 1920

    local title = step.title or ("Tutorial " .. tostring(self.currentStep))
    local body = self:WrapText(step.body or "", self.wrapChars)
    local leftHint = "[X] Close"
    local rightHint = "[Y] Next"

    local textX = minX + (self.paddingXPx * scaleRef)
    local titleY = maxY - (self.titleInsetTopPx * scaleRef)
    local bodyY = maxY - (self.bodyInsetTopPx * scaleRef)
    local hintY = minY + ((self.hintInsetBottomPx + 48) * scaleRef)
    local maxX = math.max(leftScreen, rightScreen)
    local rightHintX = maxX - (self.paddingXPx * scaleRef) - (string.len(rightHint) * self.hintScale * scaleRef * 24)

    DrawText(
        self.font,
        title,
        textX,
        titleY,
        self.titleScale * scaleRef,
        self.textColor.r,
        self.textColor.g,
        self.textColor.b
    )

    local lineIndex = 0
    for line in body:gmatch("[^\n]+") do
        DrawText(
            self.font,
            line,
            textX,
            bodyY - (lineIndex * self.lineSpacingPx * scaleRef),
            self.bodyScale * scaleRef,
            self.textColor.r,
            self.textColor.g,
            self.textColor.b
        )
        lineIndex = lineIndex + 1
    end

    DrawText(
        self.font,
        leftHint,
        textX,
        hintY,
        self.hintScale * scaleRef,
        self.textColor.r,
        self.textColor.g,
        self.textColor.b
    )

    DrawText(
        self.font,
        rightHint,
        rightHintX,
        hintY,
        self.hintScale * scaleRef,
        self.textColor.r,
        self.textColor.g,
        self.textColor.b
    )
end

function TutorialPopupUI:Destroy()
    if self.spriteID and self.spriteID > 0 then
        DestroyEntity(self.spriteID)
    end
    self.spriteID = 0
end

return TutorialPopupUI