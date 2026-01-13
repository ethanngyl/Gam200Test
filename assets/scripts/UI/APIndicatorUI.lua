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
    self.emptyTexture = self.config.emptyTexture or "assets/UI/MovP_Black.png"
    self.filledTexture = self.config.filledTexture or "assets/UI/MovP.png"

    -- State
    self.emptyIndicators = {}
    self.filledIndicators = {}
    self.lastKnownAP = 0

    -- Get camera position for initial placement
    local camX, camY, camZ = GetCameraPosition()

    -- Create empty crystal background layer
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
    end

    -- Get current AP
    local currentAP, maxPlayerAP = GetPlayerAP()
    if maxPlayerAP and maxPlayerAP > 0 then
        self.maxAP = maxPlayerAP
    end
    self.lastKnownAP = currentAP or 0

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
    end

    Log("[APIndicatorUI] Initialized - " .. self.lastKnownAP .. "/" .. self.maxAP .. " AP")
end

-- ============================================================================
-- UPDATE
-- ============================================================================

function APIndicatorUI:Update(dt, cameraPos)
    if not self.enabled then return end

    -- Get current AP
    local currentAP, maxPlayerAP = GetPlayerAP()
    if not currentAP then return end

    -- Update sprite positions if camera moved
    if self:ShouldUpdatePosition(cameraPos) then
        self:UpdatePositions(cameraPos)
    end

    -- Handle AP changes
    if currentAP ~= self.lastKnownAP then
        self:HandleAPChange(currentAP, cameraPos)
        self.lastKnownAP = currentAP
    end
end

-- ============================================================================
-- HELPER METHODS
-- ============================================================================

function APIndicatorUI:UpdatePositions(cameraPos)
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
        end
    end
end

function APIndicatorUI:Destroy()
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

    Log("[APIndicatorUI] Destroyed")
end

return APIndicatorUI
