-- ============================================================================
-- LoseLevel.lua
-- Author:        Sim Kah Yan
-- Email:         kahyan.sim@digipen.edu
-- Date:          2026-02-24
-- ----------------------------------------------------------------------------
--  Lose screen (JSON-driven) with ButtonManager Integration
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
local cornerSpriteIDs = {}

-- ============================================================================
-- LEVEL LIFECYCLE: OnInit
-- ============================================================================

function OnInit()
    Log("Lose Level Script Initialized (ButtonManager Version)")
    Log("Loading configuration from JSON file...")

    config = LoadJSON("assets/JSON/LoseLevelConfig.json")

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
    Log("Playing lose screen music: " .. music.name)

    -- Create scroll overlay from JSON config
    if config.menu.scrollOverlay then
        local scroll = config.menu.scrollOverlay
        scrollOverlayID = SpawnSprite(
            scroll.texture,
            cam.position.x + scroll.position.x,
            cam.position.y + scroll.position.y,
            scroll.scale.x, scroll.scale.y,
            scroll.layer
        )
        if scrollOverlayID > 0 then
            Log("Scroll overlay created (ID: " .. scrollOverlayID .. ")")
        end
    end

    ButtonManager.Initialize(config.menu.buttons)

    initialized = true
    Log("Lose screen initialization complete")
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

    -- Draw scroll overlay text from JSON config
    if config.menu.scrollOverlay and config.menu.scrollOverlay.text then
        local fbW, fbH = GetFramebufferSize()
        if fbW and fbW > 0 then
            local t = config.menu.scrollOverlay.text
            local scaleRef = fbW / 1920
            local approxWidth = #t.content * 48 * 0.6 * (t.scale or 1.0) * scaleRef
            local textX = (fbW * 0.5) - (approxWidth * 0.5)
            local textY = (fbH * 0.5) + (40 * scaleRef)
            DrawText(t.font, t.content, textX, textY, (t.scale or 1.0) * scaleRef, t.color.r, t.color.g, t.color.b)
        end
    end
end

-- ============================================================================
-- LEVEL LIFECYCLE: OnDestroy
-- ============================================================================

function OnDestroy()
    Log("Lose screen cleanup...")

    StopAllSounds()
    ButtonManager.Cleanup()

    if backgroundSpriteID > 0 then
        DestroyEntity(backgroundSpriteID)
    end

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
    scrollOverlayID = 0
    cornerSpriteIDs = {}

    Log("Lose screen cleanup complete")
end
