-- ============================================================================
-- TutorialLevel.lua
-- Author:        GE YONGQI
-- Email:         yongqi.ge@digipen.edu
-- Date:          2025-11-13
-- Contribution:  100%
--
-- REFACTORED:    2025-12-21 (Using ButtonManager module)
-- ----------------------------------------------------------------------------
--  JSON-Driven Tutorial Level with ButtonManager Integration
--
--  Purpose:
--  Tutorial level that demonstrates game mechanics using JSON configuration
--  with centralized button management via ButtonManager module.
-- ============================================================================

-- Load ButtonManager module
local ButtonManager = require("assets/scripts/ButtonManager")

-- ============================================================================
-- LEVEL STATE VARIABLES
-- ============================================================================

local initialized = false
local config = nil
local backgroundSpriteID = 0
local scrollOverlayID = 0
local overlaySprites = {}
local cornerSpriteIDs = {}

-- ============================================================================
-- LEVEL LIFECYCLE: OnInit
-- ============================================================================

function OnInit()
    Log("Tutorial Level Script Initialized (ButtonManager Version)")
    Log("Loading configuration from JSON file...")

    config = LoadJSON("assets/JSON/tutorial_config.json")

    if not config then
        Log("ERROR: Failed to load JSON configuration!")
        return
    end

    Log("Successfully loaded configuration for: " .. config.menu.name)

    local cam = config.menu.camera
    SetCameraPosition(cam.position.x, cam.position.y, cam.position.z)
    SetCameraZoom(cam.zoom)

    DisableImGui()
    SetEnginePlayState(true)

    -- Create background
    local background = config.menu.background
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

    -- Create overlay sprites
    if config.menu.overlaySprites then
        Log("Creating overlay sprites...")

        for i, overlay in ipairs(config.menu.overlaySprites) do
            local spriteID = SpawnSprite(
                overlay.texture,
                overlay.position.x,
                overlay.position.y,
                overlay.scale.x,
                overlay.scale.y,
                overlay.layer,
                overlay.rotation or 0
            )

            if spriteID > 0 then
                overlaySprites[overlay.id] = spriteID
                Log("  Overlay sprite '" .. overlay.id .. "' created (ID: " .. spriteID .. ")")
            else
                Log("  FAILED to create overlay sprite: " .. overlay.id)
            end
        end

        Log("Overlay sprites complete!")
    end

    -- Create corner sprites
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
                corner.rotation
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

    local music = config.menu.music
    PlaySound(music.name, music.loop, music.volume)
    Log("Playing tutorial music: " .. music.name)

    -- ========================================================================
    -- INITIALIZE BUTTON MANAGER
    -- ========================================================================
    ButtonManager.Initialize(config.menu.buttons)

    initialized = true
    Log("Tutorial initialization complete")
    Log("Press F1 to toggle editor mode")
end

-- ============================================================================
-- BUTTON CALLBACKS
-- ============================================================================

function OnNextButtonClicked()
    if not ButtonManager.CanExecuteCallback() then
        return
    end

    Log("NEXT button clicked!")
    ButtonManager.TransitionTo("Level_select")
end

-- ============================================================================
-- LEVEL LIFECYCLE: OnUpdate
-- ============================================================================

function OnUpdate(dt)
    UpdateAudio(dt)

    -- ButtonManager handles F1 toggle and state transitions
    ButtonManager.Update(dt)
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
    Log("Tutorial cleanup...")

    StopAllSounds()

    -- ButtonManager handles button cleanup
    ButtonManager.Cleanup()

    if backgroundSpriteID > 0 then
        DestroyEntity(backgroundSpriteID)
        Log("Background sprite destroyed")
    end

    for overlayID, spriteID in pairs(overlaySprites) do
        if spriteID > 0 then
            DestroyEntity(spriteID)
            Log("Overlay sprite '" .. overlayID .. "' destroyed")
        end
    end

    for cornerID, spriteID in pairs(cornerSpriteIDs) do
        if spriteID > 0 then
            DestroyEntity(spriteID)
            Log("Corner sprite '" .. cornerID .. "' destroyed")
        end
    end

    config = nil
    initialized = false
    backgroundSpriteID = 0
    overlaySprites = {}
    cornerSpriteIDs = {}

    Log("Tutorial cleanup complete")
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
    Log("Tutorial Configuration:")
    Log("  Menu Name: " .. config.menu.name)
    Log("  Music: " .. config.menu.music.name)
    Log("  Camera Zoom: " .. config.menu.camera.zoom)
    Log("  Total Buttons: " .. ButtonManager.GetButtonCount())
    Log("═══════════════════════════════════════")
end
