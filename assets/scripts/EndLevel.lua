-- ============================================================================
-- EndLevel.lua
-- Author:        Josh Ong
-- Email:         Josh.o@digipen.edu
-- Date:          2025-11-13
--
-- REFACTORED:    2025-12-21 (Using ButtonManager module)
-- ----------------------------------------------------------------------------
-- Description:
-- End level/victory screen with JSON-driven UI configuration. Now uses
-- centralized ButtonManager module to eliminate code duplication.
-- ============================================================================

-- Load ButtonManager module
local ButtonManager = require("assets/scripts/ButtonManager")

-- ============================================================================
-- LEVEL STATE VARIABLES
-- ============================================================================

local initialized = false
local config = nil
local backgroundSpriteID = 0  -- Store background sprite entity ID
local logoSpriteID = 0        -- Store logo sprite entity ID
local cornerSpriteIDs = {}    -- Store corner sprite entity IDs

-- Level progression
local currentLevel = 1
local totalLevels  = 3
local hasMoreLevels = false

-- ============================================================================
-- LEVEL LIFECYCLE: OnInit
-- ============================================================================

function OnInit()
    Log("End Level Script Initialized (ButtonManager Version)")
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

    -- CRITICAL: Force editor camera to sync with mainCamera for this level
    SetEnginePlayState(false)  -- Sync editorCamera ← mainCamera
    SetEnginePlayState(true)   -- Back to playing mode

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
                Log("  Corner sprite '" .. corner.id .. "' created (ID: " .. spriteID .. ", rotation: " .. corner.rotation .. " degrees)")
            else
                Log("  FAILED to create corner sprite: " .. corner.id)
            end
        end

        Log("Corner decorations complete!")
    end

    -- Start menu background music from JSON config
    local music = config.menu.music
    PlayMusic(music.name, 0.8, music.loop)
    Log("Playing menu music: " .. music.name)

    -- ========================================================================
    -- INITIALIZE BUTTON MANAGER
    -- ========================================================================
    ButtonManager.Initialize(config.menu.buttons)

    -- ========================================================================
    -- LEVEL PROGRESSION: Add "Next Level" button if more levels remain
    -- ========================================================================
    local progress = LoadJSON("assets/JSON/LevelProgress.json")
    if progress then
        currentLevel = progress.currentLevel or 1
        totalLevels  = progress.totalLevels  or 3
    end

    hasMoreLevels = (currentLevel <= totalLevels)

    if hasMoreLevels then
        Log("[EndLevel] Level " .. currentLevel .. " / " .. totalLevels .. " - showing Next Level button")
        ButtonManager.CreateButton({
            id       = "nextLevel",
            texture  = "assets/Menu/Ui_btn.png",
            position = { x = 0.0, y = -0.05 },
            scale    = { x = 1.0, y = 0.2 },
            layer    = 10,
            text     = {
                content = "NEXT LEVEL",
                font    = "Jersey20Regular",
                offset  = { x = -80.0, y = -13.0 },
                scale   = 0.75,
                color   = { r = 0.2, g = 1.0, b = 0.2 },
            },
            callback = "OnNextLevelButtonClicked",
        })
    else
        Log("[EndLevel] All " .. totalLevels .. " levels complete!")
    end

    initialized = true
    Log("End Level initialization complete")
    Log("Press F1 to toggle editor")
end

-- ============================================================================
-- BUTTON CALLBACKS
-- ============================================================================

function OnNextLevelButtonClicked()
    if not ButtonManager.CanExecuteCallback() then
        return
    end

    Log("NEXT LEVEL button clicked! Loading level " .. currentLevel .. "...")
    ButtonManager.TransitionTo("LEVEL_3")
end

function OnBackButtonClicked()
    if not ButtonManager.CanExecuteCallback() then
        return
    end

    Log("BACK button clicked!")

    -- Reset level progress when returning to main menu
    local f = io.open("assets/JSON/LevelProgress.json", "w")
    if f then
        f:write("{\n  \"currentLevel\": 1,\n  \"totalLevels\": " .. totalLevels .. "\n}\n")
        f:close()
        Log("[EndLevel] Level progress reset to 1")
    end

    ButtonManager.TransitionTo("mainMenu", "button2")
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

    -- Level completion text
    local fbW, fbH = GetFramebufferSize()
    if fbW and fbW > 0 then
        local cx = fbW * 0.5
        local scaleRef = fbW / 1920
        if hasMoreLevels then
            DrawText("Jersey20Regular", "LEVEL " .. (currentLevel - 1) .. " COMPLETE!",
                cx - 160 * scaleRef, 80 * scaleRef, 1.0 * scaleRef, 1, 1, 0.4)
        else
            DrawText("Jersey20Regular", "ALL LEVELS COMPLETE!",
                cx - 180 * scaleRef, 80 * scaleRef, 1.0 * scaleRef, 1, 0.85, 0.2)
        end
    end
end

-- ============================================================================
-- LEVEL LIFECYCLE: OnDestroy
-- ============================================================================

function OnDestroy()
    Log("End Level Cleanup...")

    -- Stop all sounds
    StopMusic(0.5)

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

    Log("End Level cleanup complete")
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
    Log("End Level Configuration (from JSON):")
    Log("  Menu Name: " .. config.menu.name)
    Log("  Music: " .. config.menu.music.name)
    Log("  Camera Zoom: " .. config.menu.camera.zoom)
    Log("  Button Count: " .. ButtonManager.GetButtonCount())

    Log("═══════════════════════════════════════")
end
