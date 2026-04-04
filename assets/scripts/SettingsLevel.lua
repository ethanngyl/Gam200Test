-- ============================================================================
-- SettingsLevel.lua
-- Author:        Sim Kah Yan
-- Email:         kahyan.sim@digipen.edu
-- Date:          2026-03-08
-- ----------------------------------------------------------------------------
--  JSON-Driven Settings Page with ButtonManager Integration
--  Blank settings page (placeholder) using JSON configuration.
-- ============================================================================

local ButtonManager = require("assets/scripts/ButtonManager")

local initialized = false
local config = nil
local backgroundSpriteID = 0
local overlaySprites = {}
local musicEnabled = true
local sfxEnabled   = true

function OnInit()
    Log("Settings Level Script Initialized (ButtonManager Version)")
    Log("Loading configuration from JSON file...")

    config = LoadJSON("assets/JSON/settings_config.json")

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
    Log("Playing settings page music: " .. music.name)

    -- Initialize buttons
    ButtonManager.Initialize(config.menu.buttons)

    -- Sync button labels to the actual current volume state
    musicEnabled = (GetMusicVolume() > 0.0)
    sfxEnabled   = (GetSfxVolume()   > 0.0)

    if not musicEnabled then
        ButtonManager.SetButtonText("toggle_music", "MUSIC: OFF")
    end
    if not sfxEnabled then
        ButtonManager.SetButtonText("toggle_sfx", "SFX: OFF")
    end

    initialized = true
    Log("Settings page initialization complete")
    Log("Press F1 to toggle editor mode")
end

-- ============================================================================
-- BUTTON CALLBACKS
-- ============================================================================

function OnToggleMusicClicked()
    if not ButtonManager.CanExecuteCallback() then return end

    musicEnabled = not musicEnabled

    if musicEnabled then
        SetMusicVolume(1.0)
        ButtonManager.SetButtonText("toggle_music", "MUSIC: ON")
        Log("Music enabled")
    else
        SetMusicVolume(0.0)
        ButtonManager.SetButtonText("toggle_music", "MUSIC: OFF")
        Log("Music disabled")
    end
end

function OnToggleSfxClicked()
    if not ButtonManager.CanExecuteCallback() then return end

    sfxEnabled = not sfxEnabled

    if sfxEnabled then
        SetSfxVolume(1.0)
        ButtonManager.SetButtonText("toggle_sfx", "SFX: ON")
        Log("SFX enabled")
    else
        SetSfxVolume(0.0)
        ButtonManager.SetButtonText("toggle_sfx", "SFX: OFF")
        Log("SFX disabled")
    end
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
    Log("Settings page cleanup...")

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

    Log("Settings page cleanup complete")
end
