-- ============================================================================
-- WinLevel.lua
-- Author:        Sim Kah Yan
-- Email:         kahyan.sim@digipen.edu
-- Date:          2026-02-24
-- ----------------------------------------------------------------------------
--  Win screen (JSON-driven) with ButtonManager Integration
-- ============================================================================

-- Load ButtonManager module
local ButtonManager = require("assets/scripts/ButtonManager")

-- ============================================================================
-- LEVEL STATE VARIABLES
-- ============================================================================

local initialized = false
local config = nil
local backgroundSpriteID = 0
local scrollOverlayID = 0     -- ADDED: Scroll overlay behind win text
local cornerSpriteIDs = {}

-- ============================================================================
-- LEVEL LIFECYCLE: OnInit
-- ============================================================================

function OnInit()
    Log("Win Level Script Initialized (ButtonManager Version)")
    Log("Loading configuration from JSON file...")

    config = LoadJSON("assets/JSON/WinLevelConfig.json")

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

    -- ========================================================================
    -- ADDED: CREATE SCROLL OVERLAY (behind "YOU WIN!" text from JSON config)
    -- text_title button is at (-0.5, 0.2) - scroll centers behind it
    -- ========================================================================
    scrollOverlayID = SpawnSprite(
        "assets/Menu/Scroll Overlay.png",
        cam.position.x,
        cam.position.y + 0.2,
        1.2, 0.4,
        8  -- Above background (1) and corners (5), below buttons (10)
    )
    if scrollOverlayID > 0 then
        Log("Scroll overlay created (ID: " .. scrollOverlayID .. ")")
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
                Log("  Corner sprite '" .. corner.id .. "' created (ID: " .. spriteID .. ")")
            else
                Log("  FAILED to create corner sprite: " .. corner.id)
            end
        end

        Log("Corner decorations complete!")
    end

    local music = config.menu.music
    PlaySound(music.name, music.loop, music.volume)
    Log("Playing win screen music: " .. music.name)

    ButtonManager.Initialize(config.menu.buttons)

    initialized = true
    Log("Win screen initialization complete")
end

-- ============================================================================
-- BUTTON CALLBACKS
-- ============================================================================

function OnBackButtonClicked()
    if not ButtonManager.CanExecuteCallback() then
        return
    end

    Log("BACK button clicked!")
    ButtonManager.TransitionTo("mainMenu", "button2")
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
    Log("Win screen cleanup...")

    StopAllSounds()
    ButtonManager.Cleanup()

    if backgroundSpriteID > 0 then
        DestroyEntity(backgroundSpriteID)
    end

    -- ADDED: Destroy scroll overlay
    if scrollOverlayID > 0 then
        DestroyEntity(scrollOverlayID)
    end

    for cornerID, spriteID in pairs(cornerSpriteIDs) do
        if spriteID > 0 then
            DestroyEntity(spriteID)
        end
    end

    config = nil
    initialized = false
    backgroundSpriteID = 0
    scrollOverlayID = 0    -- ADDED
    cornerSpriteIDs = {}

    Log("Win screen cleanup complete")
end