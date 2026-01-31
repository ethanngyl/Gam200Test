--[[
===============================================================================
| File:          PauseMenu.lua
| Author:        Claude (AI Assistant)
| Date:          2025-01-27
| ------------------------------------------------------------------------------
|  Reusable Pause Menu System with Visual UI Elements
|
|  Features:
|  - Wood background with semi-transparent overlay
|  - Button images with text labels
|  - Horizontal layout: Quit | Settings | Resume
|  - Keyboard and visual feedback
|
|  Usage:
|    local PauseMenu = require("PauseMenu")
|    PauseMenu.Init()           -- In OnInit
|    PauseMenu.Update(dt)       -- In OnUpdate
|    PauseMenu.Draw()           -- In OnDraw
===============================================================================
--]]

local PauseMenu = {}

-- ============================================================================
-- CONFIGURATION
-- ============================================================================

local config = {
    -- Background
    background = {
        texture = "assets/Menu/WoodBackground.png",
        scale = { x = 2.0, y = 1.5 },
        layer = 50
    },
    
    -- Title (adjust these values)
    title = {
        text = "Paused",
        offsetX = -150,       -- X offset from center (negative = left)
        offsetY = -0.2,       -- Y offset from center (positive = up, in screen ratio)
        scale = 2.0,
        color = { r = 0.2, g = 0.15, b = 0.1 }
    },
    
    -- Buttons (adjust these values)
    buttons = {
        texture = "assets/Menu/Ui_btn.png",
        scale = { x = 0.35, y = 0.10 },
        layer = 51,
        textScale = 0.8,
        textColor = { r = 0.95, g = 0.85, b = 0.6 },
        selectedColor = { r = 1.0, g = 0.9, b = 0.3 },
        
        -- Each button has its own position (adjust individually)
        --  /imageY = world coordinates for button image (relative to camera)
        -- textX/textY = screen coordinates for text (relative to screen center)
        items = {
            { 
                id = "quit", 
                label = "MainMenu",
                imageX = -0.5,    -- World X offset from camera
                imageY = 0.0,     -- World Y offset from camera
                textX = -570,     -- Screen X offset from center (pixels)
                textY = -10       -- Screen Y offset from center (pixels)
            },
            { 
                id = "settings", 
                label = "Settings",
                imageX = 0.02,
                imageY = 0.0,
                textX = -50,
                textY = -10
            },
            { 
                id = "resume", 
                label = "Resume",
                imageX = 0.55,
                imageY = 0.0,
                textX = 480,
                textY = -10
            }
        }
    }
}

-- ============================================================================
-- STATE
-- ============================================================================

local state = {
    initialized = false,
    selectedIndex = 3,       -- Default to "Resume" (rightmost)
    
    -- Entity IDs
    backgroundID = 0,
    buttonIDs = {},
    
    -- Input tracking
    wasLeftPressed = false,
    wasRightPressed = false,
    wasEnterPressed = false,
    wasEscapePressed = false
}

-- ============================================================================
-- INITIALIZATION
-- ============================================================================

function PauseMenu.Init()
    state.selectedIndex = 3  -- Default to Resume
    state.initialized = true
    
    -- Reset input states
    state.wasLeftPressed = false
    state.wasRightPressed = false
    state.wasEnterPressed = false
    state.wasEscapePressed = false
    
    Log("[PauseMenu] Initialized")
end

-- ============================================================================
-- CREATE UI ELEMENTS (called when paused)
-- ============================================================================

local function CreatePauseUI()
    local camX, camY, camZ = GetCameraPosition()
    
    -- Create background
    state.backgroundID = SpawnSprite(
        config.background.texture,
        camX,
        camY,
        config.background.scale.x,
        config.background.scale.y,
        config.background.layer
    )
    
    -- Create button sprites using individual positions
    state.buttonIDs = {}
    local btnConfig = config.buttons
    
    for i, btn in ipairs(btnConfig.items) do
        -- Use individual button position (world coordinates, relative to camera)
        local btnX = camX + btn.imageX
        local btnY = camY + btn.imageY
        
        local btnID = SpawnSprite(
            btnConfig.texture,
            btnX, btnY,
            btnConfig.scale.x, btnConfig.scale.y,
            btnConfig.layer
        )
        
        state.buttonIDs[i] = {
            id = btnID,
            label = btn.label,
            textX = btn.textX,   -- Store text position
            textY = btn.textY,
            action = btn.id
        }
    end
end

-- ============================================================================
-- DESTROY UI ELEMENTS (called when resumed)
-- ============================================================================

local function DestroyPauseUI()
    -- Destroy background
    if state.backgroundID and state.backgroundID > 0 then
        DestroyEntity(state.backgroundID)
        state.backgroundID = 0
    end
    
    -- Destroy buttons
    for i, btn in ipairs(state.buttonIDs) do
        if btn.id and btn.id > 0 then
            DestroyEntity(btn.id)
        end
    end
    state.buttonIDs = {}
    
    Log("[PauseMenu] UI destroyed")
end

-- ============================================================================
-- UPDATE
-- ============================================================================

function PauseMenu.Update(dt)
    -- Toggle pause with Escape key
    local isEscapePressed = IsKeyDown("Escape")
    
    if isEscapePressed and not state.wasEscapePressed then
        TogglePause()
        
        if IsPaused() then
            Log("[PauseMenu] Game PAUSED")
            SetMasterVolume(0.3)  -- Lower volume instead of muting
            state.selectedIndex = 3  -- Reset to Resume
            CreatePauseUI()
        else
            Log("[PauseMenu] Game RESUMED")
            SetMasterVolume(1.0)
            DestroyPauseUI()
        end
    end
    state.wasEscapePressed = isEscapePressed
    
    -- If not paused, don't handle menu input
    if not IsPaused() then
        return
    end
    
    -- ========================================================================
    -- PAUSE MENU INPUT (horizontal navigation)
    -- ========================================================================
    
    local isLeftPressed = IsKeyDown("Left") or IsKeyDown("A")
    local isRightPressed = IsKeyDown("Right") or IsKeyDown("D")
    local isEnterPressed = IsKeyDown("Enter") or IsKeyDown("Space")
    
    -- Navigate left
    if isLeftPressed and not state.wasLeftPressed then
        state.selectedIndex = state.selectedIndex - 1
        if state.selectedIndex < 1 then
            state.selectedIndex = #config.buttons.items
        end
        PlaySound("button", false, 0.5)
    end
    
    -- Navigate right
    if isRightPressed and not state.wasRightPressed then
        state.selectedIndex = state.selectedIndex + 1
        if state.selectedIndex > #config.buttons.items then
            state.selectedIndex = 1
        end
        PlaySound("button", false, 0.5)
    end
    
    state.wasLeftPressed = isLeftPressed
    state.wasRightPressed = isRightPressed
    
    -- Quick select with number keys
    if IsKeyDown("1") then
        state.selectedIndex = 1
        PauseMenu.ExecuteAction()
        return
    elseif IsKeyDown("2") then
        state.selectedIndex = 2
        PauseMenu.ExecuteAction()
        return
    elseif IsKeyDown("3") then
        state.selectedIndex = 3
        PauseMenu.ExecuteAction()
        return
    end
    
    -- Enter/Space to select
    if isEnterPressed and not state.wasEnterPressed then
        PauseMenu.ExecuteAction()
    end
    state.wasEnterPressed = isEnterPressed
end

-- ============================================================================
-- EXECUTE SELECTED ACTION
-- ============================================================================

function PauseMenu.ExecuteAction()
    local action = config.buttons.items[state.selectedIndex].id
    
    PlaySound("button2", false, 0.7)
    
    if action == "resume" then
        PauseMenu.OnResume()
    elseif action == "settings" then
        PauseMenu.OnSettings()
    elseif action == "quit" then
        PauseMenu.OnQuit()
    end
end

-- ============================================================================
-- DRAW
-- ============================================================================

function PauseMenu.Draw()
    if not IsPaused() then
        return
    end
    
    local camX, camY, camZ = GetCameraPosition()
    local fbWidth, fbHeight = GetFramebufferSize()
    local centerX = fbWidth * 0.5
    local centerY = fbHeight * 0.5
    
    -- Screen scale factor (based on 1920x1080 reference resolution)
    local scaleFactorX = fbWidth / 1920
    local scaleFactorY = fbHeight / 1080
    local scaleFactor = math.min(scaleFactorX, scaleFactorY)  -- Use smaller to maintain aspect ratio
    
    -- ========================================================================
    -- DRAW TITLE
    -- ========================================================================
    local titleCfg = config.title
    local titleX = centerX + (titleCfg.offsetX * scaleFactorX)
    local titleY = centerY - (titleCfg.offsetY * fbHeight)
    local titleScale = titleCfg.scale * scaleFactor
    
    DrawText("Sans48", titleCfg.text, titleX, titleY, titleScale,
             titleCfg.color.r, titleCfg.color.g, titleCfg.color.b)
    
    -- ========================================================================
    -- DRAW BUTTON LABELS
    -- ========================================================================
    local btnConfig = config.buttons
    
    for i, btn in ipairs(state.buttonIDs) do
        local isSelected = (i == state.selectedIndex)
        
        -- Use individual text position (scaled by screen size)
        local textX = centerX + (btn.textX * scaleFactorX)
        local textY = centerY + (btn.textY * scaleFactorY)
        
        local scale = btnConfig.textScale * scaleFactor
        local color = isSelected and btnConfig.selectedColor or btnConfig.textColor
        
        local label = btn.label
        if isSelected then
            label = "> " .. label .. " <"
            textX = textX - (20 * scaleFactorX)
        end
        
        DrawText("Sans48", label, textX, textY, scale, color.r, color.g, color.b)
    end
end

-- ============================================================================
-- CALLBACKS
-- ============================================================================

function PauseMenu.OnResume()
    TogglePause()
    SetMasterVolume(1.0)
    DestroyPauseUI()
    Log("[PauseMenu] Resumed")
end

function PauseMenu.OnSettings()
    -- TODO: Open settings menu
    Log("[PauseMenu] Settings - Not implemented yet")
end

function PauseMenu.OnQuit()
    DestroyPauseUI()
    TogglePause()
    SetMasterVolume(1.0)
    SetNextGameState("mainMenu")
    Log("[PauseMenu] Returning to main menu")
end

-- ============================================================================
-- CLEANUP
-- ============================================================================

function PauseMenu.Destroy()
    DestroyPauseUI()
    state.initialized = false
    Log("[PauseMenu] Destroyed")
end

-- ============================================================================
-- RETURN MODULE
-- ============================================================================

return PauseMenu
