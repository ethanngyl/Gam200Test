-- ============================================================================
-- LevelSelectLevel.lua
-- Complete Level Select Level Script
-- ============================================================================
-- This script replaces the hardcoded LevelSelect C++ logic with a Lua-driven
-- approach, following the same pattern as MainMenuLevel.lua
--
-- Author: GE YONGQI
-- Date: 2025-11-13
-- ============================================================================

-- ============================================================================
-- LEVEL STATE VARIABLES
-- ============================================================================

local level1ButtonID = 0
local level2ButtonID = 0
local level3ButtonID = 0
local backButtonID = 0
local initialized = false

-- Audio settings
local menuMusicName = "mmbgm"
local menuMusicVolume = 1.0

-- UI Configuration (can be loaded from JSON in the future)
local uiConfig = {
    level1Button = {
        texture = "assets/Ui_btn.png",
        posX = 0.0,
        posY = 0.6,
        scaleX = 1.0,
        scaleY = 0.4,
        textContent = "Level 1",
        textOffsetX = -90.0,
        textOffsetY = -24.0
    },
    
    level2Button = {
        texture = "assets/Ui_btn.png",
        posX = 0.0,
        posY = 0.3,
        scaleX = 1.0,
        scaleY = 0.4,
        textContent = "Level 2",
        textOffsetX = -90.0,
        textOffsetY = -24.0
    },
    
    level3Button = {
        texture = "assets/Ui_btn.png",
        posX = 0.0,
        posY = 0.0,
        scaleX = 1.0,
        scaleY = 0.4,
        textContent = "Level 3",
        textOffsetX = -90.0,
        textOffsetY = -24.0
    },
    
    backButton = {
        texture = "assets/Ui_btn.png",
        posX = 0.0,
        posY = -0.3,
        scaleX = 1.0,
        scaleY = 0.4,
        textContent = "Back",
        textOffsetX = -60.0,
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
    Log("LevelSelect Level Script Initialized")
    Log("This is a fully Lua-scripted level select state!")
    
    -- Set camera position for level select
    SetCameraPosition(0.0, 0.0, 0.0)
    SetCameraZoom(1.0)
    
    -- Disable ImGui overlay
    DisableImGui()
    
    -- Set engine to editor mode (non-playing)
    SetEnginePlayState(false)
    
    -- Start menu background music (same as main menu)
    PlaySound(menuMusicName, true, menuMusicVolume)
    Log("Playing level select music: " .. menuMusicName)
    
    -- Clear any existing UI from previous states
    ClearAllButtons()
    
    -- Create UI buttons
    CreateLevelSelectButtons()
    
    initialized = true
    Log("LevelSelect initialization complete")
end

-- ============================================================================
-- UI CREATION FUNCTIONS
-- ============================================================================

function CreateLevelSelectButtons()
    Log("Creating level select buttons...")
    
    -- Create Level 1 button
    level1ButtonID = CreateButton(
        uiConfig.level1Button.texture,
        uiConfig.level1Button.posX,
        uiConfig.level1Button.posY,
        uiConfig.level1Button.scaleX,
        uiConfig.level1Button.scaleY,
        "OnLevel1ButtonClicked"
    )
    
    if level1ButtonID > 0 then
        Log("Level 1 button created (ID: " .. level1ButtonID .. ")")
    else
        Log("Failed to create Level 1 button!")
    end
    
    -- Create Level 2 button
    level2ButtonID = CreateButton(
        uiConfig.level2Button.texture,
        uiConfig.level2Button.posX,
        uiConfig.level2Button.posY,
        uiConfig.level2Button.scaleX,
        uiConfig.level2Button.scaleY,
        "OnLevel2ButtonClicked"
    )
    
    if level2ButtonID > 0 then
        Log("Level 2 button created (ID: " .. level2ButtonID .. ")")
    else
        Log("Failed to create Level 2 button!")
    end
    
    -- Create Level 3 button
    level3ButtonID = CreateButton(
        uiConfig.level3Button.texture,
        uiConfig.level3Button.posX,
        uiConfig.level3Button.posY,
        uiConfig.level3Button.scaleX,
        uiConfig.level3Button.scaleY,
        "OnLevel3ButtonClicked"
    )
    
    if level3ButtonID > 0 then
        Log("Level 3 button created (ID: " .. level3ButtonID .. ")")
    else
        Log("Failed to create Level 3 button!")
    end
    
    -- Create Back button
    backButtonID = CreateButton(
        uiConfig.backButton.texture,
        uiConfig.backButton.posX,
        uiConfig.backButton.posY,
        uiConfig.backButton.scaleX,
        uiConfig.backButton.scaleY,
        "OnBackButtonClicked"
    )
    
    if backButtonID > 0 then
        Log("Back button created (ID: " .. backButtonID .. ")")
    else
        Log("Failed to create Back button!")
    end
end

-- ============================================================================
-- BUTTON CALLBACKS
-- ============================================================================

function OnLevel1ButtonClicked()
    Log("LEVEL 1 button clicked!")
    Log("Transitioning to Level 1...")
    
    -- Play click sound effect (optional)
    -- PlaySound("button_click", false, 1.0)
    
    -- Transition to Level 1
    SetNextGameState("LEVEL_1")
end

function OnLevel2ButtonClicked()
    Log("LEVEL 2 button clicked!")
    Log("Transitioning to Level 2...")
    
    -- Play click sound effect (optional)
    -- PlaySound("button_click", false, 1.0)
    
    -- Transition to Level 2
    SetNextGameState("LEVEL_2")
end

function OnLevel3ButtonClicked()
    Log("LEVEL 3 button clicked!")
    Log("Transitioning to Level 3...")
    
    -- Play click sound effect (optional)
    -- PlaySound("button_click", false, 1.0)
    
    -- Transition to Level 3
    SetNextGameState("LEVEL_3")
end

function OnBackButtonClicked()
    Log("BACK button clicked!")
    Log("Returning to Main Menu...")
    
    -- Play click sound effect (optional)
    -- PlaySound("button_click", false, 1.0)
    
    -- Return to main menu
    SetNextGameState("mainMenu")
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
        Log("ESC pressed in level select")
        OnBackButtonClicked()
    end
    
    -- Optional: Quick level selection with number keys
    if IsKeyDown("1") then
        Log("Quick select: Level 1")
        OnLevel1ButtonClicked()
    elseif IsKeyDown("2") then
        Log("Quick select: Level 2")
        OnLevel2ButtonClicked()
    elseif IsKeyDown("3") then
        Log("Quick select: Level 3")
        OnLevel3ButtonClicked()
    end
end

-- ============================================================================
-- LEVEL LIFECYCLE: OnDraw
-- ============================================================================

function OnDraw()
    -- Draw UI text on buttons
    
    -- Level 1 button text
    DrawButtonText(
        level1ButtonID,
        uiConfig.text.font,
        uiConfig.level1Button.textContent,
        uiConfig.level1Button.textOffsetX,
        uiConfig.level1Button.textOffsetY,
        uiConfig.text.scale,
        uiConfig.text.colorR,
        uiConfig.text.colorG,
        uiConfig.text.colorB
    )
    
    -- Level 2 button text
    DrawButtonText(
        level2ButtonID,
        uiConfig.text.font,
        uiConfig.level2Button.textContent,
        uiConfig.level2Button.textOffsetX,
        uiConfig.level2Button.textOffsetY,
        uiConfig.text.scale,
        uiConfig.text.colorR,
        uiConfig.text.colorG,
        uiConfig.text.colorB
    )
    
    -- Level 3 button text
    DrawButtonText(
        level3ButtonID,
        uiConfig.text.font,
        uiConfig.level3Button.textContent,
        uiConfig.level3Button.textOffsetX,
        uiConfig.level3Button.textOffsetY,
        uiConfig.text.scale,
        uiConfig.text.colorR,
        uiConfig.text.colorG,
        uiConfig.text.colorB
    )
    
    -- Back button text
    DrawButtonText(
        backButtonID,
        uiConfig.text.font,
        uiConfig.backButton.textContent,
        uiConfig.backButton.textOffsetX,
        uiConfig.backButton.textOffsetY,
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
    Log("LevelSelect cleanup...")
    
    -- Stop all sounds
    StopAllSounds()
    
    -- Clear UI buttons
    ClearAllButtons()
    
    -- Reset button IDs
    level1ButtonID = 0
    level2ButtonID = 0
    level3ButtonID = 0
    backButtonID = 0
    initialized = false
    
    Log("LevelSelect cleanup complete")
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
    Log("LevelSelect Configuration:")
    Log("  Music: " .. menuMusicName)
    Log("  Level 1 Button: (" .. uiConfig.level1Button.posX .. ", " .. uiConfig.level1Button.posY .. ")")
    Log("  Level 2 Button: (" .. uiConfig.level2Button.posX .. ", " .. uiConfig.level2Button.posY .. ")")
    Log("  Level 3 Button: (" .. uiConfig.level3Button.posX .. ", " .. uiConfig.level3Button.posY .. ")")
    Log("  Back Button: (" .. uiConfig.backButton.posX .. ", " .. uiConfig.backButton.posY .. ")")
    Log("═══════════════════════════════════════")
end