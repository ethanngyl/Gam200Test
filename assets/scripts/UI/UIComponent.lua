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

-- Spawn an animated sprite with sprite sheet
-- @param texture: path to sprite sheet
-- @param x, y: position
-- @param scaleX, scaleY: size
-- @param layer: render layer
-- @param rows, columns: sprite sheet grid dimensions
-- @param frameCount: total frames to use (default: rows * columns)
-- @param frameTime: seconds per frame (default: 0.1)
-- @param loop: whether to loop animation (default: true)
function UIComponent:SpawnAnimatedSprite(texture, x, y, scaleX, scaleY, layer, rows, columns, frameCount, frameTime, loop)
    rows = rows or 1
    columns = columns or 1
    frameCount = frameCount or (rows * columns)
    frameTime = frameTime or 0.1
    if loop == nil then loop = true end
    
    local entityID = SpawnAnimatedSprite(texture, x, y, scaleX, scaleY, layer, rows, columns, frameCount, frameTime, loop)
    if entityID > 0 then
        table.insert(self.entities, entityID)
        SetSpriteFilterMode(entityID, true)
    end
    return entityID
end

-- Add animation to an existing sprite entity
function UIComponent:SetSpriteAnimation(entityID, texture, rows, columns, frameCount, frameTime, loop)
    if not entityID or entityID <= 0 then return false end
    rows = rows or 1
    columns = columns or 1
    frameCount = frameCount or (rows * columns)
    frameTime = frameTime or 0.1
    if loop == nil then loop = true end
    
    return SetSpriteAnimationSheet(entityID, texture, rows, columns, frameCount, frameTime, loop)
end

-- Set animation frame range for an entity
-- @param entityID: the entity ID
-- @param startFrame: first frame index in the animation range
-- @param frameCount: number of frames in the animation
-- @param resetToStart: (optional) if true, reset currentFrame to 0 (default: true)
function UIComponent:SetAnimationFrameRange(entityID, startFrame, frameCount, resetToStart)
    if not entityID or entityID <= 0 then return end
    if resetToStart == nil then resetToStart = true end
    SetAnimationFrameRange(entityID, startFrame, frameCount, resetToStart)
end

-- Set animation loop mode for an entity
function UIComponent:SetAnimationLoop(entityID, loop)
    if not entityID or entityID <= 0 then return end
    SetAnimationLoop(entityID, loop)
end

function UIComponent:SetEnabled(enabled)
    self.enabled = enabled

    -- Hide/show all UI entities so they stop rendering during enemy phase.
    for _, entityID in ipairs(self.entities) do
        if entityID and entityID > 0 then
            if SetSpriteVisibility ~= nil then
                SetSpriteVisibility(entityID, enabled)
            elseif SetSpriteColor ~= nil then
                -- Fallback: alpha to 0 to hide
                if enabled then
                    SetSpriteColor(entityID, 1.0, 1.0, 1.0, 1.0)
                else
                    SetSpriteColor(entityID, 1.0, 1.0, 1.0, 0.0)
                end
            elseif SetSpritePosition ~= nil then
                -- Last resort: move offscreen
                if not enabled then
                    SetSpritePosition(entityID, 999999.0, 999999.0)
                end
            end
        end
    end

    -- When re-enabled, force an immediate refresh (so UI snaps back correctly)
    if enabled then
        self.lastCamX = nil
        self.lastCamY = nil
        self.lastCamZ = nil
    end
end


function UIComponent:IsEnabled()
    return self.enabled
end

return UIComponent
