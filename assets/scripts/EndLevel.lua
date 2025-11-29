-- ============================================================================
-- MainMenuLevel.lua (JSON Configuration Version)
-- Complete Main Menu Level Script with JSON-driven configuration
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
local editorToggleCooldown = 0
local backgroundSpriteID = 0  -- Store background sprite entity ID
local logoSpriteID = 0        -- Store logo sprite entity ID
local cornerSpriteIDs = {}    -- Store corner sprite entity IDs  
-- ============================================================================
-- LEVEL LIFECYCLE: OnInit
-- ============================================================================

function OnInit()
    Log("MainMenu Level Script Initialized (JSON Version)")
    Log("Loading configuration from JSON file...")
    
    -- Load configuration from JSON
    config = LoadJSON("assets/JSON/EndLevelConfig.json")
    
    if not config then
        Log("ERROR: Failed to load JSON configuration!")
        return
    end
    
    Log("Successfully loaded configuration for: " .. config.menu.name)
    
    -- Apply camera settings from JSON
    local cam = config.menu.camera
    SetCameraPosition(cam.position.x, cam.position.y, cam.position.z)
    SetCameraZoom(cam.zoom)

    -- Disable ImGui overlay by default for menu
    DisableImGui()

    -- Set engine to editor mode (non-playing)
    SetEnginePlayState(true)

    -- ========================================================================
    -- CREATE BACKGROUND SPRITE
    -- ========================================================================
    -- Load background configuration (use default if not in JSON)
    local background = config.menu.background or {
        texture = "assets/Menu/WoodBackground.png",
        position = { x = 0.0, y = 0.0 },
        scale = { x = 2.0, y = 2.0 },
        layer = -10  -- Behind everything
    }

    Log("Creating background sprite: " .. background.texture)
    backgroundSpriteID = SpawnSprite(
        background.texture,
        background.position.x,
        background.position.y,
        background.scale.x,
        background.scale.y,
        background.layer
    )

    if backgroundSpriteID > 0 then
        Log("✓ Background sprite created (ID: " .. backgroundSpriteID .. ")")
    else
        Log("✗ WARNING: Failed to create background sprite")
    end
    -- ========================================================================

    -- ========================================================================
    -- CREATE CORNER SPRITES
    -- ========================================================================
    if config.menu.cornerSprites then
        Log("Creating corner decorations...")

        for i, corner in ipairs(config.menu.cornerSprites) do
            local spriteID = SpawnSprite(
                corner.texture,
                corner.offset.x,
                corner.offset.y,
                corner.scale.x,
                corner.scale.y,
                corner.layer,
                corner.rotation  -- Rotation in degrees
            )

            if spriteID > 0 then
                cornerSpriteIDs[corner.id] = spriteID
                Log("  ✓ Corner sprite '" .. corner.id .. "' created (ID: " .. spriteID .. ", rotation: " .. corner.rotation .. "°)")
            else
                Log("  ✗ FAILED to create corner sprite: " .. corner.id)
            end
        end

        Log("Corner decorations complete!")
    end
    -- ========================================================================

    -- Start menu background music from JSON config
    local music = config.menu.music
    PlaySound(music.name, music.loop, music.volume)
    Log("Playing menu music: " .. music.name)

    -- Clear any existing UI from previous states
    ClearAllButtons()

    -- Create UI buttons from JSON config
    CreateButtonsFromConfig()
    
    initialized = true
    Log("MainMenu initialization complete")
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

        -- Create button using config data (layer is optional, defaults to 10)
        local layer = button.layer or 10
        local buttonID = CreateButton(
            button.texture,
            button.position.x,
            button.position.y,
            button.scale.x,
            button.scale.y,
            button.callback,  -- Callback function name from JSON
            layer             -- Layer for rendering order
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
    UpdateAudio(dt)
    
    -- DEBUG: Check if F1 is being detected
    if IsKeyDown("F1") then
        Log("F1 KEY DETECTED!")  -- Add this line
        
        if editorToggleCooldown <= 0 then
            Log("Attempting to toggle editor...")  -- Add this
            local newState = ToggleEditor()
            Log("ToggleEditor returned: " .. tostring(newState))  -- Add this
            
            if newState then
                Log("EDITOR ON")
            else
                Log("EDITOR OFF")
            end
            editorToggleCooldown = 0.3
        else
            Log("Cooldown active: " .. editorToggleCooldown)  -- Add this
        end
    end
    
    if editorToggleCooldown > 0 then
        editorToggleCooldown = editorToggleCooldown - dt
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
    Log("End Level Cleanup...")

    -- Stop all sounds
    StopAllSounds()

    -- Clear UI buttons
    ClearAllButtons()

    -- Destroy background sprite
    if backgroundSpriteID > 0 then
        DestroyEntity(backgroundSpriteID)
        Log("Background sprite destroyed")
    end

    -- Destroy logo sprite
    if logoSpriteID > 0 then
        DestroyEntity(logoSpriteID)
        Log("Logo sprite destroyed")
    end

    -- Destroy corner sprites
    for cornerID, spriteID in pairs(cornerSpriteIDs) do
        if spriteID > 0 then
            DestroyEntity(spriteID)
            Log("Corner sprite '" .. cornerID .. "' destroyed")
        end
    end

    -- Reset state
    buttonIDs = {}
    config = nil
    initialized = false
    backgroundSpriteID = 0
    logoSpriteID = 0
    cornerSpriteIDs = {}

    Log("MainMenu cleanup complete")
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
    Log("MainMenu Configuration (from JSON):")
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