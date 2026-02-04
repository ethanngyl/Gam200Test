-- ============================================================================
-- TurnIndicatorUI.lua
-- Turn phase indicator component (Animated Version)
-- ============================================================================
-- Displays current turn phase (Player/Enemy) using animated sprite sheet
-- Row 1: Player turn animation (loops)
-- Row 2: Enemy turn animation (loops)
-- ============================================================================

local UIComponent = require("UI/UIComponent")
local TurnIndicatorUI = {}
setmetatable(TurnIndicatorUI, {__index = UIComponent})
TurnIndicatorUI.__index = TurnIndicatorUI

-- ============================================================================
-- CONSTRUCTOR
-- ============================================================================

function TurnIndicatorUI:New()
    local instance = UIComponent:New()
    setmetatable(instance, self)
    return instance
end

-- ============================================================================
-- INITIALIZATION
-- ============================================================================

function TurnIndicatorUI:Init(config)
    self.config = config or {}

    -- Position and scale
    self.offsetX = self.config.offsetX or 0.65
    self.offsetY = self.config.offsetY or 0.38
    self.scaleX = self.config.scaleX or 0.28
    self.scaleY = self.config.scaleY or 0.28
    self.layer = self.config.layer or 4

    -- Sprite sheet configuration
    self.texture = self.config.texture or "assets/UI/End_Turn_Button.png"
    self.rows = self.config.rows or 2       -- 2 rows: player (row 0), enemy (row 1)
    self.cols = self.config.cols or 4       -- columns per row (adjust as needed)
    self.frameTime = self.config.frameTime or 0.15  -- seconds per frame

    -- Calculate frames per row
    self.framesPerRow = self.cols

    -- State
    self.spriteID = 0
    self.lastTurnPhase = ""

    -- Get camera position
    local camX, camY, camZ = GetCameraPosition()
    local turnX = camX + self.offsetX
    local turnY = camY + self.offsetY

    -- Get current turn phase
    local phase = GetCurrentTurn() or "Player"
    self.lastTurnPhase = phase

    -- Create animated sprite
    self.spriteID = SpawnAnimatedSprite(
        self.texture,
        turnX, turnY,
        self.scaleX, self.scaleY,
        self.layer,
        self.rows,
        self.cols,
        self.rows * self.cols,  -- total frames
        self.frameTime,
        true  -- loop
    )

    -- Set initial animation row based on current phase
    self:SetAnimationRow(phase)

    Log("[TurnIndicatorUI] Initialized with animated sprite - Phase: " .. phase)
end

-- ============================================================================
-- UPDATE
-- ============================================================================

function TurnIndicatorUI:Update(dt, cameraPos)
    if not self.enabled then return end

    -- Get current turn phase
    local phase = GetCurrentTurn() or "Player"

    -- Update sprite position if camera moved
    if self:ShouldUpdatePosition(cameraPos) then
        self:UpdatePositions(cameraPos)
    end

    -- Handle turn phase changes
    if phase ~= self.lastTurnPhase then
        self:HandlePhaseChange(phase)
        self.lastTurnPhase = phase
    end
end

-- ============================================================================
-- HELPER METHODS
-- ============================================================================

function TurnIndicatorUI:UpdatePositions(cameraPos)
    local turnX = cameraPos.x + self.offsetX
    local turnY = cameraPos.y + self.offsetY

    if self.spriteID and self.spriteID > 0 then
        SetSpritePosition(self.spriteID, turnX, turnY)
    end
end

function TurnIndicatorUI:SetAnimationRow(phase)
    if not self.spriteID or self.spriteID <= 0 then return end

    local startFrame, frameCount

    if phase == "Player" then
        -- Row 0: frames 0 to (cols-1)
        startFrame = 0
        frameCount = self.framesPerRow
    else
        -- Row 1: frames cols to (2*cols-1)
        startFrame = self.framesPerRow
        frameCount = self.framesPerRow
    end

    -- Set animation frame range and ensure it loops
    SetAnimationFrameRange(self.spriteID, startFrame, frameCount, true)
    SetAnimationLoop(self.spriteID, true)
    SetAnimationPlaying(self.spriteID, true)
end

function TurnIndicatorUI:HandlePhaseChange(newPhase)
    self:SetAnimationRow(newPhase)
    Log("[TurnIndicatorUI] Phase changed: " .. self.lastTurnPhase .. " -> " .. newPhase)
end

function TurnIndicatorUI:Destroy()
    if self.spriteID and self.spriteID > 0 then
        DestroyEntity(self.spriteID)
    end

    self.spriteID = 0
    Log("[TurnIndicatorUI] Destroyed")
end

return TurnIndicatorUI
