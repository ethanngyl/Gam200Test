-- ============================================================================
-- TurnScrollUI.lua (uses ScrollOpen.lua filename)
-- Player turn popup: open -> show text for 2.5s -> close
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
    self.openAnim = self.config.openAnim or "ScrollOpen"
    self.openFrameTime = self.config.openFrameTime or 0.03
    self.closeFrameTime = self.config.closeFrameTime or self.openFrameTime
    self.closeFallback = self.config.closeFallback or 1.2
    self.texture = self.config.texture or "assets/UI/ScrollOpen.png"
    self.animationConfigPath = self.config.animationConfigPath or "assets/JSON/animations.json"

    -- Hold (fully-open) section
    self.holdDuration = self.config.holdDuration or 2.5
    -- Defaults assume 9x4 (36) frames: last frame = 35
    self.openEndFrame = 35
    self.closeStartFrame = 35

    -- Text config
    self.text = self.config.text or "Your Turn"
    self.textFont = self.config.textFont or "Sans48"
    self.textScale = self.config.textScale or 0.9
    self.textColor = self.config.textColor or DEFAULT_TEXT_COLOR
    self.textOffsetX = self.config.textOffsetX or 0
    self.textOffsetY = self.config.textOffsetY or 0
    self.textAnchorX = self.config.textAnchorX
    self.textAnchorY = self.config.textAnchorY
    self.textStartRatio = self.config.textStartRatio or 0.4
    self.textEndRatio = self.config.textEndRatio or 0.6
    self.textBaseSize = self.config.textBaseSize or 48
    self.textWidthFactor = self.config.textWidthFactor or 0.6
    self.textAlign = self.config.textAlign or "center"

    -- State
    self.scrollID = 0
    self.state = "idle" -- idle -> opening -> holding -> closing
    self.timer = 0.0
    self.openFrameTimer = 0.0
    self.openFrameIndex = 0
    self.holdTimer = 0.0
    self.closeFrameTimer = 0.0
    self.closeFrameIndex = 0
    self.textStartFrame = 0
    self.textEndFrame = 0
    self.lastTurnPhase = GetCurrentTurn() or "Player"

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
            PlayAnimationByName(self.scrollID, self.openAnim, false)
        end
        if SetAnimationGroup then
            SetAnimationGroup(self.scrollID, 0) -- Idle
        end
        if SetAnimationDirection then
            SetAnimationDirection(self.scrollID, 3) -- None (prevents player idle swap)
        end
        if SetAnimationPlaying then
            SetAnimationPlaying(self.scrollID, false)
        end
        if SetAnimationFrame then
            SetAnimationFrame(self.scrollID, 0)
        end
        if GetAnimationFrameCount then
            local frameCount = GetAnimationFrameCount(self.scrollID)
            if frameCount and frameCount > 0 then
                local lastFrame = frameCount - 1
                self.openEndFrame = lastFrame
                self.closeStartFrame = lastFrame
            end
        end

        local startRatio = math.max(0.0, math.min(1.0, self.textStartRatio))
        local endRatio = math.max(0.0, math.min(1.0, self.textEndRatio))
        if endRatio < startRatio then
            local tmp = endRatio
            endRatio = startRatio
            startRatio = tmp
        end
        self.textStartFrame = math.floor(self.openEndFrame * startRatio)
        self.textEndFrame = math.floor(self.openEndFrame * endRatio)
    end
end

-- ============================================================================
-- STATE HELPERS
-- ============================================================================

function TurnScrollUI:StartOpen()
    self.state = "opening"
    self.timer = 0.0
    self.openFrameTimer = 0.0
    self.openFrameIndex = 0
    self.holdTimer = 0.0

    if self.scrollID and self.scrollID > 0 then
        SetSpriteVisibility(self.scrollID, true)
        if PlayAnimationByName then
            PlayAnimationByName(self.scrollID, self.openAnim, false)
        end
        if SetAnimationGroup then
            SetAnimationGroup(self.scrollID, 0) -- Idle
        end
        if SetAnimationDirection then
            SetAnimationDirection(self.scrollID, 3) -- None
        end
        if SetAnimationPlaying then
            SetAnimationPlaying(self.scrollID, false)
        end
        if SetAnimationFrame then
            SetAnimationFrame(self.scrollID, 0)
        end
    end
end

function TurnScrollUI:StartHold()
    self.state = "holding"
    self.holdTimer = 0.0

    if self.scrollID and self.scrollID > 0 then
        if SetAnimationPlaying then
            SetAnimationPlaying(self.scrollID, false)
        end
        if SetAnimationFrame then
            SetAnimationFrame(self.scrollID, self.openEndFrame)
        end
    end
end

function TurnScrollUI:StartClose()
    self.state = "closing"
    self.timer = 0.0
    self.closeFrameTimer = 0.0
    self.closeFrameIndex = self.closeStartFrame

    if self.scrollID and self.scrollID > 0 then
        if SetAnimationFrame then
            SetAnimationFrame(self.scrollID, self.closeStartFrame)
        end
        if SetAnimationPlaying then
            SetAnimationPlaying(self.scrollID, false)
        end
    end
end

-- ============================================================================
-- UPDATE
-- ============================================================================

function TurnScrollUI:Update(dt, cameraPos)
    if not self.enabled then return end
    if not self.scrollID or self.scrollID == 0 then return end
    if SetAnimationPlaying then
        SetAnimationPlaying(self.scrollID, false)
    end
    if SetAnimationGroup then
        SetAnimationGroup(self.scrollID, 0) -- Idle
    end
    if SetAnimationDirection then
        SetAnimationDirection(self.scrollID, 3) -- None
    end

    -- Keep scroll anchored to camera
    if self:ShouldUpdatePosition(cameraPos) then
        local scrollX = cameraPos.x + self.offsetX
        local scrollY = cameraPos.y + self.offsetY
        SetSpritePosition(self.scrollID, scrollX, scrollY)
        self.scrollWorldX = scrollX
        self.scrollWorldY = scrollY
    end

    local phase = GetCurrentTurn() or "Player"

    if self.state == "idle" then
        if self.lastTurnPhase ~= "Player" and phase == "Player" then
            self:StartOpen()
        end
    elseif self.state == "opening" then
        self.openFrameTimer = self.openFrameTimer + dt
        while self.openFrameTimer >= self.openFrameTime do
            self.openFrameTimer = self.openFrameTimer - self.openFrameTime
            self.openFrameIndex = self.openFrameIndex + 1
            if self.openFrameIndex >= self.openEndFrame then
                self:StartHold()
                break
            end
            if SetAnimationFrame then
                SetAnimationFrame(self.scrollID, self.openFrameIndex)
            end
        end

    elseif self.state == "holding" then
        self.holdTimer = self.holdTimer + dt

        if self.holdTimer >= self.holdDuration then
            self:StartClose()
        end
    elseif self.state == "closing" then
        self.timer = self.timer + dt
        self.closeFrameTimer = self.closeFrameTimer + dt
        while self.closeFrameTimer >= self.closeFrameTime do
            self.closeFrameTimer = self.closeFrameTimer - self.closeFrameTime
            self.closeFrameIndex = self.closeFrameIndex - 1
            if self.closeFrameIndex <= 0 then
                self.closeFrameIndex = 0
                if SetAnimationFrame then
                    SetAnimationFrame(self.scrollID, 0)
                end
                self.state = "idle"
                SetSpriteVisibility(self.scrollID, false)
                break
            end
            if SetAnimationFrame then
                SetAnimationFrame(self.scrollID, self.closeFrameIndex)
            end
        end

    end

    self.lastTurnPhase = phase
end

-- ============================================================================
-- DRAW
-- ============================================================================

function TurnScrollUI:Draw()
    if not self.enabled then return end
    if self.state ~= "opening" then return end
    if self.openFrameIndex < self.textStartFrame or self.openFrameIndex > self.textEndFrame then
        return
    end

    local fbW, fbH = GetFramebufferSize()
    if not fbW or not fbH then return end

    local textX = nil
    local textY = nil

    if WorldToScreen and self.scrollWorldX and self.scrollWorldY then
        local sx, sy = WorldToScreen(self.scrollWorldX, self.scrollWorldY)
        textX = sx + self.textOffsetX
        textY = sy + self.textOffsetY
    else
        local anchorX = self.textAnchorX
        local anchorY = self.textAnchorY
        if anchorX == nil then
            anchorX = 0.5 + self.offsetX
        end
        if anchorY == nil then
            anchorY = 0.5 + self.offsetY
        end

        textX = (fbW * anchorX) + self.textOffsetX
        textY = (fbH * anchorY) + self.textOffsetY
    end

    if self.textAlign == "center" then
        local approxWidth = #self.text * self.textBaseSize * self.textWidthFactor * self.textScale
        textX = textX - (approxWidth * 0.5)
    end

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
-- CLEANUP
-- ============================================================================

function TurnScrollUI:Destroy()
    UIComponent.Destroy(self)
    self.scrollID = 0
end

return TurnScrollUI
