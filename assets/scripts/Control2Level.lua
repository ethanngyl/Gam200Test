-- ============================================================================
-- Control2Level.lua
-- Author:        Sim Kah Yan
-- Email:         kahyan.sim@digipen.edu
-- Date:          2026-02-24
-- Contribution:  100%
-- ----------------------------------------------------------------------------
--  JSON-Driven Control Page (Page 2) with ButtonManager Integration
-- ============================================================================

-- Load ButtonManager module
local ButtonManager = require("assets/scripts/ButtonManager")
local HealthUI = require("UI/HealthUI")

-- ============================================================================
-- LEVEL STATE VARIABLES
-- ============================================================================

local initialized = false
local config = nil
local backgroundSpriteID = 0
local overlaySprites = {}
local turnIndicatorSprites = {}
local hudSampleUIs = {}

local HUD_SAMPLE_BASE = {
    holderScaleX = 0.55,
    holderScaleY = 0.23,
    hpFillOffsetX = -0.181,
    hpFillOffsetY = 0.05,
    hpFillWidth = 0.085,
    hpFillHeight = 0.085,
    attackFillOffsetX = 0.0429,
    attackFillOffsetY = 0.05,
    attackFillWidth = 0.365,
    attackFillHeight = 0.07,
    moveFillOffsetX = 0.015,
    moveFillOffsetY = -0.047,
    moveFillWidth = 0.427,
    moveFillHeight = 0.07
}

local function CreateHudSample(sample)
    if not sample or not sample.position or not sample.scale then
        return nil
    end

    local scaleX = sample.scale.x or HUD_SAMPLE_BASE.holderScaleX
    local scaleY = sample.scale.y or HUD_SAMPLE_BASE.holderScaleY
    local ratioX = scaleX / HUD_SAMPLE_BASE.holderScaleX
    local ratioY = scaleY / HUD_SAMPLE_BASE.holderScaleY

    local ui = HealthUI:New()
    ui:Init({
        maxHP = 5,
        holderTexture = "assets/UI/health_ap_movement_holder.png",
        holderFrameTexture = "assets/UI/health_ap_movement_holder frame only.png",
        holderOnly = false,
        enableHPFill = sample.showHP == true,
        enableAttackFill = sample.showAttack == true,
        enableMoveFill = sample.showMove == true,
        holderScaleX = scaleX,
        holderScaleY = scaleY,
        holderOffsetX = sample.position.x or 0.0,
        holderOffsetY = sample.position.y or 0.0,
        holderFrameScaleX = scaleX,
        holderFrameScaleY = scaleY,
        holderFrameOffsetX = (sample.position.x or 0.0) + (sample.frameOffsetX or 0.0),
        holderFrameOffsetY = (sample.position.y or 0.0) + (sample.frameOffsetY or 0.0),
        hpFillOffsetX = HUD_SAMPLE_BASE.hpFillOffsetX * ratioX,
        hpFillOffsetY = HUD_SAMPLE_BASE.hpFillOffsetY * ratioY,
        hpFillWidth = HUD_SAMPLE_BASE.hpFillWidth * ratioX,
        hpFillHeight = HUD_SAMPLE_BASE.hpFillHeight * ratioY,
        attackFillOffsetX = HUD_SAMPLE_BASE.attackFillOffsetX * ratioX,
        attackFillOffsetY = HUD_SAMPLE_BASE.attackFillOffsetY * ratioY,
        attackFillWidth = HUD_SAMPLE_BASE.attackFillWidth * ratioX,
        attackFillHeight = HUD_SAMPLE_BASE.attackFillHeight * ratioY,
        moveFillOffsetX = HUD_SAMPLE_BASE.moveFillOffsetX * ratioX,
        moveFillOffsetY = HUD_SAMPLE_BASE.moveFillOffsetY * ratioY,
        moveFillWidth = HUD_SAMPLE_BASE.moveFillWidth * ratioX,
        moveFillHeight = HUD_SAMPLE_BASE.moveFillHeight * ratioY,
        hpFillColor = { r = 0.9, g = 0.1, b = 0.1, a = 1.0 },
        attackFillColor = { r = 0.1, g = 0.25, b = 0.7, a = 1.0 },
        moveFillColor = { r = 0.65, g = 0.35, b = 0.15, a = 1.0 },
        hpFillTexture = "assets/TileMap/Attack_Indicator.png",
        textColor = { r = 0.0, g = 0.0, b = 0.0, a = 1.0 },
        textScale = 0.6,
        showValueText = false,
        getMovementAPFunc = function() return 3, 3 end,
        getAttackAPFunc = function() return 3, 3 end,
        layer = sample.layer or 9,
        textureBasePath = "assets/UI/Health_"
    })

    return ui
end

-- ============================================================================
-- LEVEL LIFECYCLE: OnInit
-- ============================================================================

function OnInit()
    Log("Control2 Level Script Initialized (ButtonManager Version)")
    Log("Loading configuration from JSON file...")

    config = LoadJSON("assets/JSON/control2_config.json")

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

    -- Create turn indicator icons (static frames from sprite sheet)
    if config.menu.turnIndicators then
        Log("Creating turn indicator icons...")

        for i, indicator in ipairs(config.menu.turnIndicators) do
            local rows = indicator.rows or 2
            local cols = indicator.cols or 4
            local frameTime = indicator.frameTime or 0.15
            local rowIndex = indicator.row or 0

            local spriteID = SpawnAnimatedSprite(
                indicator.texture,
                indicator.position.x,
                indicator.position.y,
                indicator.scale.x,
                indicator.scale.y,
                indicator.layer,
                rows,
                cols,
                rows * cols,
                frameTime,
                false
            )

            if spriteID > 0 then
                local framesPerRow = cols
                local startFrame = rowIndex * framesPerRow
                SetAnimationFrameRange(spriteID, startFrame, framesPerRow, true)
                SetAnimationLoop(spriteID, false)
                SetAnimationPlaying(spriteID, false)
                SetAnimationFrame(spriteID, startFrame)

                turnIndicatorSprites[indicator.id] = spriteID
                Log("  Turn indicator '" .. indicator.id .. "' created (ID: " .. spriteID .. ")")
            else
                Log("  FAILED to create turn indicator: " .. indicator.id)
            end
        end

        Log("Turn indicator icons complete!")
    end

    -- Create HUD sample bars (status bar examples)
    if config.menu.hudSamples then
        Log("Creating HUD sample bars...")
        for i, sample in ipairs(config.menu.hudSamples) do
            local ui = CreateHudSample(sample)
            if ui then
                local sampleID = sample.id or ("sample_" .. tostring(i))
                hudSampleUIs[sampleID] = ui
                Log("  HUD sample '" .. sampleID .. "' created")
            else
                Log("  FAILED to create HUD sample at index " .. tostring(i))
            end
        end
        Log("HUD sample bars complete!")
    end

    local music = config.menu.music
    PlayMusic(music.name, 0.8, music.loop)
    Log("Playing control page music: " .. music.name)

    -- Initialize buttons
    ButtonManager.Initialize(config.menu.buttons)

    initialized = true
    Log("Control2 page initialization complete")
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
    ButtonManager.TransitionTo("CONTROL")
end

function OnNextButtonClicked()
    if not ButtonManager.CanExecuteCallback() then
        return
    end

    Log("NEXT button clicked!")
    ButtonManager.TransitionTo("SKILL_SETS")
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
    Log("Control2 page cleanup...")

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

    for indicatorID, spriteID in pairs(turnIndicatorSprites) do
        if spriteID > 0 then
            DestroyEntity(spriteID)
            Log("Turn indicator '" .. indicatorID .. "' destroyed")
        end
    end

    for sampleID, ui in pairs(hudSampleUIs) do
        if ui and ui.Destroy then
            ui:Destroy()
            Log("HUD sample '" .. sampleID .. "' destroyed")
        end
    end

    config = nil
    initialized = false
    backgroundSpriteID = 0
    overlaySprites = {}
    turnIndicatorSprites = {}
    hudSampleUIs = {}

    Log("Control2 page cleanup complete")
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
    Log("Control2 Configuration:")
    Log("  Menu Name: " .. config.menu.name)
    Log("  Music: " .. config.menu.music.name)
    Log("  Camera Zoom: " .. config.menu.camera.zoom)
    Log("  Total Buttons: " .. ButtonManager.GetButtonCount())
    Log("═══════════════════════════════════════")
end
