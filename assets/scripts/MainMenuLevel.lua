-- ============================================================================
-- MainMenuLevel.lua
-- Author:        GE YONGQI
-- Email:         yongqi.ge@digipen.edu
-- Date:          2025-11-13
-- Contribution:  100%
--
-- REFACTORED:    2025-12-21 (Using ButtonManager module)
-- ----------------------------------------------------------------------------
--  JSON-Driven Main Menu with ButtonManager Integration
--
--  Purpose:
--  Complete main menu level script using JSON configuration for all UI
--  elements. Now uses centralized ButtonManager module to eliminate code
--  duplication.
--
--  Features:
--  - Loads UI configuration from mainmenu_config.json
--  - Creates layered sprite system (background, logo, corner decorations)
--  - Uses ButtonManager for all button operations
--  - F1 editor mode toggle with visual feedback (handled by ButtonManager)
--  - Audio integration with background music
--  - Smooth state transitions with delayed execution
--
--  Level Lifecycle:
--  - OnInit()     : Load JSON, setup camera, spawn sprites, init ButtonManager
--  - OnUpdate(dt) : Update ButtonManager (handles F1, transitions, etc.)
--  - OnDraw()     : ButtonManager.DrawAll() renders all buttons automatically
--  - OnDestroy()  : Cleanup entities, ButtonManager handles button cleanup
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
local wasWinKeyDown = false
local wasLoseKeyDown = false

-- Settings button (custom animated sprite)
local settingsButton = {
    spriteID = 0,
    config = nil,
    state = "idle",
    releaseTimer = 0.0,
    releaseDuration = 0.15,
    wasMouseDown = false,
    position = { x = 0.0, y = 0.0 },
    scaleX = 0.18,
    scaleY = 0.18,
    layer = 2,
    rows = 2,
    cols = 2,
    frameCount = 4,
    frameTime = 0.15,
    useViewportCoords = false
}

local SETTINGS_FRAME = {
    idle = 0,     -- top-left wrench
    pressed = 1,  -- top-right wrench
    released = 2  -- bottom-left wrench
}

local function ApplySettingsButtonFrame(state)
    if not settingsButton.spriteID or settingsButton.spriteID <= 0 then
        return
    end
    local frame = SETTINGS_FRAME[state]
    if frame == nil then
        frame = SETTINGS_FRAME.idle
    end
    if SetAnimationFrame then
        SetAnimationFrame(settingsButton.spriteID, frame)
    end
end

local function CreateSettingsButton()
    if not settingsButton.config then
        return
    end
    local cfg = settingsButton.config
    local pos = cfg.position or { x = 0.0, y = 0.0 }
    local scale = cfg.scale or { x = 0.18, y = 0.18 }
    local uniformScale = math.min(scale.x or 0.18, scale.y or 0.18)
    local layer = cfg.layer or 2
    local texture = cfg.texture or "assets/UI/Settings_ParchmentButtonSettings.png"

    settingsButton.position = { x = pos.x, y = pos.y }
    settingsButton.scaleX = uniformScale
    settingsButton.scaleY = uniformScale
    settingsButton.layer = layer
    settingsButton.state = "idle"
    settingsButton.releaseTimer = 0.0
    settingsButton.wasMouseDown = false

    if SpawnAnimatedSprite then
        settingsButton.spriteID = SpawnAnimatedSprite(
            texture,
            settingsButton.position.x,
            settingsButton.position.y,
            settingsButton.scaleX,
            settingsButton.scaleY,
            settingsButton.layer,
            settingsButton.rows,
            settingsButton.cols,
            settingsButton.frameCount,
            settingsButton.frameTime,
            false
        )
        if settingsButton.spriteID and settingsButton.spriteID > 0 then
            SetAnimationFrameRange(settingsButton.spriteID, 0, settingsButton.frameCount, true)
            SetAnimationLoop(settingsButton.spriteID, false)
            SetAnimationPlaying(settingsButton.spriteID, false)
        end
    else
        settingsButton.spriteID = SpawnSprite(
            texture,
            settingsButton.position.x,
            settingsButton.position.y,
            settingsButton.scaleX,
            settingsButton.scaleY,
            settingsButton.layer
        )
    end

    ApplySettingsButtonFrame("idle")
end

local function IsMouseOverSettingsButton()
    if not settingsButton.spriteID or settingsButton.spriteID <= 0 then
        return false
    end
    if not GetMouseWorldPosition then
        return false
    end

    local mouseWX, mouseWY = GetMouseWorldPosition()

    local halfW = settingsButton.scaleX * 0.5
    local halfH = settingsButton.scaleY * 0.5

    local left   = settingsButton.position.x - halfW
    local right  = settingsButton.position.x + halfW
    local bottom = settingsButton.position.y - halfH
    local top    = settingsButton.position.y + halfH

    return mouseWX >= left and mouseWX <= right and mouseWY >= bottom and mouseWY <= top
end

local function UpdateSettingsButton(dt)
    if not settingsButton.spriteID or settingsButton.spriteID <= 0 then
        return
    end

    if ShouldDisableGameplay and ShouldDisableGameplay() then
        if settingsButton.state ~= "idle" then
            settingsButton.state = "idle"
            ApplySettingsButtonFrame("idle")
        end
        settingsButton.wasMouseDown = false
        return
    end

    if settingsButton.releaseTimer and settingsButton.releaseTimer > 0 then
        settingsButton.releaseTimer = settingsButton.releaseTimer - dt
        if settingsButton.releaseTimer <= 0 then
            settingsButton.state = "idle"
            ApplySettingsButtonFrame("idle")
        end
        settingsButton.wasMouseDown = IsMouseButtonDown and IsMouseButtonDown("Left") or false
        return
    end

    local hovering = IsMouseOverSettingsButton()
    local mouseDown = IsMouseButtonDown and IsMouseButtonDown("Left") or false

    if hovering and mouseDown then
        if settingsButton.state ~= "pressed" then
            settingsButton.state = "pressed"
            ApplySettingsButtonFrame("pressed")
        end
    else
        if settingsButton.state ~= "idle" then
            settingsButton.state = "idle"
            ApplySettingsButtonFrame("idle")
        end
    end

    if settingsButton.wasMouseDown and not mouseDown and hovering then
        settingsButton.state = "released"
        ApplySettingsButtonFrame("released")
        settingsButton.releaseTimer = settingsButton.releaseDuration
        OnSettingsButtonClicked()
    end

    settingsButton.wasMouseDown = mouseDown
end

-- ============================================================================
-- LEVEL LIFECYCLE: OnInit
-- ============================================================================

function OnInit()
    Log("MainMenu Level Script Initialized (ButtonManager Version)")
    Log("Loading configuration from JSON file...")

    -- Load configuration from JSON
    config = LoadJSON("assets/JSON/mainmenu_config.json")

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

    -- ========================================================================
    -- CREATE BACKGROUND SPRITE
    -- ========================================================================
    local background = config.menu.background or {
        texture = "assets/Menu/WoodBackground.png",
        position = { x = 0.0, y = 0.0 },
        scale = { x = 2.0, y = 2.0 },
        layer = -10
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
    -- CREATE LOGO SPRITE
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

    -- Start menu background music from JSON config
    local music = config.menu.music
    PlayMusic(music.name, 0.8, music.loop)
    Log("Playing menu music: " .. music.name)

    -- ========================================================================
    -- INITIALIZE BUTTON MANAGER
    -- ========================================================================
    local menuButtons = {}
    if config.menu.buttons then
        for _, button in ipairs(config.menu.buttons) do
            if button.id == "settings" then
                settingsButton.config = button
            else
                table.insert(menuButtons, button)
            end
        end
    end
    ButtonManager.Initialize(menuButtons)
    CreateSettingsButton()

    initialized = true
    Log("MainMenu initialization complete")
    Log("Press F1 to toggle editor mode")
end

-- ============================================================================
-- BUTTON CALLBACKS
-- ============================================================================

function OnPlayButtonClicked()
    -- Check if button can execute (handled by ButtonManager)
    if not ButtonManager.CanExecuteCallback() then
        return
    end

    Log("PLAY button clicked!")
    ButtonManager.TransitionTo("TUTORIAL")
end

function OnHowToPlayButtonClicked()
    -- Check if button can execute (handled by ButtonManager)
    if not ButtonManager.CanExecuteCallback() then
        return
    end

    Log("HOW TO PLAY button clicked!")
    ButtonManager.TransitionTo("CONTROL")
end

function OnSettingsButtonClicked()
    -- Check if button can execute (handled by ButtonManager)
    if not ButtonManager.CanExecuteCallback() then
        return
    end

    Log("SETTINGS button clicked!")
    ButtonManager.TransitionTo("SETTINGS")
end

function OnExitButtonClicked()
    -- Check if button can execute (handled by ButtonManager)
    if not ButtonManager.CanExecuteCallback() then
        return
    end

    Log("EXIT button clicked!")
    ButtonManager.TransitionTo("GS_QUIT")
end

-- ============================================================================
-- LEVEL LIFECYCLE: OnUpdate
-- ============================================================================

function OnUpdate(dt)
    UpdateAudio(dt)

    -- ButtonManager handles F1 toggle and state transitions
    ButtonManager.Update(dt)
    UpdateSettingsButton(dt)

    -- Debug: quick access to win/lose screens
    local winKeyDown = IsKeyDown and IsKeyDown("7") or false
    if winKeyDown and not wasWinKeyDown then
        SetNextGameState("WIN_SCREEN")
    end
    wasWinKeyDown = winKeyDown

    local loseKeyDown = IsKeyDown and IsKeyDown("8") or false
    if loseKeyDown and not wasLoseKeyDown then
        SetNextGameState("LOSE_SCREEN")
    end
    wasLoseKeyDown = loseKeyDown
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
    Log("MainMenu cleanup...")

    -- Keep menu BGM playing across menu page transitions.

    -- ButtonManager handles button cleanup
    ButtonManager.Cleanup()

    -- Destroy settings button sprite
    if settingsButton.spriteID and settingsButton.spriteID > 0 then
        DestroyEntity(settingsButton.spriteID)
        settingsButton.spriteID = 0
    end

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

    Log("MainMenu cleanup complete")
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
    Log("MainMenu Configuration (from JSON):")
    Log("  Menu Name: " .. config.menu.name)
    Log("  Music: " .. config.menu.music.name)
    Log("  Camera Zoom: " .. config.menu.camera.zoom)
    Log("  Button Count: " .. ButtonManager.GetButtonCount())

    Log("═══════════════════════════════════════")
end
