-- ============================================================================
-- UIComponent.lua
-- Base class for UI components
-- ============================================================================
-- Provides common functionality for camera-relative UI elements
-- All UI components should inherit from this
-- ============================================================================

local UIComponent = {}
UIComponent.__index = UIComponent

-- ============================================================================
-- CONSTRUCTOR
-- ============================================================================

function UIComponent:New()
    local instance = {
        entities = {},           -- Entity IDs managed by this component
        enabled = true,          -- Is this component active?
        lastCamX = nil,         -- Cache for optimization
        lastCamY = nil,
        lastCamZ = nil,
        updateThreshold = 0.01  -- Only update when camera moves > this amount
    }
    setmetatable(instance, self)
    return instance
end

-- ============================================================================
-- LIFECYCLE (Override these in child classes)
-- ============================================================================

function UIComponent:Init(config)
    -- Override this - initialize UI elements
    error("UIComponent:Init() must be overridden!")
end

function UIComponent:Update(dt, cameraPos)
    -- Override this - update UI state
    error("UIComponent:Update() must be overridden!")
end

function UIComponent:Destroy()
    -- Clean up all entities
    for _, entityID in ipairs(self.entities) do
        if entityID and entityID > 0 then
            DestroyEntity(entityID)
        end
    end
    self.entities = {}
end

-- ============================================================================
-- HELPER METHODS
-- ============================================================================

function UIComponent:ShouldUpdatePosition(cameraPos)
    -- Check if camera moved enough to warrant position update
    if self.lastCamX == nil then
        self.lastCamX = cameraPos.x
        self.lastCamY = cameraPos.y
        self.lastCamZ = cameraPos.z
        return true
    end

    local moved = math.abs(cameraPos.x - self.lastCamX) > self.updateThreshold or
                  math.abs(cameraPos.y - self.lastCamY) > self.updateThreshold or
                  math.abs(cameraPos.z - self.lastCamZ) > self.updateThreshold

    if moved then
        self.lastCamX = cameraPos.x
        self.lastCamY = cameraPos.y
        self.lastCamZ = cameraPos.z
    end

    return moved
end

function UIComponent:SpawnSprite(texture, x, y, scaleX, scaleY, layer)
    local entityID = SpawnSprite(texture, x, y, scaleX, scaleY, layer)
    if entityID > 0 then
        table.insert(self.entities, entityID)
        -- Use nearest-neighbor filtering to prevent black box artifacts
        -- Pixel art/UI should not use linear filtering which creates semi-transparent edge pixels
        SetSpriteFilterMode(entityID, true)
    end
    return entityID
end

function UIComponent:SetEnabled(enabled)
    self.enabled = enabled
end

function UIComponent:IsEnabled()
    return self.enabled
end

return UIComponent
