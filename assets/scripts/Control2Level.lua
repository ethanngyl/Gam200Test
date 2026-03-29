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

-- ============================================================================
-- LEVEL STATE VARIABLES
-- ============================================================================

local initialized = false
local config = nil
local backgroundSpriteID = 0
local overlaySprites = {}
local turnIndicatorSprites = {}
local hudSampleSprites = {}

local HUD_SAMPLE_BASE = {
    holderScaleX = 0.55,
    holderScaleY = 0.23,
    hpFillOffsetX = -0.189,
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

local HUD_SAMPLE_COLORS = {
    hp = { r = 0.9, g = 0.1, b = 0.1, a = 1.0 },
    attack = { r = 0.1, g = 0.25, b = 0.7, a = 1.0 },
    move = { r = 0.65, g = 0.35, b = 0.15, a = 1.0 }
}

local function SpawnHudSample(sample)
    if not sample then return end

    local scaleX = (sample.scale and sample.scale.x) or 0.35
    local scaleY = (sample.scale and sample.scale.y) or 0.15
    local scaleFactorX = scaleX / HUD_SAMPLE_BASE.holderScaleX
    local scaleFactorY = scaleY / HUD_SAMPLE_BASE.holderScaleY

    local baseX = sample.position and sample.position.x or 0.0
    local baseY = sample.position and sample.position.y or 0.0
    local layer = sample.layer or 9
    local holderTexture = sample.holderTexture or "assets/UI/health_ap_movement_holder.png"
    local frameTexture = sample.holderFrameTexture or "assets/UI/health_ap_movement_holder frame only.png"

    local holderID = SpawnSprite(
        holderTexture,
        baseX,
        baseY,
        scaleX,
        scaleY,
        layer
    )

    local sprites = {
        holder = holderID,
        frame = 0,
        hp = 0,
        attack = 0,
        move = 0
    }

    local function spawnFill(offsetX, offsetY, width, height, color)
        local fillID = SpawnSprite(
            "",
            baseX + offsetX,
            baseY + offsetY,
            width,
            height,
            layer + 1
        )
        if fillID and fillID > 0 then
            SetSpriteColor(fillID, color.r, color.g, color.b, color.a or 1.0)
        end
        return fillID
    end

    if sample.showHP then
        sprites.hp = spawnFill(
            HUD_SAMPLE_BASE.hpFillOffsetX * scaleFactorX,
            HUD_SAMPLE_BASE.hpFillOffsetY * scaleFactorY,
            HUD_SAMPLE_BASE.hpFillWidth * scaleFactorX,
            HUD_SAMPLE_BASE.hpFillHeight * scaleFactorY,
            HUD_SAMPLE_COLORS.hp
        )
    end

    if sample.showAttack then
        sprites.attack = spawnFill(
            HUD_SAMPLE_BASE.attackFillOffsetX * scaleFactorX,
            HUD_SAMPLE_BASE.attackFillOffsetY * scaleFactorY,
            HUD_SAMPLE_BASE.attackFillWidth * scaleFactorX,
            HUD_SAMPLE_BASE.attackFillHeight * scaleFactorY,
            HUD_SAMPLE_COLORS.attack
        )
    end

    if sample.showMove then
        sprites.move = spawnFill(
            HUD_SAMPLE_BASE.moveFillOffsetX * scaleFactorX,
            HUD_SAMPLE_BASE.moveFillOffsetY * scaleFactorY,
            HUD_SAMPLE_BASE.moveFillWidth * scaleFactorX,
            HUD_SAMPLE_BASE.moveFillHeight * scaleFactorY,
            HUD_SAMPLE_COLORS.move
        )
    end

    if frameTexture and frameTexture ~= "" then
        local frameOffsetX = sample.frameOffsetX or 0.0
        local frameOffsetY = sample.frameOffsetY or 0.0
        sprites.frame = SpawnSprite(
            frameTexture,
            baseX + frameOffsetX,
            baseY + frameOffsetY,
            scaleX,
            scaleY,
            layer + 2
        )
    end

    hudSampleSprites[sample.id or tostring(#hudSampleSprites + 1)] = sprites
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

    -- Create HUD samples (holder + single fill)
    if config.menu.hudSamples then
        Log("Creating HUD samples...")
        for _, sample in ipairs(config.menu.hudSamples) do
            SpawnHudSample(sample)
        end
        Log("HUD samples complete!")
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

    for sampleID, sprites in pairs(hudSampleSprites) do
        if sprites.holder and sprites.holder > 0 then
            DestroyEntity(sprites.holder)
        end
        if sprites.frame and sprites.frame > 0 then
            DestroyEntity(sprites.frame)
        end
        if sprites.hp and sprites.hp > 0 then
            DestroyEntity(sprites.hp)
        end
        if sprites.attack and sprites.attack > 0 then
            DestroyEntity(sprites.attack)
        end
        if sprites.move and sprites.move > 0 then
            DestroyEntity(sprites.move)
        end
        Log("HUD sample '" .. sampleID .. "' destroyed")
    end

    config = nil
    initialized = false
    backgroundSpriteID = 0
    overlaySprites = {}
    turnIndicatorSprites = {}
    hudSampleSprites = {}

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
