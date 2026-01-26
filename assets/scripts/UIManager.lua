-- ============================================================================
-- UIManager.lua
-- Central UI coordination system
-- ============================================================================
-- Manages all UI components in a unified, organized way
-- Handles initialization, updates, and cleanup for all UI elements
-- ============================================================================

local UIManager = {}

-- Import UI components
local APIndicatorUI = require("UI/APIndicatorUI")
local AttackAPIndicatorUI = require("UI/AttackAPIndicatorUI")
local HealthUI = require("UI/HealthUI")
local TurnIndicatorUI = require("UI/TurnIndicatorUI")

-- ============================================================================
-- STATE
-- ============================================================================

UIManager.components = {}
UIManager.initialized = false
UIManager.cameraMoveThreshold = 0.01

-- ============================================================================
-- INITIALIZATION
-- ============================================================================

-- Helper function to get active character's movement AP (for party system)
local function GetActiveCharacterAP()
    if GetActiveCharacter then
        local activeEntityID = GetActiveCharacter()
        if activeEntityID and activeEntityID > 0 then
            return GetEntityAP(activeEntityID)
        end
    end
    -- Fallback to old single-player API
    return GetPlayerAP()
end

function UIManager.Init(config)
    config = config or {}

    Log("========================================")
    Log("[UIManager] Initializing UI System...")
    Log("========================================")

    -- Create Movement AP Indicator
    UIManager.components.movementAP = APIndicatorUI:New()
    UIManager.components.movementAP:Init({
        maxAP = 5,
        size = 0.06,
        spacing = 0.1,
        offsetX = -0.64,
        offsetY = -0.42,
        layer = 4,
        filledTexture = "assets/UI/MovP.png",
        useTint = true,
        useGray = true,
        emptyGrayAmount = 1.0,
        filledGrayAmount = 0.0,
        emptyTint = { r = 0.45, g = 0.45, b = 0.45 },
        getAPFunc = GetActiveCharacterAP  -- Use active character's AP
    })

    -- Create Attack AP Indicator
    UIManager.components.attackAP = AttackAPIndicatorUI:New()
    UIManager.components.attackAP:Init({
        maxAP = 3,
        size = 0.06,
        spacing = 0.1,
        offsetX = -0.64,
        offsetY = -0.32,
        layer = 4,
        filledTexture = "assets/UI/AP_Crystal.png",
        useTint = true,
        useGray = true,
        emptyGrayAmount = 1.0,
        filledGrayAmount = 0.0,
        emptyTint = { r = 0.45, g = 0.45, b = 0.45 },
        filledTint = { r = 0.75, g = 0.85, b = 1.0 }
    })

    -- Create Health UI
    UIManager.components.health = HealthUI:New()
    UIManager.components.health:Init({
        maxHP = 5,
        scale = 0.10,
        offsetX = -0.72,
        offsetY = -0.22,
        layer = 4,
        textureBasePath = "assets/UI/Health_"
    })

    -- Create Turn Indicator
    UIManager.components.turnIndicator = TurnIndicatorUI:New()
    UIManager.components.turnIndicator:Init({
        offsetX = 0.65,
        offsetY = 0.38,
        scaleX = 0.28,
        scaleY = 0.28,
        layer = 4,
        enemyTexture = "assets/UI/Enemy_Turn_Icon.png",
        playerTexture = "assets/UI/Player_Turn_Icon.png"
    })

    -- TODO: Add more components as needed:
    -- - ChestProgressUI
    -- - PlayerIconsUI (boots, sword)
    -- - MinimapUI
    -- - etc.

    UIManager.initialized = true

    Log("[UIManager] UI System initialized successfully")
    Log("  Components: " .. UIManager.GetComponentCount())
    Log("========================================")
end

-- ============================================================================
-- UPDATE
-- ============================================================================

function UIManager.Update(dt)
    if not UIManager.initialized then
        return
    end

    -- Get camera position once per frame
    local camX, camY, camZ = GetCameraPosition()
    local cameraPos = {x = camX, y = camY, z = camZ}

    -- Update all components
    for name, component in pairs(UIManager.components) do
        if component and component.Update then
            component:Update(dt, cameraPos)
        end
    end
end

-- ============================================================================
-- ANIMATION STATE QUERIES
-- ============================================================================

function UIManager.IsAPAnimating()
    if not UIManager.initialized then
        return false
    end

    -- Check if movement AP is animating
    if UIManager.components.movementAP and UIManager.components.movementAP.IsAnimating then
        return UIManager.components.movementAP:IsAnimating()
    end

    return false
end

-- ============================================================================
-- CLEANUP
-- ============================================================================

function UIManager.Destroy()
    if not UIManager.initialized then
        return
    end

    Log("[UIManager] Destroying UI System...")

    -- Destroy all components
    for name, component in pairs(UIManager.components) do
        if component and component.Destroy then
            component:Destroy()
        end
    end

    UIManager.components = {}
    UIManager.initialized = false

    Log("[UIManager] UI System destroyed")
end

-- ============================================================================
-- COMPONENT ACCESS
-- ============================================================================

function UIManager.GetComponent(name)
    return UIManager.components[name]
end

function UIManager.GetComponentCount()
    local count = 0
    for _ in pairs(UIManager.components) do
        count = count + 1
    end
    return count
end

function UIManager.EnableComponent(name)
    if UIManager.components[name] then
        UIManager.components[name]:SetEnabled(true)
    end
end

function UIManager.DisableComponent(name)
    if UIManager.components[name] then
        UIManager.components[name]:SetEnabled(false)
    end
end

-- ============================================================================
-- DEBUG
-- ============================================================================

function UIManager.PrintStatus()
    Log("========================================")
    Log("[UIManager] Status Report")
    Log("  Initialized: " .. tostring(UIManager.initialized))
    Log("  Components: " .. UIManager.GetComponentCount())
    for name, component in pairs(UIManager.components) do
        local status = component:IsEnabled() and "ENABLED" or "DISABLED"
        Log("    - " .. name .. ": " .. status)
    end
    Log("========================================")
end

return UIManager
