--[[
===============================================================================
 File:          HealthUI.lua
 Authors:       
 Co-Authors:    
 Date:          
 Contribution:  
 ------------------------------------------------------------------------------

 HEALTH UI - Player Health Display Component

 Brief:
    Displays player HP as a single heart sprite with multiple textures.
    Swaps texture based on current HP (Health_0.png to Health_5.png).
    Tracks the active character from the party system and updates the
    display when HP changes. Maintains UI position relative to camera.

 Features:
    - Single sprite approach with texture swapping
    - Supports HP values 0-5 (configurable max)
    - Camera-relative positioning
    - Party system integration (tracks active character)
    - Smooth handling during enemy turns

 Usage:
    local HealthUI = require("UI/HealthUI")
    
    local healthDisplay = HealthUI:New()
    healthDisplay:Init({
        maxHP = 5,
        scale = 0.10,
        offsetX = -0.72,
        offsetY = -0.22,
        layer = 4,
        textureBasePath = "assets/UI/Health_"
    })

    -- In update loop
    healthDisplay:Update(dt, cameraPos)

    -- Cleanup
    healthDisplay:Destroy()


 Copyright (C) 2026 DigiPen Institute of Technology.
 Reproduction or disclosure of this file or its contents
 without the prior written consent of DigiPen Institute of
 Technology is prohibited.
===============================================================================
]]--

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
    self.healthSpriteID = 0  -- Single sprite that we'll change texture on
    self.lastHP = -1
    self.trackedCharID = 0   -- Currently tracked character ID

    -- Get camera position
    local camX, camY, camZ = GetCameraPosition()

    -- Get character to track: try party system first, fallback to FindPlayer
    local charID = nil
    if GetActiveCharacter then
        charID = GetActiveCharacter()
    end
    if (not charID or charID <= 0) and FindPlayer then
        charID = FindPlayer()
    end
    
    self.trackedCharID = charID or 0
    local curHP, maxHP = 5, 5  -- Default
    if charID and charID > 0 then
        curHP, maxHP = GetEntityHP(charID)
    end
    if maxHP and maxHP > 0 then
        self.maxHP = maxHP
    end
    if not curHP then
        curHP = self.maxHP
    end
    self.lastHP = curHP

    -- Create a SINGLE health sprite with the current HP texture
    local texture = self.textureBasePath .. tostring(curHP) .. ".png"
    local x = camX + self.offsetX
    local y = camY + self.offsetY

    self.healthSpriteID = self:SpawnSprite(texture, x, y, self.scale, self.scale, self.layer)

    Log("[HealthUI] Initialized - " .. curHP .. "/" .. self.maxHP .. " HP (single sprite approach)")
end

-- ============================================================================
-- UPDATE
-- ============================================================================

function HealthUI:Update(dt, cameraPos)
    if not self.enabled then return end

    -- Get character to display HP for
    -- Priority: 1) Active character from party system, 2) Keep last tracked character during enemy turn
    local charID = nil
    local currentTurn = GetCurrentTurn and GetCurrentTurn() or "Player"
    
    if GetActiveCharacter then
        charID = GetActiveCharacter()
    end
    
    -- During enemy turn, keep tracking the last active character (don't switch)
    -- This prevents the UI from jumping to a different character during enemy attacks
    if currentTurn == "Enemy" and self.trackedCharID and self.trackedCharID > 0 then
        -- Keep the last tracked character during enemy turn
        charID = self.trackedCharID
    end
    
    -- Fallback to FindPlayer only if we have no tracked character at all
    if (not charID or charID <= 0) and (not self.trackedCharID or self.trackedCharID <= 0) then
        if FindPlayer then
            charID = FindPlayer()
        end
    end
    
    -- Still no character? Use last tracked character if valid
    if (not charID or charID <= 0) and self.trackedCharID and self.trackedCharID > 0 then
        charID = self.trackedCharID
    end
    
    if not charID or charID <= 0 then
        return
    end

    -- Check if tracked character changed (e.g., player switched characters)
    -- Only update tracking during player turn to avoid confusion during enemy turn
    if currentTurn == "Player" and self.trackedCharID ~= charID then
        self.trackedCharID = charID
        -- Force update when character changes
        self.lastHP = -1
    end

    -- Always use the tracked character for HP display
    local displayCharID = self.trackedCharID
    if not displayCharID or displayCharID <= 0 then
        displayCharID = charID
    end

    local curHP, maxHP = GetEntityHP(displayCharID)
    if not curHP then
        -- Entity might have been destroyed or has no Health component
        return
    end

    -- Update sprite positions if camera moved (keeps UI fixed on screen)
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

    if self.healthSpriteID and self.healthSpriteID > 0 then
        SetSpritePosition(self.healthSpriteID, x, y)
    end
end

function HealthUI:HandleHPChange(newHP)
    -- Clamp HP to valid range
    if newHP < 0 then newHP = 0 end
    if newHP > self.maxHP then newHP = self.maxHP end

    -- Change the texture on our single sprite
    local newTexture = self.textureBasePath .. tostring(newHP) .. ".png"

    if self.healthSpriteID and self.healthSpriteID > 0 and SetSpriteTexture then
        SetSpriteTexture(self.healthSpriteID, newTexture)
    end

    Log("[HealthUI] HP changed: " .. self.lastHP .. " -> " .. newHP)
end

function HealthUI:Destroy()
    if self.healthSpriteID and self.healthSpriteID > 0 then
        DestroyEntity(self.healthSpriteID)
    end

    self.healthSpriteID = 0
    Log("[HealthUI] Destroyed")
end

return HealthUI
