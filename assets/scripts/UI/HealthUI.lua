-- ============================================================================
-- HealthUI.lua
-- Player health display component
-- ============================================================================
-- Displays player HP as a single heart sprite with multiple textures
-- Swaps texture based on current HP (Health_0.png to Health_5.png)
-- ============================================================================

local UIComponent = require("UI/UIComponent")
local HealthUI = {}
setmetatable(HealthUI, {__index = UIComponent})
HealthUI.__index = HealthUI

-- ============================================================================
-- CONSTRUCTOR
-- ============================================================================

function HealthUI:New()
    local instance = UIComponent:New()
    setmetatable(instance, self)
    return instance
end

-- ============================================================================
-- INITIALIZATION
-- ============================================================================

function HealthUI:Init(config)
    self.config = config or {}

    -- Configuration
    self.maxHP = self.config.maxHP or 5
    self.scale = self.config.scale or 0.10
    self.offsetX = self.config.offsetX or -0.72
    self.offsetY = self.config.offsetY or -0.22
    self.layer = self.config.layer or 4

    -- Texture base path
    self.textureBasePath = self.config.textureBasePath or "assets/UI/Health_"

    -- State
    self.heartSprites = {}  -- Map HP value to entity ID
    self.lastHP = -1

    -- Get camera position
    local camX, camY, camZ = GetCameraPosition()

    -- Get current HP
    local curHP, maxHP = GetPlayerHP()
    if maxHP and maxHP > 0 then
        self.maxHP = maxHP
    end
    if not curHP then
        curHP = self.maxHP
    end
    self.lastHP = curHP

    -- Create all heart sprites (one for each HP value)
    for hpVal = 0, self.maxHP do
        local texture = self.textureBasePath .. tostring(hpVal) .. ".png"
        local x = camX + self.offsetX
        local y = camY + self.offsetY

        local entityID = self:SpawnSprite(texture, x, y, self.scale, self.scale, self.layer)
        self.heartSprites[hpVal] = entityID

        -- Initially hide all except current HP
        SetSpriteVisibility(entityID, hpVal == curHP)
    end

    Log("[HealthUI] Initialized - " .. curHP .. "/" .. self.maxHP .. " HP")
end

-- ============================================================================
-- UPDATE
-- ============================================================================

function HealthUI:Update(dt, cameraPos)
    if not self.enabled then return end

    -- Get current HP
    local curHP, maxHP = GetPlayerHP()
    if not curHP then return end

    -- Update sprite positions if camera moved
    if self:ShouldUpdatePosition(cameraPos) then
        self:UpdatePositions(cameraPos)
    end

    -- Handle HP changes
    if curHP ~= self.lastHP then
        self:HandleHPChange(curHP)
        self.lastHP = curHP
    end
end

-- ============================================================================
-- HELPER METHODS
-- ============================================================================

function HealthUI:UpdatePositions(cameraPos)
    local x = cameraPos.x + self.offsetX
    local y = cameraPos.y + self.offsetY

    for hpVal, entityID in pairs(self.heartSprites) do
        if entityID and entityID > 0 then
            SetSpritePosition(entityID, x, y)
        end
    end
end

function HealthUI:HandleHPChange(newHP)
    -- Hide all hearts
    for _, entityID in pairs(self.heartSprites) do
        if entityID and entityID > 0 then
            SetSpriteVisibility(entityID, false)
        end
    end

    -- Show heart matching current HP
    if self.heartSprites[newHP] then
        SetSpriteVisibility(self.heartSprites[newHP], true)
    end

    Log("[HealthUI] HP changed: " .. self.lastHP .. " -> " .. newHP)
end

function HealthUI:Destroy()
    for _, entityID in pairs(self.heartSprites) do
        if entityID and entityID > 0 then
            DestroyEntity(entityID)
        end
    end

    self.heartSprites = {}
    Log("[HealthUI] Destroyed")
end

return HealthUI
