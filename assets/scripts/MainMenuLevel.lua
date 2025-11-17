-- ============================================================================
-- MainMenuLevel.lua
-- Complete Main Menu Level Script
-- ============================================================================
-- This script replaces the hardcoded MainMenu C++ logic with a Lua-driven
-- approach. It demonstrates how entire game states can be scripted.
--
-- Author: GE YONGQI
-- Date: 2025-11-13
-- ============================================================================

-- ============================================================================
-- LEVEL STATE VARIABLES
-- ============================================================================

local playButtonID = 0
local exitButtonID = 0
local initialized = false

-- Audio settings
local menuMusicName = "mmbgm"
local menuMusicVolume = 1.0

-- UI Configuration (can be loaded from JSON in the future)
local uiConfig = {
    playButton = {
        texture = "assets/Ui_btn.png",
        posX = 0.0,
        posY = 0.3,
        scaleX = 1.0,
        scaleY = 0.5,
        textContent = "Play",
        textOffsetX = -60.0,
        textOffsetY = -24.0
    },
    
    exitButton = {
        texture = "assets/Ui_btn.png",
        posX = 0.0,
        posY = -0.3,
        scaleX = 1.0,
        scaleY = 0.5,
        textContent = "Exit",
        textOffsetX = -50.0,
        textOffsetY = -24.0
    },
    
    text = {
        font = "Sans48",
        scale = 1.0,
        colorR = 1.0,
        colorG = 1.0,
        colorB = 1.0
    }
}

-- ============================================================================
-- LEVEL LIFECYCLE: OnInit
-- ============================================================================

function OnInit()
    Log("MainMenu Level Script Initialized")
    Log("This is a fully Lua-scripted game state!")
    
    -- Load configuration from file (future enhancement)
    -- LoadConfigFromJSON("assets/config/mainmenu.json")
    
    -- Set camera position for menu
    SetCameraPosition(0.0, 0.0, 0.0)
    SetCameraZoom(1.0)
    
    -- Disable ImGui overlay
    DisableImGui()
    
    -- Set engine to editor mode (non-playing)
    SetEnginePlayState(false)
    
    -- Start menu background music
    PlaySound(menuMusicName, true, menuMusicVolume)
    Log("Playing menu music: " .. menuMusicName)
    
    -- Clear any existing UI from previous states
    ClearAllButtons()
    
    -- Create UI buttons
    CreateMenuButtons()
    
    initialized = true
    Log("MainMenu initialization complete")
end

-- ============================================================================
-- UI CREATION FUNCTIONS
-- ============================================================================

function CreateMenuButtons()
    Log("Creating menu buttons...")
    
    -- Create Play button
    playButtonID = CreateButton(
        uiConfig.playButton.texture,
        uiConfig.playButton.posX,
        uiConfig.playButton.posY,
        uiConfig.playButton.scaleX,
        uiConfig.playButton.scaleY,
        "OnPlayButtonClicked"  -- Callback function name
    )
    
    if playButtonID > 0 then
        Log("Play button created (ID: " .. playButtonID .. ")")
    else
        Log("Failed to create Play button!")
    end
    
    -- Create Exit button
    exitButtonID = CreateButton(
        uiConfig.exitButton.texture,
        uiConfig.exitButton.posX,
        uiConfig.exitButton.posY,
        uiConfig.exitButton.scaleX,
        uiConfig.exitButton.scaleY,
        "OnExitButtonClicked"  -- Callback function name
    )
    
    if exitButtonID > 0 then
        Log("Exit button created (ID: " .. exitButtonID .. ")")
    else
        Log("Failed to create Exit button!")
    end
end

-- ============================================================================
-- BUTTON CALLBACKS
-- ============================================================================

function OnPlayButtonClicked()
    Log("PLAY button clicked!")
    Log("Transitioning to Level Select...")
    
    -- Play click sound effect (optional)
    -- PlaySound("button_click", false, 1.0)
    
    -- Transition to Level Select state
    SetNextGameState("Level_select")
end

function OnExitButtonClicked()
    Log("EXIT button clicked!")
    Log("Quitting game...")
    
    -- Play click sound effect (optional)
    -- PlaySound("button_click", false, 1.0)
    
    -- Quit the game
    SetNextGameState("GS_QUIT")
end

-- ============================================================================
-- LEVEL LIFECYCLE: OnUpdate
-- ============================================================================

function OnUpdate(dt)
    -- Update audio system
    UpdateAudio(dt)
    
    -- Optional: Add animated background effects here
    -- Example: Rotating background elements, particle systems, etc.
    
    -- Optional: Handle debug input
    if IsKeyDown("Escape") then
        Log("ESC pressed in menu")
        OnExitButtonClicked()
    end
end

-- ============================================================================
-- LEVEL LIFECYCLE: OnDraw
-- ============================================================================

function OnDraw()
    -- Draw UI text on buttons
    -- Note: This uses world-to-screen coordinate conversion
    
    -- Play button text
    DrawButtonText(
        playButtonID,
        uiConfig.text.font,
        uiConfig.playButton.textContent,
        uiConfig.playButton.textOffsetX,
        uiConfig.playButton.textOffsetY,
        uiConfig.text.scale,
        uiConfig.text.colorR,
        uiConfig.text.colorG,
        uiConfig.text.colorB
    )
    
    -- Exit button text
    DrawButtonText(
        exitButtonID,
        uiConfig.text.font,
        uiConfig.exitButton.textContent,
        uiConfig.exitButton.textOffsetX,
        uiConfig.exitButton.textOffsetY,
        uiConfig.text.scale,
        uiConfig.text.colorR,
        uiConfig.text.colorG,
        uiConfig.text.colorB
    )
end

-- ============================================================================
-- LEVEL LIFECYCLE: OnDestroy
-- ============================================================================

function OnDestroy()
    Log("MainMenu cleanup...")
    
    -- Stop all sounds
    StopAllSounds()
    
    -- Clear UI buttons
    ClearAllButtons()
    
    -- Reset button IDs
    playButtonID = 0
    exitButtonID = 0
    initialized = false
    
    Log("MainMenu cleanup complete")
end

-- ============================================================================
-- UTILITY FUNCTIONS
-- ============================================================================

function LoadConfigFromJSON(filepath)
    -- Future enhancement: Load UI configuration from JSON
    -- This would allow level designers to modify UI without touching code
    Log("Loading config: " .. filepath)
    -- Implementation would use a JSON parser exposed from C++
end

-- ============================================================================
-- DEBUG FUNCTIONS
-- ============================================================================

function PrintConfig()
    Log("═══════════════════════════════════════")
    Log("MainMenu Configuration:")
    Log("  Music: " .. menuMusicName)
    Log("  Play Button: (" .. uiConfig.playButton.posX .. ", " .. uiConfig.playButton.posY .. ")")
    Log("  Exit Button: (" .. uiConfig.exitButton.posX .. ", " .. uiConfig.exitButton.posY .. ")")
    Log("═══════════════════════════════════════")
end