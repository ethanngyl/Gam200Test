--[[
===============================================================================
| File:          PauseMenu.lua
| Author:        GE YONGQI
| Date:          2026-01-27
| ------------------------------------------------------------------------------
|  Reusable Pause Menu System with Visual UI Elements
|
|  Features:
|  - Wood background with semi-transparent overlay
|  - Button images with hover highlight (like MainMenu)
|  - Horizontal layout: Quit | Settings | Resume
|  - Settings sub-menu with volume control
|
|  Usage:
|    local PauseMenu = require("PauseMenu")
|    PauseMenu.Init()           -- In OnInit
|    PauseMenu.Update(dt)       -- In OnUpdate
|    PauseMenu.Draw()           -- In OnDraw

 Copyright (C) 2026 DigiPen Institute of Technology.
 Reproduction or disclosure of this file or its contents
 without the prior written consent of DigiPen Institute of
 Technology is prohibited.
===============================================================================
--]]

local PauseMenu = {}
local SettingsMenu = require("SettingsMenu")

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
    
    -- Scroll overlay (on top of background, behind buttons)
    overlay = {
        texture = "assets/Menu/Scroll Overlay.png",
        scale = { x = 2.25, y = 1.25 },
        layer = 50
    },

    -- Title
    title = {
        text = "Paused",
        scale = 2.0,
        color = { r = 0.2, g = 0.15, b = 0.1 }
    },
    
    -- Buttons (using CreateButton for hover effect)
    buttons = {
        texture = "assets/Menu/Ui_btn.png",
        scale = { x = 0.35, y = 0.10 },
        layer = 51,
        
        -- 2x2 grid layout:
        --   [Resume]   [Settings]
        --   [Restart]  [MainMenu]
        items = {
            { 
                id = "resume", 
                label = "Resume",
                offsetX = -0.24,    -- World X offset from camera
                offsetY = 0.0,     -- World Y offset from camera (top row)
                callback = "OnPauseResumeClicked",
                text = {
                    offsetX = -60,
                    offsetY = -10,
                    scale = 0.8,
                    color = { r = 255, g = 255, b = 255 }
                }
            },
            { 
                id = "settings", 
                label = "Settings",
                offsetX = 0.24,
                offsetY = 0.0,
                callback = "OnPauseSettingsClicked",
                text = {
                    offsetX = -60,
                    offsetY = -10,
                    scale = 0.8,
                    color = { r = 255, g = 255, b = 255 }
                }
            },
            { 
                id = "restart", 
                label = "Restart",
                offsetX = -0.24,
                offsetY = -0.15,    -- Bottom row
                callback = "OnPauseRestartClicked",
                text = {
                    offsetX = -60,
                    offsetY = -15,
                    scale = 0.8,
                    color = { r = 255, g = 255, b = 255 }
                }
            },
            { 
                id = "quit", 
                label = "MainMenu",
                offsetX = 0.24,
                offsetY = -0.15,
                callback = "OnPauseQuitClicked",
                text = {
                    offsetX = -80,
                    offsetY = -15,
                    scale = 0.8,
                    color = { r = 255, g = 255, b = 255 }
                }
            }
        }
    }
}

-- ============================================================================
-- STATE
-- ============================================================================

local state = {
    initialized = false,
    
    -- Entity IDs
    backgroundID = 0,
    overlayID = 0,
    buttonIDs = {},  -- Array of {id = buttonID, config = buttonConfig}
    
    -- Input tracking
    wasEscapePressed = false
}

-- ============================================================================
-- GLOBAL BUTTON CALLBACKS (called by CreateButton system)
-- ============================================================================

function OnPauseQuitClicked()
    PlaySound("button2", false, 0.7)
    PauseMenu.OnQuit()
end

function OnPauseSettingsClicked()
    PlaySound("button2", false, 0.7)
    PauseMenu.OnSettings()
end

function OnPauseResumeClicked()
    PlaySound("button2", false, 0.7)
    PauseMenu.OnResume()
end

function OnPauseRestartClicked()
    PlaySound("button2", false, 0.7)
    PauseMenu.OnRestart()
end

-- ============================================================================
-- INITIALIZATION
-- ============================================================================

function PauseMenu.Init()
    state.initialized = true
    
    -- Reset input states
    state.wasEscapePressed = false
    
    -- Initialize settings menu
    SettingsMenu.Init()
    
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
    
    -- Create scroll overlay on top of background
    state.overlayID = SpawnSprite(
        config.overlay.texture,
        camX,
        camY,
        config.overlay.scale.x,
        config.overlay.scale.y,
        config.overlay.layer
    )
    
    -- Create buttons using CreateButton (with hover highlight)
    state.buttonIDs = {}
    local btnConfig = config.buttons
    
    for i, btn in ipairs(btnConfig.items) do
        local btnX = camX + btn.offsetX
        local btnY = camY + btn.offsetY
        
        local buttonID = CreateButton(
            btnConfig.texture,
            btnX, btnY,
            btnConfig.scale.x, btnConfig.scale.y,
            btn.callback,
            btnConfig.layer
        )
        
        state.buttonIDs[i] = {
            id = buttonID,
            config = btn
        }
        
        Log("[PauseMenu] Created button: " .. btn.id .. " (ID: " .. buttonID .. ")")
    end
    
    Log("[PauseMenu] UI created")
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
    
    -- Destroy scroll overlay
    if state.overlayID and state.overlayID > 0 then
        DestroyEntity(state.overlayID)
        state.overlayID = 0
    end
    
    -- Clear all buttons (created with CreateButton)
    ClearAllButtons()
    state.buttonIDs = {}
    
    Log("[PauseMenu] UI destroyed")
end

-- ============================================================================
-- UPDATE
-- ============================================================================

function PauseMenu.Update(dt)
    -- Don't process pause input when SkillSwapUI is active
    if _G.SkillSwapUI and _G.SkillSwapUI.IsActive and _G.SkillSwapUI.IsActive() then
        return
    end

    -- If settings menu is active, let it handle input
    if SettingsMenu.IsActive() then
        SettingsMenu.Update(dt)
        return
    end
    
    -- Toggle pause with Escape key
    local isEscapePressed = IsKeyDown("Escape")
    
    if isEscapePressed and not state.wasEscapePressed then
        TogglePause()
        
        if IsPaused() then
            Log("[PauseMenu] Game PAUSED")
            -- Lower volume to 30% of the saved volume setting
            local savedVolume = SettingsMenu.GetSavedVolume()
            SetMasterVolume(savedVolume * 0.3)
            CreatePauseUI()
        else
            Log("[PauseMenu] Game RESUMED")
            -- Restore to saved volume setting
            SetMasterVolume(SettingsMenu.GetSavedVolume())
            DestroyPauseUI()
        end
    end
    state.wasEscapePressed = isEscapePressed
end

-- ============================================================================
-- DRAW
-- ============================================================================

function PauseMenu.Draw()
    if not IsPaused() then
        return
    end

    -- Don't draw pause menu when SkillSwapUI is showing
    if _G.SkillSwapUI and _G.SkillSwapUI.IsActive and _G.SkillSwapUI.IsActive() then
        return
    end
    
    -- If settings menu is active, draw it instead
    if SettingsMenu.IsActive() then
        SettingsMenu.Draw()
        return
    end
    
    local fbWidth, fbHeight = GetFramebufferSize()
    local centerX = fbWidth * 0.5
    local centerY = fbHeight * 0.5
    
    -- Screen scale factor (based on 1920x1080 reference resolution)
    local scaleFactorX = fbWidth / 1920
    local scaleFactorY = fbHeight / 1080
    local scaleFactor = math.min(scaleFactorX, scaleFactorY)
    
    -- ========================================================================
    -- DRAW TITLE
    -- ========================================================================
    local titleCfg = config.title
    local titleText = titleCfg.text
    local titleScale = titleCfg.scale * scaleFactor
    
    -- Center the title
    local approxTitleWidth = #titleText * 30 * titleScale
    local titleX = centerX - (approxTitleWidth * 0.5)
    local titleY = centerY + 150 * scaleFactorY  -- Above the buttons
    
    DrawText("Playfair48", titleText, titleX, titleY, titleScale,
             titleCfg.color.r, titleCfg.color.g, titleCfg.color.b)
    
    -- ========================================================================
    -- DRAW BUTTON TEXT (using DrawButtonText for proper positioning)
    -- ========================================================================
    for i, btnData in ipairs(state.buttonIDs) do
        if btnData.id and btnData.id > 0 then
            local textCfg = btnData.config.text
            DrawButtonText(
                btnData.id,
                "Playfair48",
                btnData.config.label,
                textCfg.offsetX,
                textCfg.offsetY,
                textCfg.scale,
                textCfg.color.r,
                textCfg.color.g,
                textCfg.color.b
            )
        end
    end
end

-- ============================================================================
-- CALLBACKS
-- ============================================================================

function PauseMenu.OnResume()
    TogglePause()
    -- Restore to saved volume setting
    SetMasterVolume(SettingsMenu.GetSavedVolume())
    DestroyPauseUI()
    Log("[PauseMenu] Resumed")
end

function PauseMenu.OnSettings()
    -- Hide pause menu UI elements
    DestroyPauseUI()
    
    -- Open settings menu with callback to restore pause menu when closed
    SettingsMenu.Open(function()
        -- When settings closes, recreate pause menu UI
        CreatePauseUI()
    end)
    
    Log("[PauseMenu] Opening Settings")
end

function PauseMenu.OnRestart()
    -- Reset campaign progress back to level 1
    local f = io.open("assets/JSON/LevelProgress.json", "w")
    if f then
        f:write("{\n  \"currentLevel\": 1,\n  \"totalLevels\": 3\n}\n")
        f:close()
        Log("[PauseMenu] Level progress reset to 1")
    end

    DestroyPauseUI()
    TogglePause()
    -- Restore to saved volume setting
    SetMasterVolume(SettingsMenu.GetSavedVolume())
    SetNextGameState("LEVEL_3")
    Log("[PauseMenu] Restarting campaign from Level 1")
end

function PauseMenu.OnQuit()
    DestroyPauseUI()
    TogglePause()
    -- Restore to saved volume setting
    SetMasterVolume(SettingsMenu.GetSavedVolume())
    SetNextGameState("mainMenu")
    Log("[PauseMenu] Returning to main menu")
end

-- ============================================================================
-- CLEANUP
-- ============================================================================

function PauseMenu.Destroy()
    SettingsMenu.Destroy()
    DestroyPauseUI()
    state.initialized = false
    Log("[PauseMenu] Destroyed")
end

-- ============================================================================
-- RETURN MODULE
-- ============================================================================

return PauseMenu
