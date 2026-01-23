-- ============================================================================
-- AttackAPIndicatorUI.lua
-- Attack Action Point (AP) indicator component
-- ============================================================================
-- Displays attack AP as a row of diamonds
-- Uses tint-based single layer by default (no empty sprite)
-- ============================================================================

local UIComponent = require("UI/UIComponent")
local AttackAPIndicatorUI = {}
setmetatable(AttackAPIndicatorUI, {__index = UIComponent})
AttackAPIndicatorUI.__index = AttackAPIndicatorUI

-- ============================================================================
-- CONSTRUCTOR
-- ============================================================================

function AttackAPIndicatorUI:New()
    local instance = UIComponent:New()
    setmetatable(instance, self)
    return instance
end

-- ============================================================================
-- INITIALIZATION
-- ============================================================================

function AttackAPIndicatorUI:Init(config)
    self.config = config or {}

    -- Configuration
    self.maxAP = self.config.maxAP or 3
    self.indicatorSize = self.config.size or 0.06
    self.indicatorSpacing = self.config.spacing or 0.1
    self.offsetX = self.config.offsetX or -0.64
    self.offsetY = self.config.offsetY or -0.32
    self.layer = self.config.layer or 4
    self.emptyLayerOffset = self.config.emptyLayerOffset or 0
    self.filledLayerOffset = self.config.filledLayerOffset or 1

    -- Textures
    self.emptyTexture = self.config.emptyTexture or "assets/UI/AP_Empty.png"
    self.filledTexture = self.config.filledTexture or "assets/UI/AP_Crystal.png"

    -- Tint configuration
    self.useTint = self.config.useTint
    if self.useTint == nil then
        self.useTint = true
    end
    self.filledTint = self.config.filledTint or { r = 1.0, g = 1.0, b = 1.0 }
    self.emptyTint = self.config.emptyTint or self.filledTint
    self.useGray = self.config.useGray or false
    self.filledGrayAmount = self.config.filledGrayAmount or 0.0
    self.emptyGrayAmount = self.config.emptyGrayAmount or 1.0

    -- State
    self.indicators = {}
    self.emptyIndicators = {}
    self.filledIndicators = {}
    self.lastKnownAP = 0

    -- Get current AP
    local currentAP = self:GetCurrentAP()
    self.lastKnownAP = math.min(currentAP or 0, self.maxAP)

    -- Get camera position for initial placement
    local camX, camY, camZ = GetCameraPosition()

    if self.useTint then
        -- Single-layer: tint diamonds based on AP
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
        -- Two-layer: empty background + filled foreground
        for i = 1, self.maxAP do
            local xPos = camX + self.offsetX + ((i - 1) * self.indicatorSpacing)
            local yPos = camY + self.offsetY

            local entityID = self:SpawnSprite(
                self.emptyTexture,
                xPos, yPos,
                self.indicatorSize, self.indicatorSize,
                self.layer + self.emptyLayerOffset
            )

            self.emptyIndicators[i] = entityID
        end

        for i = 1, self.lastKnownAP do
            local xPos = camX + self.offsetX + ((i - 1) * self.indicatorSpacing)
            local yPos = camY + self.offsetY

            local entityID = self:SpawnSprite(
                self.filledTexture,
                xPos, yPos,
                self.indicatorSize, self.indicatorSize,
                self.layer + self.filledLayerOffset
            )

            self.filledIndicators[i] = entityID

            if entityID and entityID > 0 then
                self:ApplyVisual(entityID, self.filledTint, self.filledGrayAmount)
            end
        end
    end

    Log("[AttackAPIndicatorUI] Initialized - " .. self.lastKnownAP .. "/" .. self.maxAP .. " AP")
end

-- ============================================================================
-- UPDATE
-- ============================================================================

function AttackAPIndicatorUI:Update(dt, cameraPos)
    if not self.enabled then return end

    -- Get current AP
    local currentAP = self:GetCurrentAP()
    if currentAP == nil then return end

    currentAP = math.min(currentAP, self.maxAP)

    -- Update sprite positions if camera moved
    if self:ShouldUpdatePosition(cameraPos) then
        self:UpdatePositions(cameraPos)
    end

    -- Handle AP changes
    if currentAP ~= self.lastKnownAP then
        if self.useTint then
            self:UpdateTints(currentAP)
        else
            self:HandleAPChange(currentAP, cameraPos)
        end
        self.lastKnownAP = currentAP
    end
end

-- ============================================================================
-- HELPER METHODS
-- ============================================================================

function AttackAPIndicatorUI:GetCurrentAP()
    local currentAP, maxPlayerAP = GetPlayerAttackAP()
    return currentAP or 0
end

function AttackAPIndicatorUI:UpdatePositions(cameraPos)
    if self.useTint then
        for i = 1, #self.indicators do
            local entityID = self.indicators[i]
            if entityID and entityID > 0 then
                local xPos = cameraPos.x + self.offsetX + ((i - 1) * self.indicatorSpacing)
                local yPos = cameraPos.y + self.offsetY
                SetSpritePosition(entityID, xPos, yPos)
            end
        end
    else
        for i = 1, #self.emptyIndicators do
            local entityID = self.emptyIndicators[i]
            if entityID and entityID > 0 then
                local xPos = cameraPos.x + self.offsetX + ((i - 1) * self.indicatorSpacing)
                local yPos = cameraPos.y + self.offsetY
                SetSpritePosition(entityID, xPos, yPos)
            end
        end

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

function AttackAPIndicatorUI:HandleAPChange(newAP, cameraPos)
    newAP = math.max(0, math.min(newAP, self.maxAP))

    if newAP < self.lastKnownAP then
        -- AP decreased - destroy extra diamonds
        for i = self.lastKnownAP, newAP + 1, -1 do
            if self.filledIndicators[i] then
                DestroyEntity(self.filledIndicators[i])
                self.filledIndicators[i] = nil
            end
        end
    elseif newAP > self.lastKnownAP then
        -- AP increased - create new diamonds
        for i = self.lastKnownAP + 1, newAP do
            local xPos = cameraPos.x + self.offsetX + ((i - 1) * self.indicatorSpacing)
            local yPos = cameraPos.y + self.offsetY

            local entityID = self:SpawnSprite(
                self.filledTexture,
                xPos, yPos,
                self.indicatorSize, self.indicatorSize,
                self.layer + self.filledLayerOffset
            )

            self.filledIndicators[i] = entityID

            if entityID and entityID > 0 then
                self:ApplyTint(entityID, self.filledTint)
            end
        end
    end
end

function AttackAPIndicatorUI:UpdateTints(currentAP)
    for i = 1, #self.indicators do
        local entityID = self.indicators[i]
        if entityID and entityID > 0 then
            local tint = (i <= currentAP) and self.filledTint or self.emptyTint
            local grayAmount = (i <= currentAP) and self.filledGrayAmount or self.emptyGrayAmount
            self:ApplyVisual(entityID, tint, grayAmount)
        end
    end
end

function AttackAPIndicatorUI:ApplyVisual(entityID, tint, grayAmount)
    self:ApplyTint(entityID, tint)
    if self.useGray then
        SetSpriteGray(entityID, grayAmount or 0.0)
    end
end

function AttackAPIndicatorUI:ApplyTint(entityID, tint)
    if not entityID or entityID <= 0 or not tint then
        return
    end

    local r = tint.r or tint[1] or 1.0
    local g = tint.g or tint[2] or 1.0
    local b = tint.b or tint[3] or 1.0
    SetSpriteColor(entityID, r, g, b)
end

function AttackAPIndicatorUI:Destroy()
    if self.useTint then
        for i = 1, #self.indicators do
            if self.indicators[i] and self.indicators[i] > 0 then
                DestroyEntity(self.indicators[i])
            end
        end
        self.indicators = {}
    else
        for i = 1, #self.emptyIndicators do
            if self.emptyIndicators[i] and self.emptyIndicators[i] > 0 then
                DestroyEntity(self.emptyIndicators[i])
            end
        end

        for i = 1, #self.filledIndicators do
            if self.filledIndicators[i] and self.filledIndicators[i] > 0 then
                DestroyEntity(self.filledIndicators[i])
            end
        end
    end

    self.emptyIndicators = {}
    self.filledIndicators = {}

    Log("[AttackAPIndicatorUI] Destroyed")
end

return AttackAPIndicatorUI
