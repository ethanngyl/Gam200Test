-- ============================================================================
-- LevelSelectLevel.lua
-- Author:        Padilla carl jameson
-- Email:         c.Padilla@digipen.edu
-- Date:          2025-11-13
-- Contribution:  100%
--
-- REFACTORED:    2025-12-21 (Using ButtonManager module)
-- ----------------------------------------------------------------------------
-- Description:
-- Level selection menu with JSON-driven UI configuration. Now uses centralized
-- ButtonManager module to eliminate code duplication and simplify maintenance.
-- ============================================================================

-- Load ButtonManager module
local ButtonManager = require("assets/scripts/ButtonManager")

-- ============================================================================
-- LEVEL STATE VARIABLES
-- ============================================================================

local initialized = false
local config = nil
local backgroundSpriteID = 0    -- Store background sprite entity ID
local logoSpriteID = 0          -- Store logo sprite entity ID
local cornerSpriteIDs = {}      -- Store corner sprite entity IDs

-- ============================================================================
-- LEVEL LIFECYCLE: OnInit
-- ============================================================================

function OnInit()
    Log("LevelSelect Level Script Initialized (ButtonManager Version)")
    Log("Loading configuration from JSON file...")

    -- Load configuration from JSON
    config = LoadJSON("assets/JSON/levelselect_config.json")

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

    -- ========================================================================
    -- CREATE BACKGROUND SPRITE
    -- ========================================================================
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
        Log("Background sprite created (ID: " .. backgroundSpriteID .. ")")
    else
        Log("WARNING: Failed to create background sprite")
    end

    -- ========================================================================
    -- CREATE LOGO SPRITE (optional)
    -- ========================================================================
    if config.menu.logo then
        local logo = config.menu.logo
        Log("Creating logo sprite: " .. logo.texture)

        logoSpriteID = SpawnSprite(
            logo.texture,
            logo.position.x,
            logo.position.y,
            logo.scale.x,
            logo.scale.y,
            logo.layer
        )

        if logoSpriteID > 0 then
            Log("Logo sprite created (ID: " .. logoSpriteID .. ")")
        else
            Log("WARNING: Failed to create logo sprite")
        end
    else
        Log("No logo configuration found in JSON")
    end

    -- ========================================================================
    -- CREATE CORNER SPRITES (optional decorations)
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
                Log("  Corner sprite '" .. corner.id .. "' created (ID: " .. spriteID .. ", rotation: " .. corner.rotation .. " degrees)")
            else
                Log("  FAILED to create corner sprite: " .. corner.id)
            end
        end

        Log("Corner decorations complete!")
    end

    -- Start menu background music from JSON config
    local music = config.menu.music
    PlaySound(music.name, music.loop, music.volume)
    Log("Playing level select music: " .. music.name)

    -- ========================================================================
    -- INITIALIZE BUTTON MANAGER
    -- ========================================================================
    ButtonManager.Initialize(config.menu.buttons)

    initialized = true
    Log("LevelSelect initialization complete")
    Log("Press F1 to toggle editor mode")
end

-- ============================================================================
-- BUTTON CALLBACKS
-- ============================================================================

function OnLevel2ButtonClicked()
    if not ButtonManager.CanExecuteCallback() then
        return
    end

    Log("LEVEL EDITOR button clicked!")
    ButtonManager.TransitionTo("LEVEL_2")
end

function OnLevel3ButtonClicked()
    if not ButtonManager.CanExecuteCallback() then
        return
    end

    Log("DEMO button clicked!")
    ButtonManager.TransitionTo("LEVEL_3")
end

function OnBackButtonClicked()
    if not ButtonManager.CanExecuteCallback() then
        return
    end

    Log("BACK button clicked!")
    ButtonManager.TransitionTo("mainMenu")
end

-- ============================================================================
-- LEVEL LIFECYCLE: OnUpdate
-- ============================================================================

function OnUpdate(dt)
    -- Update audio system
    UpdateAudio(dt)

    -- ButtonManager handles F1 toggle and state transitions
    ButtonManager.Update(dt)

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
    if not config then
        return
    end

    -- ButtonManager handles all button rendering and editor mode visuals
    ButtonManager.DrawAll()
end

-- ============================================================================
-- LEVEL LIFECYCLE: OnDestroy
-- ============================================================================

function OnDestroy()
    Log("LevelSelect cleanup...")

    -- Stop all sounds
    StopAllSounds()

    -- ButtonManager handles button cleanup
    ButtonManager.Cleanup()

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
    config = nil
    initialized = false
    backgroundSpriteID = 0
    logoSpriteID = 0
    cornerSpriteIDs = {}

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
    Log("  Button Count: " .. ButtonManager.GetButtonCount())

    Log("═══════════════════════════════════════")
end
