-- ============================================================================
-- TurnIndicatorUI.lua
-- Turn phase indicator component
-- ============================================================================
-- Displays current turn phase (Player/Enemy) with overlapping sprites
-- Base sprite (enemy) always visible, player sprite overlays during player turn
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

    -- Configuration
    self.offsetX = self.config.offsetX or 0.65
    self.offsetY = self.config.offsetY or 0.38
    self.scaleX = self.config.scaleX or 0.28
    self.scaleY = self.config.scaleY or 0.28
    self.layer = self.config.layer or 4

    -- Textures
    self.enemyTexture = self.config.enemyTexture or "assets/UI/Enemy_Turn_Icon.png"
    self.playerTexture = self.config.playerTexture or "assets/UI/Player_Turn_Icon.png"

    -- State
    self.enemyID = 0
    self.playerID = 0
    self.lastTurnPhase = ""

    -- Get camera position
    local camX, camY, camZ = GetCameraPosition()
    local turnX = camX + self.offsetX
    local turnY = camY + self.offsetY

    -- Get current turn phase
    local phase = GetCurrentTurn() or "Player"
    self.lastTurnPhase = phase

    -- Create enemy base sprite (always visible)
    self.enemyID = self:SpawnSprite(
        self.enemyTexture,
        turnX, turnY,
        self.scaleX, self.scaleY,
        self.layer
    )

    -- Create player overlay sprite (shown only during player phase)
    self.playerID = self:SpawnSprite(
        self.playerTexture,
        turnX, turnY,
        self.scaleX, self.scaleY,
        self.layer
    )

    -- Set initial visibility
    if phase == "Player" then
        SetSpriteVisibility(self.enemyID, false)
        SetSpriteVisibility(self.playerID, true)
    else
        SetSpriteVisibility(self.enemyID, true)
        SetSpriteVisibility(self.playerID, false)
    end

    Log("[TurnIndicatorUI] Initialized - Phase: " .. phase)
end

-- ============================================================================
-- UPDATE
-- ============================================================================

function TurnIndicatorUI:Update(dt, cameraPos)
    if not self.enabled then return end

    -- Get current turn phase
    local phase = GetCurrentTurn() or "Player"

    -- Update sprite positions if camera moved (keeps UI fixed on screen)
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

    if self.enemyID and self.enemyID > 0 then
        SetSpritePosition(self.enemyID, turnX, turnY)
    end

    if self.playerID and self.playerID > 0 then
        SetSpritePosition(self.playerID, turnX, turnY)
    end
end

function TurnIndicatorUI:HandlePhaseChange(newPhase)
    if newPhase == "Player" then
        -- Player turn: show player overlay
        SetSpriteVisibility(self.enemyID, false)
        SetSpriteVisibility(self.playerID, true)
    else
        -- Enemy turn: hide player overlay
        SetSpriteVisibility(self.enemyID, true)
        SetSpriteVisibility(self.playerID, false)
    end

    Log("[TurnIndicatorUI] Phase changed: " .. self.lastTurnPhase .. " -> " .. newPhase)
end

function TurnIndicatorUI:Destroy()
    if self.enemyID and self.enemyID > 0 then
        DestroyEntity(self.enemyID)
    end

    if self.playerID and self.playerID > 0 then
        DestroyEntity(self.playerID)
    end

    self.enemyID = 0
    self.playerID = 0

    Log("[TurnIndicatorUI] Destroyed")
end

return TurnIndicatorUI
