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

-- ============================================================================
-- VOLUME SLIDER STATE
-- ============================================================================
local SLIDER_TEXTURE_BASE = "assets/UI/sprite_volumeslider"
local SLIDER_OFFSET_X = 0.0    -- offset from camera centre (world units)
local SLIDER_OFFSET_Y = -0.34
local SLIDER_SCALE_X  = 0.8
local SLIDER_SCALE_Y  = 0.12
local SLIDER_LAYER    = 11     -- above buttons (layer 10) and overlay (layer 2)

local sliderID        = 0
local volumeLevel     = 0    -- 0 = max (1.0), 9 = min (0.1)
local wasLeftPressed  = false
local wasRightPressed = false

local function LevelToVolume(level)
    if level >= 9 then return 0.0 end
    return 1.0 - (level * 0.1)
end

local function VolumeToLevel(volume)
    if volume <= 0.05 then return 9 end
    volume = math.min(1.0, volume)
    local level = math.floor((1.0 - volume) * 10 + 0.5)
    return math.max(0, math.min(8, level))
end

local function GetSliderTexture(level)
    return SLIDER_TEXTURE_BASE .. string.format("%02d", level) .. ".png"
end

local function SpawnSlider()
    if sliderID > 0 then DestroyEntity(sliderID) end
    local camX, camY = GetCameraPosition()
    local tex = GetSliderTexture(volumeLevel)
    Log("[SettingsLevel] Spawning slider: " .. tex .. " at (" .. tostring(camX + SLIDER_OFFSET_X) .. ", " .. tostring(camY + SLIDER_OFFSET_Y) .. ")")
    sliderID = SpawnSprite(
        tex,
        camX + SLIDER_OFFSET_X,
        camY + SLIDER_OFFSET_Y,
        SLIDER_SCALE_X,
        SLIDER_SCALE_Y,
        SLIDER_LAYER
    )
    Log("[SettingsLevel] Slider entity ID: " .. tostring(sliderID))
end

local function ApplyVolume()
    local volume = LevelToVolume(volumeLevel)
    SetMasterVolume(volume)
    SaveMasterVolume(volume)
    SpawnSlider()
    Log("[SettingsLevel] Volume -> " .. tostring(volume) .. " (level " .. volumeLevel .. ")")
end

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

    -- Initialize volume slider from saved master volume
    if GetMasterVolume then
        volumeLevel = VolumeToLevel(GetMasterVolume())
    end
    SpawnSlider()

    -- Sync button labels to the actual current volume state
    if GetMusicVolume then
        musicEnabled = (GetMusicVolume() > 0.0)
    end
    if GetSfxVolume then
        sfxEnabled = (GetSfxVolume() > 0.0)
    end

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
        if SetMusicVolume then SetMusicVolume(1.0) end
        ButtonManager.SetButtonText("toggle_music", "MUSIC: ON")
        Log("Music enabled")
    else
        if SetMusicVolume then SetMusicVolume(0.0) end
        ButtonManager.SetButtonText("toggle_music", "MUSIC: OFF")
        Log("Music disabled")
    end
end

function OnToggleSfxClicked()
    if not ButtonManager.CanExecuteCallback() then return end

    sfxEnabled = not sfxEnabled

    if sfxEnabled then
        if SetSfxVolume then SetSfxVolume(1.0) end
        ButtonManager.SetButtonText("toggle_sfx", "SFX: ON")
        Log("SFX enabled")
    else
        if SetSfxVolume then SetSfxVolume(0.0) end
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

    -- Volume slider keyboard input
    local leftDown  = IsKeyDown("Left")
    local rightDown = IsKeyDown("Right")

    if leftDown and not wasLeftPressed then
        if volumeLevel < 9 then
            volumeLevel = volumeLevel + 1
            ApplyVolume()
        end
    end
    if rightDown and not wasRightPressed then
        if volumeLevel > 0 then
            volumeLevel = volumeLevel - 1
            ApplyVolume()
        end
    end
    wasLeftPressed  = leftDown
    wasRightPressed = rightDown

    -- Volume slider mouse click
    local fbWidth, fbHeight = GetFramebufferSize()
    local centerX = fbWidth  * 0.5
    local centerY = fbHeight * 0.5
    local scaleX  = fbWidth  / 1920
    local scaleY  = fbHeight / 1080

    -- Map slider world position to screen coordinates
    -- With camera at (0,0) zoom=1, visible half-height ≈ 1.25 world units
    local sliderScreenX = centerX + (SLIDER_OFFSET_X) * (fbWidth  * 0.5 / 2.35)
    local sliderScreenY = centerY - (SLIDER_OFFSET_Y) * (fbHeight * 0.5 / 1.25)
    local halfW = 200 * scaleX
    local halfH =  40 * scaleY

    local mx, my = GetMousePosition()
    local hovering = mx >= sliderScreenX - halfW and mx <= sliderScreenX + halfW
                 and my >= sliderScreenY - halfH and my <= sliderScreenY + halfH

    if IsMouseButtonPressed("Left") and hovering then
        if mx < sliderScreenX then
            if volumeLevel < 9 then volumeLevel = volumeLevel + 1; ApplyVolume() end
        else
            if volumeLevel > 0 then volumeLevel = volumeLevel - 1; ApplyVolume() end
        end
    end
end

-- ============================================================================
-- LEVEL LIFECYCLE: OnDraw
-- ============================================================================

function OnDraw()
    if not config then return end

    ButtonManager.DrawAll()

    -- Draw volume slider label and instruction
    local fbWidth, fbHeight = GetFramebufferSize()
    local centerX = fbWidth  * 0.5
    local centerY = fbHeight * 0.5
    local scaleX  = fbWidth  / 1920
    local scaleY  = fbHeight / 1080
    local scale   = math.min(scaleX, scaleY)

    local sliderScreenY = centerY - SLIDER_OFFSET_Y * (fbHeight * 0.5 / 1.25)

    -- "< Volume >" label above slider
    local labelText  = "< Volume >"
    local labelScale = 0.8 * scale
    local labelW     = #labelText * 18 * labelScale
    local labelX     = centerX - labelW * 0.5
    local labelY     = sliderScreenY - 250
    DrawText("Jersey20Regular", labelText, labelX, labelY, labelScale, 0.2, 0.15, 0.1)

    -- Instruction text below slider
    local instrText  = "Left / Right arrow keys to adjust volume."
    local instrScale = 0.65 * scale
    local instrW     = #instrText * 13 * instrScale
    local instrX     = centerX - instrW * 0.6
    local instrY     = sliderScreenY - 370
    DrawText("Jersey20Regular", instrText, instrX, instrY, instrScale, 0.2, 0.15, 0.1)
end

-- ============================================================================
-- LEVEL LIFECYCLE: OnDestroy
-- ============================================================================

function OnDestroy()
    Log("Settings page cleanup...")

    -- Keep menu BGM playing across menu page transitions.

    ButtonManager.Cleanup()

    if sliderID > 0 then
        DestroyEntity(sliderID)
        sliderID = 0
    end

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
