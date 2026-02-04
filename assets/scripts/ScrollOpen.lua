--[[
===============================================================================
File:        ScrollOpen.lua
Author:      Sim Kah Yan
Email:       kahyan.sim@digipen.edu
Date:        2026-02-04 (yyyy-mm-dd)
Contribution: 100% (336 lines)
-------------------------------------------------------------------------------
Brief:
TurnScrollUI (filename ScrollOpen.lua): UI component that shows a "Your Turn"
popup when the turn switches to Player. Plays ScrollOpen.png animation once
(open -> close); draws text only during the middle frames when the scroll is open.

Details:
- Inherits from UIComponent. Init(config): offset/scale/layer; animation
  (animName, frameTime, texture, totalFrames 0..35); text (text, font, scale,
  color, textStartFrame/textEndFrame for visibility range). Spawns scroll
  sprite at camera+offset, loads animation via PlayAnimationByName, keeps
  manual frame control (SetAnimationPlaying false). State: "idle" or "playing".
- StartAnimation(): state=playing, frame=0, scroll visible. Update(dt, cameraPos):
  anchors scroll to camera; in idle, triggers on first Player turn or Enemy->Player
  transition; in playing, advances frame timer, steps currentFrame, hides at
  last frame. Draw(): only when playing and currentFrame in [textStartFrame,
  textEndFrame]; centers text on screen (GetFramebufferSize, DrawText). Destroy()
  cleans up via UIComponent.Destroy.

Notes:
- Default text "Your Turn"; frames 8-12 (configurable) for text visibility.
  hasTriggeredOnce prevents repeat on first load. C++ animation system is
  told not to auto-play so this script drives frames manually.

Safety:
- scrollID and optional APIs (PlayAnimationByName, SetAnimationFrame, etc.)
  checked before use. IsPaused guarded in Draw. GetFramebufferSize nil-checked.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
===============================================================================
]]

-- ============================================================================
-- TurnScrollUI.lua (uses ScrollOpen.lua filename)
-- Player turn popup: plays ScrollOpen.png animation once (open -> close)
-- The sprite contains the full animation: frames 0-35 = open then close
-- ============================================================================

local UIComponent = require("UI/UIComponent")

local TurnScrollUI = {}
setmetatable(TurnScrollUI, {__index = UIComponent})
TurnScrollUI.__index = TurnScrollUI

local DEFAULT_TEXT_COLOR = {0.0, 0.0, 0.0}

-- ============================================================================
-- CONSTRUCTOR
-- ============================================================================

function TurnScrollUI:New()
    local instance = UIComponent:New()
    setmetatable(instance, self)
    return instance
end

-- ============================================================================
-- INITIALIZATION
-- ============================================================================

function TurnScrollUI:Init(config)
    self.config = config or {}

    -- Positioning + size
    self.offsetX = self.config.offsetX or 0.0
    self.offsetY = self.config.offsetY or 0.05
    self.scaleX = self.config.scaleX or 0.9
    self.scaleY = self.config.scaleY or 0.35
    self.layer = self.config.layer or 6

    -- Animation config
    self.animName = self.config.animName or "ScrollOpen"
    self.texture = self.config.texture or "assets/UI/ScrollOpen.png"
    self.animationConfigPath = self.config.animationConfigPath or "assets/JSON/animations.json"

    -- Frame count (36 frames: 0-35)
    -- Assuming: frames 0-17 = opening, frames 18-35 = closing
    self.totalFrames = 36
    self.lastFrame = self.totalFrames - 1  -- 35
    self.openEndFrame = 17   -- Last frame of opening animation
    self.closeStartFrame = 18 -- First frame of closing animation
    
    -- Timing configuration
    self.openDuration = self.config.openDuration or 0.75    -- Opening animation duration
    self.holdDuration = self.config.holdDuration or 1.5     -- Time to stay open
    self.closeDuration = self.config.closeDuration or 0.75  -- Closing animation duration
    
    -- Calculate frame times based on durations
    local openFrameCount = self.openEndFrame + 1  -- 18 frames for opening
    local closeFrameCount = self.totalFrames - self.closeStartFrame  -- 18 frames for closing
    self.openFrameTime = self.openDuration / openFrameCount
    self.closeFrameTime = self.closeDuration / closeFrameCount

    -- Text config
    self.text = self.config.text or "Your Turn"
    self.textFont = self.config.textFont or "Sans48"
    self.textScale = self.config.textScale or 0.9
    self.textColor = self.config.textColor or DEFAULT_TEXT_COLOR
    self.textOffsetX = self.config.textOffsetX or 0
    self.textOffsetY = self.config.textOffsetY or 0
    self.textAnchorX = self.config.textAnchorX
    self.textAnchorY = self.config.textAnchorY
    self.textBaseSize = self.config.textBaseSize or 48
    self.textWidthFactor = self.config.textWidthFactor or 0.6
    self.textAlign = self.config.textAlign or "center"
    
    -- Text visibility: show when scroll is open (during hold phase and partially during open/close)
    self.textStartFrame = self.config.textStartFrame or 10
    self.textEndFrame = self.config.textEndFrame or 25

    -- State: "idle", "opening", "holding", "closing"
    self.scrollID = 0
    self.state = "idle"
    self.currentFrame = 0
    self.frameTimer = 0.0
    self.holdTimer = 0.0  -- Timer for hold phase
    
    -- Track turn phase for triggering
    self.lastTurnPhase = GetCurrentTurn() or "Player"
    self.hasTriggeredOnce = false

    if LoadAnimationConfig then
        LoadAnimationConfig(self.animationConfigPath)
    end

    -- Spawn scroll sprite
    local camX, camY, _ = GetCameraPosition()
    local scrollX = camX + self.offsetX
    local scrollY = camY + self.offsetY

    self.scrollID = self:SpawnSprite(
        self.texture,
        scrollX, scrollY,
        self.scaleX, self.scaleY,
        self.layer
    )
    self.scrollWorldX = scrollX
    self.scrollWorldY = scrollY

    if self.scrollID and self.scrollID > 0 then
        SetSpriteVisibility(self.scrollID, false)
        if PlayAnimationByName then
            PlayAnimationByName(self.scrollID, self.animName, false)
        end
        if SetAnimationGroup then
            SetAnimationGroup(self.scrollID, 0)
        end
        if SetAnimationDirection then
            SetAnimationDirection(self.scrollID, 3)
        end
        if SetAnimationPlaying then
            SetAnimationPlaying(self.scrollID, false)
        end
        if SetAnimationFrame then
            SetAnimationFrame(self.scrollID, 0)
        end
        
        -- Get actual frame count from animation system
        if GetAnimationFrameCount then
            local frameCount = GetAnimationFrameCount(self.scrollID)
            if frameCount and frameCount > 0 then
                self.totalFrames = frameCount
                self.lastFrame = frameCount - 1
                
                -- Recalculate animation phases based on actual frame count
                -- Assume first half is opening, second half is closing
                self.openEndFrame = math.floor(self.totalFrames / 2) - 1
                self.closeStartFrame = math.floor(self.totalFrames / 2)
                
                -- Recalculate frame times
                local openFrameCount = self.openEndFrame + 1
                local closeFrameCount = self.totalFrames - self.closeStartFrame
                self.openFrameTime = self.openDuration / openFrameCount
                self.closeFrameTime = self.closeDuration / closeFrameCount
                
                Log("[ScrollOpen] Recalculated: totalFrames=" .. self.totalFrames .. 
                    ", openEndFrame=" .. self.openEndFrame .. 
                    ", closeStartFrame=" .. self.closeStartFrame ..
                    ", openFrameTime=" .. string.format("%.4f", self.openFrameTime) ..
                    ", closeFrameTime=" .. string.format("%.4f", self.closeFrameTime))
            end
        end
    end
    
    Log("[ScrollOpen] Initialized - scrollID=" .. tostring(self.scrollID) ..
        ", totalFrames=" .. self.totalFrames ..
        ", expected duration=" .. (self.openDuration + self.holdDuration + self.closeDuration) .. "s")
end

-- ============================================================================
-- START ANIMATION
-- ============================================================================

function TurnScrollUI:StartAnimation()
    Log("[ScrollOpen] Starting animation (open: " .. self.openDuration .. "s, hold: " .. self.holdDuration .. "s, close: " .. self.closeDuration .. "s)")
    self.state = "opening"
    self.currentFrame = 0
    self.frameTimer = 0.0
    self.holdTimer = 0.0

    if self.scrollID and self.scrollID > 0 then
        SetSpriteVisibility(self.scrollID, true)
        if SetAnimationFrame then
            SetAnimationFrame(self.scrollID, 0)
        end
        if SetAnimationPlaying then
            SetAnimationPlaying(self.scrollID, false)  -- We control frames manually
        end
    end
end

-- ============================================================================
-- UPDATE
-- ============================================================================

function TurnScrollUI:Update(dt, cameraPos)
    if not self.enabled then return end
    if not self.scrollID or self.scrollID == 0 then return end
    
    -- Prevent C++ animation system from auto-playing
    if SetAnimationPlaying then
        SetAnimationPlaying(self.scrollID, false)
    end
    if SetAnimationGroup then
        SetAnimationGroup(self.scrollID, 0)
    end
    if SetAnimationDirection then
        SetAnimationDirection(self.scrollID, 3)
    end

    -- Keep scroll anchored to camera (keeps UI fixed on screen)
    if self:ShouldUpdatePosition(cameraPos) then
        local scrollX = cameraPos.x + self.offsetX
        local scrollY = cameraPos.y + self.offsetY
        SetSpritePosition(self.scrollID, scrollX, scrollY)
        self.scrollWorldX = scrollX
        self.scrollWorldY = scrollY
    end

    local phase = GetCurrentTurn() or "Player"

    if self.state == "idle" then
        -- Check if we should trigger the animation
        local shouldTrigger = false
        
        if not self.hasTriggeredOnce and phase == "Player" then
            shouldTrigger = true
            self.hasTriggeredOnce = true
            Log("[ScrollOpen] First trigger")
        elseif self.lastTurnPhase == "Enemy" and phase == "Player" then
            shouldTrigger = true
            Log("[ScrollOpen] Turn change: Enemy -> Player")
        end
        
        if shouldTrigger then
            self:StartAnimation()
        end
        
        self.lastTurnPhase = phase
        
    elseif self.state == "opening" then
        -- Opening animation phase (0.75s)
        self.frameTimer = self.frameTimer + dt
        
        while self.frameTimer >= self.openFrameTime do
            self.frameTimer = self.frameTimer - self.openFrameTime
            self.currentFrame = self.currentFrame + 1
            
            if self.currentFrame > self.openEndFrame then
                -- Opening complete, transition to holding
                self.currentFrame = self.openEndFrame
                self.state = "holding"
                self.holdTimer = 0.0
                Log("[ScrollOpen] Opening complete, now holding for " .. self.holdDuration .. "s")
                break
            end
            
            if SetAnimationFrame then
                SetAnimationFrame(self.scrollID, self.currentFrame)
            end
        end
        
    elseif self.state == "holding" then
        -- Hold phase - scroll stays open (1.5s)
        self.holdTimer = self.holdTimer + dt
        
        if self.holdTimer >= self.holdDuration then
            -- Hold complete, start closing
            self.state = "closing"
            self.currentFrame = self.closeStartFrame
            self.frameTimer = 0.0
            Log("[ScrollOpen] Hold complete, now closing")
            
            if SetAnimationFrame then
                SetAnimationFrame(self.scrollID, self.currentFrame)
            end
        end
        
    elseif self.state == "closing" then
        -- Closing animation phase (0.75s)
        self.frameTimer = self.frameTimer + dt
        
        while self.frameTimer >= self.closeFrameTime do
            self.frameTimer = self.frameTimer - self.closeFrameTime
            self.currentFrame = self.currentFrame + 1
            
            if self.currentFrame > self.lastFrame then
                -- Animation complete
                self.state = "idle"
                self.currentFrame = 0
                SetSpriteVisibility(self.scrollID, false)
                if SetAnimationFrame then
                    SetAnimationFrame(self.scrollID, 0)
                end
                Log("[ScrollOpen] Animation complete")
                break
            end
            
            if SetAnimationFrame then
                SetAnimationFrame(self.scrollID, self.currentFrame)
            end
        end
    end
end

-- ============================================================================
-- DRAW
-- ============================================================================

function TurnScrollUI:Draw()
    if not self.enabled then return end
    
    -- Don't draw if game is paused (prevent text from showing through pause menu)
    if IsPaused and IsPaused() then
        return
    end
    
    -- Only show text during holding phase (when scroll is fully open)
    -- or during late opening / early closing when scroll is mostly open
    local showText = false
    
    if self.state == "holding" then
        -- Always show text during hold phase
        showText = true
    elseif self.state == "opening" and self.currentFrame >= self.textStartFrame then
        -- Show text in late opening phase
        showText = true
    elseif self.state == "closing" and self.currentFrame <= self.textEndFrame then
        -- Show text in early closing phase
        showText = true
    end
    
    if not showText then
        return
    end

    -- Get screen dimensions
    local fbW, fbH = GetFramebufferSize()
    if not fbW or not fbH then
        return
    end

    -- Calculate text position - center of screen (where scroll is)
    -- Scroll is at camera center + offsetY, so text should also be centered
    local textX = fbW * 0.5
    local textY = fbH * 0.5 + 40  -- Move up to center in scroll
    
    -- Approximate text width to center it horizontally
    local approxTextWidth = #self.text * self.textBaseSize * self.textWidthFactor * self.textScale
    textX = textX - (approxTextWidth * 0.5)
    
    -- Get text color
    local r = tonumber(self.textColor[1]) or 0
    local g = tonumber(self.textColor[2]) or 0
    local b = tonumber(self.textColor[3]) or 0

    DrawText(
        self.textFont,
        self.text,
        textX, textY,
        self.textScale,
        r, g, b
    )
end

-- ============================================================================
-- QUERIES
-- ============================================================================

function TurnScrollUI:IsPlaying()
    -- Animation is playing during opening, holding, or closing phases
    return self.state == "opening" or self.state == "holding" or self.state == "closing"
end

-- ============================================================================
-- CLEANUP
-- ============================================================================

function TurnScrollUI:Destroy()
    UIComponent.Destroy(self)
    self.scrollID = 0
end

return TurnScrollUI
