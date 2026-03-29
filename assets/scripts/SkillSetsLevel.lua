-- ============================================================================
-- SkillSetsLevel.lua
-- Author:        Sim Kah Yan
-- Email:         kahyan.sim@digipen.edu
-- Date:          2026-02-24
-- Contribution:  100%
-- ----------------------------------------------------------------------------
--  JSON-Driven Skill Sets Page with ButtonManager Integration
-- ============================================================================

-- Load ButtonManager module
local ButtonManager = require("assets/scripts/ButtonManager")

-- ============================================================================
-- LEVEL STATE VARIABLES
-- ============================================================================

local initialized = false
local config = nil
local backgroundSpriteID = 0
local overlaySprites = {}

-- ============================================================================
-- LEVEL LIFECYCLE: OnInit
-- ============================================================================

function OnInit()
    Log("SkillSets Level Script Initialized (ButtonManager Version)")
    Log("Loading configuration from JSON file...")

    config = LoadJSON("assets/JSON/skillsets_config.json")

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

    -- Create overlay sprites (scroll, etc.)
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

    local music = config.menu.music
    PlayMusic(music.name, 0.8, music.loop)
    Log("Playing skill sets page music: " .. music.name)

    -- Initialize buttons
    ButtonManager.Initialize(config.menu.buttons)

    initialized = true
    Log("Skill sets page initialization complete")
    Log("Press F1 to toggle editor mode")
end

-- ============================================================================
-- BUTTON CALLBACKS
-- ============================================================================

function OnBackButtonClicked()
    if not ButtonManager.CanExecuteCallback() then
        return
    end

    Log("BACK button clicked!")
    ButtonManager.TransitionTo("CONTROL2")
end

-- ============================================================================
-- LEVEL LIFECYCLE: OnUpdate
-- ============================================================================

function OnUpdate(dt)
    UpdateAudio(dt)
    ButtonManager.Update(dt)
end

-- ============================================================================
-- LEVEL LIFECYCLE: OnDraw
-- ============================================================================

function OnDraw()
    if not config then
        return
    end

    ButtonManager.DrawAll()
end

-- ============================================================================
-- LEVEL LIFECYCLE: OnDestroy
-- ============================================================================

function OnDestroy()
    Log("Skill sets page cleanup...")

    -- Keep menu BGM playing across menu page transitions.

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

    config = nil
    initialized = false
    backgroundSpriteID = 0
    overlaySprites = {}

    Log("Skill sets page cleanup complete")
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
    Log("Skill Sets Configuration:")
    Log("  Menu Name: " .. config.menu.name)
    Log("  Music: " .. config.menu.music.name)
    Log("  Camera Zoom: " .. config.menu.camera.zoom)
    Log("  Total Buttons: " .. ButtonManager.GetButtonCount())
    Log("═══════════════════════════════════════")
end
