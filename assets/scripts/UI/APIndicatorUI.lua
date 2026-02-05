--[[
===============================================================================
File:        APIndicatorUI.lua
Author:      Sim Kah Yan
Email:       kahyan.sim@digipen.edu
Date:        2026-02-04 (yyyy-mm-dd)
Contribution: 46% 
-------------------------------------------------------------------------------
Brief:
UI component that displays Action Points (AP) as a row of crystal-style
indicators. Supports two display modes (tint-based or two-layer sprites),
camera-relative positioning, and optional refill animation on full AP restore.

Details:
- Inherits from UIComponent; uses SpawnSprite, camera position, and
  ShouldUpdatePosition/UpdatePositions for camera-relative layout.
- Config: maxAP, size, spacing, offsetX/offsetY, layer; filled/empty textures
  (or single texture with useTint); optional grayscale (filledGrayAmount/
  emptyGrayAmount); getAPFunc (default GetPlayerAP); showEmpty, useMaxFromAP.
- Two modes: (1) useTint=true — one sprite per slot, filled vs empty shown by
  tint and optional grayscale. (2) useTint=false — background row of empty
  sprites plus foreground row of filled sprites; AP changes spawn/destroy
  filled sprites or update count.
- On full refill (AP 0 -> maxAP), runs refill animation: crystals appear
  one-by-one with refillAnimDelay; during animation, normal AP updates are
  deferred. Consumption updates apply immediately (remove or tint empty).
- Init uses current camera position to place indicators; Update(dt, cameraPos)
  polls getAPFunc, updates positions when camera moves, and applies AP changes
  or refill animation. Destroy() removes all spawned entities.

Notes:
- Default texture "assets/UI/MovP.png"; empty can share texture with
  different tint/gray. Layer default 4.
- useMaxFromAP=true uses max from getAPFunc() when available; otherwise
  uses config maxAP. showEmpty=false only draws filled crystals (no back row).

Safety:
- Nil-checks on entityID and tint; getAPFunc() return used only when non-nil.
- Destroy() iterates and destroys all indicators; tables cleared after.
- Refill animation state (isAnimatingRefill) prevents overlapping animations.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
===============================================================================
]]

-- ============================================================================
-- APIndicatorUI.lua
-- Movement Action Point (AP) indicator component
-- ============================================================================
-- Displays movement AP as a row of crystals that fill/empty
-- Uses two-layer system: empty background + filled foreground
-- ============================================================================

local UIComponent = require("UI/UIComponent")
local APIndicatorUI = {}
setmetatable(APIndicatorUI, {__index = UIComponent})
APIndicatorUI.__index = APIndicatorUI

-- ============================================================================
-- CONSTRUCTOR
-- ============================================================================

function APIndicatorUI:New()
    local instance = UIComponent:New()
    setmetatable(instance, self)
    return instance
end

-- ============================================================================
-- INITIALIZATION
-- ============================================================================

function APIndicatorUI:Init(config)
    self.config = config or {}

    -- Configuration
    self.maxAP = self.config.maxAP or 5
    self.indicatorSize = self.config.size or 0.06
    self.indicatorSpacing = self.config.spacing or 0.1
    self.offsetX = self.config.offsetX or -0.64
    self.offsetY = self.config.offsetY or -0.42
    self.layer = self.config.layer or 4

    -- Textures
    self.filledTexture = self.config.filledTexture or "assets/UI/MovP.png"
    self.emptyTexture = self.config.emptyTexture or self.filledTexture

    -- Tint configuration (used when sharing texture instead of separate empty sprite)
    self.useTint = self.config.useTint or false
    self.filledTint = self.config.filledTint or { r = 1.0, g = 1.0, b = 1.0 }
    self.emptyTint = self.config.emptyTint or self.filledTint

    -- Optional grayscale control (uses shader parameter in GraphicsSystemV2)
    self.useGray = self.config.useGray or false
    self.filledGrayAmount = self.config.filledGrayAmount or 0.0
    self.emptyGrayAmount = self.config.emptyGrayAmount or 1.0

    -- AP source + empty layer toggle
    self.getAPFunc = self.config.getAPFunc or GetPlayerAP
    self.showEmpty = self.config.showEmpty
    if self.showEmpty == nil then
        self.showEmpty = true
    end
    self.useMaxFromAP = self.config.useMaxFromAP
    if self.useMaxFromAP == nil then
        self.useMaxFromAP = true
    end

    -- State
    self.emptyIndicators = {}
    self.filledIndicators = {}
    self.lastKnownAP = 0

    -- Animation state for refill
    self.isAnimatingRefill = false
    self.refillAnimTimer = 0.0
    self.refillAnimDelay = 0.15  -- Delay between each crystal appearing (seconds)
    self.refillAnimTarget = 0    -- Target AP to animate to
    self.refillAnimCurrent = 0   -- Current animated AP value

    -- Get current AP (needed for initial tinting)
    local currentAP, maxPlayerAP = self.getAPFunc()
    if self.useMaxFromAP and maxPlayerAP and maxPlayerAP > 0 then
        self.maxAP = maxPlayerAP
    end
    self.lastKnownAP = currentAP or 0

    -- Get camera position for initial placement
    local camX, camY, camZ = GetCameraPosition()

    if self.useTint then
        -- Single-layer mode: tint sprites based on AP
        self.indicators = {}
        for i = 1, self.maxAP do
            local xPos = camX + self.offsetX + ((i - 1) * self.indicatorSpacing)
            local yPos = camY + self.offsetY

            local entityID = self:SpawnSprite(
                self.filledTexture,
                xPos, yPos,
                self.indicatorSize, self.indicatorSize,
                self.layer
            )

            self.indicators[i] = entityID

            if entityID and entityID > 0 then
                local tint = (i <= self.lastKnownAP) and self.filledTint or self.emptyTint
                local grayAmount = (i <= self.lastKnownAP) and self.filledGrayAmount or self.emptyGrayAmount
                self:ApplyVisual(entityID, tint, grayAmount)
            end
        end
    else
        -- Two-layer mode: empty background + filled foreground
        if self.showEmpty then
            for i = 1, self.maxAP do
                local xPos = camX + self.offsetX + ((i - 1) * self.indicatorSpacing)
                local yPos = camY + self.offsetY

                local entityID = self:SpawnSprite(
                    self.emptyTexture,
                    xPos, yPos,
                    self.indicatorSize, self.indicatorSize,
                    self.layer
                )

                self.emptyIndicators[i] = entityID

                if entityID and entityID > 0 then
                    self:ApplyVisual(entityID, self.emptyTint, self.emptyGrayAmount)
                end
            end
        end

        -- Create filled crystal foreground layer
        for i = 1, self.lastKnownAP do
            local xPos = camX + self.offsetX + ((i - 1) * self.indicatorSpacing)
            local yPos = camY + self.offsetY

            local entityID = self:SpawnSprite(
                self.filledTexture,
                xPos, yPos,
                self.indicatorSize, self.indicatorSize,
                self.layer
            )

            self.filledIndicators[i] = entityID

            if entityID and entityID > 0 then
                self:ApplyVisual(entityID, self.filledTint, self.filledGrayAmount)
            end
        end
    end

    Log("[APIndicatorUI] Initialized - " .. self.lastKnownAP .. "/" .. self.maxAP .. " AP")
end

-- ============================================================================
-- UPDATE
-- ============================================================================

function APIndicatorUI:Update(dt, cameraPos)
    if not self.enabled then return end

    -- Get current AP
    local currentAP, maxPlayerAP = self.getAPFunc()
    if not currentAP then return end

    -- Update sprite positions if camera moved (keeps UI fixed on screen)
    if self:ShouldUpdatePosition(cameraPos) then
        self:UpdatePositions(cameraPos)
    end

    -- Handle refill animation
    if self.isAnimatingRefill then
        self.refillAnimTimer = self.refillAnimTimer + dt

        -- Check if it's time to show next crystal
        if self.refillAnimTimer >= self.refillAnimDelay then
            self.refillAnimTimer = self.refillAnimTimer - self.refillAnimDelay
            self.refillAnimCurrent = self.refillAnimCurrent + 1

            -- Update visual
            if self.useTint then
                self:UpdateTints(self.refillAnimCurrent)
            else
                self:HandleAPChange(self.refillAnimCurrent, cameraPos)
            end

            print("[APIndicatorUI] Refill animation: " .. self.refillAnimCurrent .. "/" .. self.refillAnimTarget)

            -- Check if animation complete
            if self.refillAnimCurrent >= self.refillAnimTarget then
                self.isAnimatingRefill = false
                self.lastKnownAP = currentAP
                print("[APIndicatorUI] Refill animation COMPLETE!")
            end
        end
        return  -- Don't process normal AP changes during animation
    end

    -- Handle normal AP changes (not during animation)
    if currentAP ~= self.lastKnownAP then
        -- Check if this is a full refill (turn switch)
        if currentAP == self.maxAP and self.lastKnownAP == 0 then
            -- Start refill animation
            self:StartRefillAnimation(currentAP)
        else
            -- Instant update for normal AP changes (consumption)
            if self.useTint then
                self:UpdateTints(currentAP)
            else
                self:HandleAPChange(currentAP, cameraPos)
            end
            self.lastKnownAP = currentAP
        end
    end
end

-- ============================================================================
-- HELPER METHODS
-- ============================================================================

function APIndicatorUI:UpdatePositions(cameraPos)
    if self.useTint then
        -- Update single-layer indicators
        for i = 1, #self.indicators do
            local entityID = self.indicators[i]
            if entityID and entityID > 0 then
                local xPos = cameraPos.x + self.offsetX + ((i - 1) * self.indicatorSpacing)
                local yPos = cameraPos.y + self.offsetY
                SetSpritePosition(entityID, xPos, yPos)
            end
        end
    else
        -- Update empty indicators
        for i = 1, #self.emptyIndicators do
            local entityID = self.emptyIndicators[i]
            if entityID and entityID > 0 then
                local xPos = cameraPos.x + self.offsetX + ((i - 1) * self.indicatorSpacing)
                local yPos = cameraPos.y + self.offsetY
                SetSpritePosition(entityID, xPos, yPos)
            end
        end

        -- Update filled indicators
        for i = 1, #self.filledIndicators do
            local entityID = self.filledIndicators[i]
            if entityID and entityID > 0 then
                local xPos = cameraPos.x + self.offsetX + ((i - 1) * self.indicatorSpacing)
                local yPos = cameraPos.y + self.offsetY
                SetSpritePosition(entityID, xPos, yPos)
            end
        end
    end
end

function APIndicatorUI:HandleAPChange(newAP, cameraPos)
    if newAP < self.lastKnownAP then
        -- AP decreased - destroy filled crystals
        for i = self.lastKnownAP, newAP + 1, -1 do
            if self.filledIndicators[i] then
                DestroyEntity(self.filledIndicators[i])
                self.filledIndicators[i] = nil
            end
        end
    elseif newAP > self.lastKnownAP then
        -- AP increased - create new filled crystals
        for i = self.lastKnownAP + 1, newAP do
            local xPos = cameraPos.x + self.offsetX + ((i - 1) * self.indicatorSpacing)
            local yPos = cameraPos.y + self.offsetY

            local entityID = self:SpawnSprite(
                self.filledTexture,
                xPos, yPos,
                self.indicatorSize, self.indicatorSize,
                self.layer
            )

            self.filledIndicators[i] = entityID

            if entityID and entityID > 0 then
                self:ApplyVisual(entityID, self.filledTint, self.filledGrayAmount)
            end
        end
    end
end

function APIndicatorUI:UpdateTints(currentAP)
    for i = 1, #self.indicators do
        local entityID = self.indicators[i]
        if entityID and entityID > 0 then
            local tint = (i <= currentAP) and self.filledTint or self.emptyTint
            local grayAmount = (i <= currentAP) and self.filledGrayAmount or self.emptyGrayAmount
            self:ApplyVisual(entityID, tint, grayAmount)
        end
    end
end

function APIndicatorUI:ApplyVisual(entityID, tint, grayAmount)
    self:ApplyTint(entityID, tint)
    if self.useGray then
        SetSpriteGray(entityID, grayAmount or 0.0)
    end
end

function APIndicatorUI:ApplyTint(entityID, tint)
    if not entityID or entityID <= 0 or not tint then
        return
    end

    local r = tint.r or tint[1] or 1.0
    local g = tint.g or tint[2] or 1.0
    local b = tint.b or tint[3] or 1.0
    SetSpriteColor(entityID, r, g, b)
end

-- ============================================================================
-- REFILL ANIMATION
-- ============================================================================

function APIndicatorUI:StartRefillAnimation(targetAP)
    print("[APIndicatorUI] Starting refill animation from 0 to " .. targetAP)
    self.isAnimatingRefill = true
    self.refillAnimTimer = 0.0
    self.refillAnimTarget = targetAP
    self.refillAnimCurrent = 0

    -- Reset visuals to empty state
    if self.useTint then
        self:UpdateTints(0)
    end
end

function APIndicatorUI:IsAnimating()
    return self.isAnimatingRefill
end

-- ============================================================================
-- FORCE UPDATE - Instantly update AP display (skip animation)
-- Call this when switching characters to show full AP immediately
-- ============================================================================

function APIndicatorUI:ForceUpdate()
    -- Stop any ongoing animation
    self.isAnimatingRefill = false
    self.refillAnimTimer = 0.0
    
    -- Get current AP from the active character
    local currentAP, maxPlayerAP = self.getAPFunc()
    if not currentAP then 
        currentAP = self.maxAP  -- Default to full if can't get AP
    end
    
    -- Update max AP if needed
    if self.useMaxFromAP and maxPlayerAP and maxPlayerAP > 0 then
        self.maxAP = maxPlayerAP
    end
    
    print("[APIndicatorUI] ForceUpdate: Setting AP to " .. currentAP .. "/" .. self.maxAP .. " (was " .. self.lastKnownAP .. ")")
    
    -- Get camera position for positioning
    local camX, camY, camZ = GetCameraPosition()
    local cameraPos = { x = camX, y = camY, z = camZ }
    
    -- Instantly update visuals
    if self.useTint then
        self:UpdateTints(currentAP)
    else
        -- For two-layer mode, need to handle AP change
        self:HandleAPChange(currentAP, cameraPos)
    end
    
    -- Update last known AP to current
    self.lastKnownAP = currentAP
    
    print("[APIndicatorUI] ForceUpdate complete - now showing " .. currentAP .. " AP")
end

-- Alias for consistency with AttackAPIndicatorUI
function APIndicatorUI:RestoreAllAP()
    self:ForceUpdate()
end

-- ============================================================================
-- CLEANUP
-- ============================================================================

function APIndicatorUI:Destroy()
    if self.useTint then
        for i = 1, #self.indicators do
            if self.indicators[i] and self.indicators[i] > 0 then
                DestroyEntity(self.indicators[i])
            end
        end
        self.indicators = {}
    else
        -- Destroy empty indicators
        for i = 1, #self.emptyIndicators do
            if self.emptyIndicators[i] and self.emptyIndicators[i] > 0 then
                DestroyEntity(self.emptyIndicators[i])
            end
        end

        -- Destroy filled indicators
        for i = 1, #self.filledIndicators do
            if self.filledIndicators[i] and self.filledIndicators[i] > 0 then
                DestroyEntity(self.filledIndicators[i])
            end
        end

        self.emptyIndicators = {}
        self.filledIndicators = {}
    end

    Log("[APIndicatorUI] Destroyed")
end

return APIndicatorUI