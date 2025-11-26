-- ============================================================================
-- LevelSelectLevel.lua (JSON Configuration Version)
-- Complete Level Select Level Script with JSON-driven configuration
-- ============================================================================
-- This version loads all UI configuration from a JSON file, making it
-- easy for designers to modify the UI without touching Lua code.
--
-- Author:  GE YONGQI
-- Email:   yongqi.ge@digipen.edu
-- Date:    2025-11-13
-- ============================================================================

-- ============================================================================
-- LEVEL STATE VARIABLES
-- ============================================================================

local buttonIDs = {}
local initialized = false
local config = nil
local editorToggleCooldown = 0  -- Cooldown for F1 editor toggle

-- ============================================================================
-- LEVEL LIFECYCLE: OnInit
-- ============================================================================

function OnInit()
    Log("LevelSelect Level Script Initialized (JSON Version)")
    Log("Loading configuration from JSON file...")
    
    -- Load configuration from JSON
    config = LoadJSON("assets/scripts/JSON/levelselect_config.json")
    
    if not config then
        Log("ERROR: Failed to load JSON configuration!")
        return
    end
    
    Log("Successfully loaded configuration for: " .. config.menu.name)
    
    -- Apply camera settings from JSON
    local cam = config.menu.camera
    SetCameraPosition(cam.position.x, cam.position.y, cam.position.z)
    SetCameraZoom(cam.zoom)
    
    -- Disable ImGui overlay
    DisableImGui()
    
    -- Set engine to editor mode (non-playing)
    SetEnginePlayState(true)
    
    -- Start menu background music from JSON config
    local music = config.menu.music
    PlaySound(music.name, music.loop, music.volume)
    Log("Playing level select music: " .. music.name)
    
    -- Clear any existing UI from previous states
    ClearAllButtons()
    
    -- Create UI buttons from JSON config
    CreateButtonsFromConfig()
    
    initialized = true
    Log("LevelSelect initialization complete")
    Log("Press F1 to toggle editor")
end

-- ============================================================================
-- UI CREATION FROM JSON
-- ============================================================================

function CreateButtonsFromConfig()
    if not config or not config.menu or not config.menu.buttons then
        Log("ERROR: Invalid configuration - no buttons found!")
        return
    end
    
    Log("Creating " .. #config.menu.buttons .. " buttons from config...")
    
    -- Iterate through buttons array in JSON
    for i, button in ipairs(config.menu.buttons) do
        Log("Creating button: " .. button.id)
        
        -- Create button using config data
        local buttonID = CreateButton(
            button.texture,
            button.position.x,
            button.position.y,
            button.scale.x,
            button.scale.y,
            button.callback  -- Callback function name from JSON
        )
        
        if buttonID > 0 then
            -- Store button ID with its config
            buttonIDs[button.id] = {
                id = buttonID,
                config = button
            }
            Log("  ✓ Button '" .. button.id .. "' created (ID: " .. buttonID .. ")")
        else
            Log("  ✗ Failed to create button: " .. button.id)
        end
    end
    
    Log("Button creation complete!")
end

-- ============================================================================
-- BUTTON CALLBACKS
-- ============================================================================

function OnLevel2ButtonClicked()
    Log("LEVEL EDITOR button clicked!")
    Log("Transitioning to Level Editor...")
    
    -- Play click sound effect (optional)
    -- PlaySound("button_click", false, 1.0)
    
    -- Transition to Level 2
    SetNextGameState("LEVEL_2")
end

function OnLevel3ButtonClicked()
    Log("DEMO button clicked!")
    Log("Transitioning to Demo...")
    
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

    -- Toggle ImGui editor with F1
    if IsKeyDown("F1") then
        if editorToggleCooldown <= 0 then
            local newState = ToggleEditor()
            if newState then
                Log("EDITOR ON")
            else
                Log("EDITOR OFF")
            end
            editorToggleCooldown = 0.3
        end
    end

    if editorToggleCooldown > 0 then
        editorToggleCooldown = editorToggleCooldown - dt
    end

    -- Optional: Add animated background effects here
    -- Example: Rotating background elements, particle systems, etc.

    -- Optional: Handle debug input
    if IsKeyDown("Escape") then
        Log("ESC pressed in level select")
        OnBackButtonClicked()
    end
    
    -- Optional: Quick level selection with number keys
    if IsKeyDown("2") then
        Log("Quick select: Level Editor")
        OnLevel2ButtonClicked()
    elseif IsKeyDown("3") then
        Log("Quick select: Demo")
        OnLevel3ButtonClicked()
    end
end

-- ============================================================================
-- LEVEL LIFECYCLE: OnDraw
-- ============================================================================

function OnDraw()
    if not config or not buttonIDs then
        return
    end
    
    -- Draw text on each button using config data
    for buttonKey, buttonData in pairs(buttonIDs) do
        local button = buttonData.config
        local text = button.text
        
        DrawButtonText(
            buttonData.id,
            text.font,
            text.content,
            text.offset.x,
            text.offset.y,
            text.scale,
            text.color.r,
            text.color.g,
            text.color.b
        )
    end
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
    
    -- Reset state
    buttonIDs = {}
    config = nil
    initialized = false
    
    Log("LevelSelect cleanup complete")
end

-- ============================================================================
-- DEBUG FUNCTIONS
-- ============================================================================

function PrintConfig()
    if not config then
        Log("No configuration loaded!")
        return
    end
    
    Log("═══════════════════════════════════════")
    Log("LevelSelect Configuration (from JSON):")
    Log("  Menu Name: " .. config.menu.name)
    Log("  Music: " .. config.menu.music.name)
    Log("  Camera Zoom: " .. config.menu.camera.zoom)
    Log("  Button Count: " .. #config.menu.buttons)
    
    for i, button in ipairs(config.menu.buttons) do
        Log("  Button " .. i .. ": " .. button.id .. " at (" .. 
            button.position.x .. ", " .. button.position.y .. ")")
    end
    
    Log("═══════════════════════════════════════")
end