-- ============================================================================
-- Level3Clean.lua
-- REFACTORED - Clean, modular version of Level3
-- ============================================================================
-- This is a refactored version showing proper separation of concerns
-- UI management is delegated to UIManager
-- Level script focuses on level lifecycle and coordination
-- ============================================================================

local PauseMenu = require("PauseMenu")
local UIManager = require("UIManager")

-- ============================================================================
-- LEVEL STATE
-- ============================================================================

local initialized = false
local editorToggleCooldown = 0

-- Grid configuration
local kStartX = -0.6
local kStartY = -0.4
local kSpacingX = 0.1
local kSpacingY = 0.1

-- Audio configuration
local audioConfig = nil

-- ============================================================================
-- LIFECYCLE: OnInit
-- ============================================================================

function OnInit()
    Log("========================================")
    Log("LEVEL 3: Tactical Grid Level (REFACTORED)")
    Log("========================================")

    -- Initialize pause menu
    PauseMenu.Init()

    -- Load and start audio
    InitializeAudio()

    -- Setup camera
    SetCameraPosition(0.0, 0.0, 0.0)
    SetCameraZoom(2.0)
    Log("Camera initialized: pos(0,0,0), zoom=2.0")

    -- Configure editor mode
    if IS_EDITOR_LOAD then
        Log("Level loaded from Editor - Keeping ImGui ENABLED")
    else
        DisableImGui()
        Log("ImGui disabled (Press F1 to toggle)")
    end

    -- Set engine state
    SetEnginePlayState(true)

    -- Load tilemap
    if not LoadTileMapData() then
        Log("ERROR: Failed to load tilemap!")
        return
    end

    -- Load animations
    LoadAnimationConfig("assets/JSON/animations.json")
    LoadPlayerAnimation("Idle_front")

    -- Setup entities
    SetupPlayer()
    SetupEnemies()

    -- Initialize UI system (replaces 500+ lines of UI code!)
    UIManager.Init()

    initialized = true
    Log("========================================")
    Log("Level 3 initialization complete")
    Log("Controls:")
    Log("  - Arrow keys/WASD to move")
    Log("  - Press P/ESC to pause")
    Log("  - Press F1 to toggle editor")
    Log("  - Press 5 to return to main menu")
    Log("========================================")
end

-- ============================================================================
-- LIFECYCLE: OnUpdate
-- ============================================================================

function OnUpdate(dt)
    -- Update audio
    UpdateAudio(dt)

    -- Handle pause menu (always runs)
    PauseMenu.Update(dt)

    -- Skip game logic if paused
    if IsPaused() then
        return
    end

    -- Handle editor toggle
    HandleEditorToggle(dt)

    -- Update UI system (replaces 300+ lines of UI update code!)
    UIManager.Update(dt)
end

-- ============================================================================
-- LIFECYCLE: OnDraw
-- ============================================================================

function OnDraw()
    -- Render pause menu
    PauseMenu.Draw()

    -- Show editor mode indicator
    if IsEditorMode() then
        DrawText("Sans48", "EDITOR MODE", 50, 50, 0.8, 1.0, 0.3, 0.3)
    end
end

-- ============================================================================
-- LIFECYCLE: OnDestroy
-- ============================================================================

function OnDestroy()
    Log("========================================")
    Log("Level 3 cleanup...")
    Log("========================================")

    -- Stop audio
    StopAllSounds()
    Log(" All audio stopped")

    -- Destroy UI system (replaces 100+ lines of UI cleanup code!)
    UIManager.Destroy()

    -- Reset state
    audioConfig = nil
    initialized = false

    Log("Level 3 cleanup complete")
    Log("========================================")
end

-- ============================================================================
-- HELPER FUNCTIONS - Audio
-- ============================================================================

function InitializeAudio()
    Log("Loading audio configuration...")
    audioConfig = LoadJSON("assets/JSON/AudioConfig.json")

    if not audioConfig then
        Log("ERROR: Failed to load audio configuration!")
        return
    end

    Log(" Audio configuration loaded successfully")

    -- Find and play background music
    local bgmSound = nil
    if audioConfig.sounds then
        for i, sound in ipairs(audioConfig.sounds) do
            if sound.name == "igbgm" then
                bgmSound = sound
                break
            end
        end
    end

    if bgmSound then
        Log("Starting background music: " .. bgmSound.name)
        PlaySound(bgmSound.name, bgmSound.loop or false, bgmSound.volume or 1.0)
        Log(" Background music started: " .. bgmSound.filepath)
    else
        Log("WARNING: Background music 'igbgm' not found")
    end
end

-- ============================================================================
-- HELPER FUNCTIONS - Map Loading
-- ============================================================================

function LoadTileMapData()
    Log("Loading tilemap from: assets/JSON/TileMap.json")
    Log("  Grid start: (" .. kStartX .. ", " .. kStartY .. ")")
    Log("  Tile spacing: (" .. kSpacingX .. ", " .. kSpacingY .. ")")

    local success = LoadTileMap(
        "assets/JSON/TileMap.json",
        kStartX,
        kStartY,
        kSpacingX,
        kSpacingY
    )

    if success then
        Log(" Tilemap loaded successfully")
        Log("  Player and map tiles spawned from JSON")
    end

    return success
end

-- ============================================================================
-- HELPER FUNCTIONS - Entity Setup
-- ============================================================================

function SetupPlayer()
    Log("========================================")
    Log("Setting up player...")
    Log("========================================")

    local playerID = FindPlayer()

    if not playerID or playerID == 0 then
        Log("ERROR: Player not found!")
        return false
    end

    Log(" Found Player (Entity ID: " .. playerID .. ")")

    -- Attach player movement script
    local scriptSuccess = AddScriptComponentToEntity(playerID, "assets/scripts/PlayerScript.lua")

    if scriptSuccess then
        Log("   PlayerScript.lua attached successfully")
    else
        Log("  ✗ FAILED to attach PlayerScript.lua")
    end

    return true
end

function SetupEnemies()
    Log("========================================")
    Log("Configuring enemies...")
    Log("========================================")

    local playerID = FindPlayer()
    if not playerID or playerID == 0 then
        Log("ERROR: Cannot configure enemies without player")
        return false
    end

    local enemies = GetAllEnemies() or {}
    local enemyCount = #enemies

    if enemyCount == 0 then
        Log("WARNING: No enemies found in level")
        return true
    end

    Log("Found " .. enemyCount .. " enemies")

    -- Configure each enemy
    for i, enemyID in ipairs(enemies) do
        -- Set target
        local targetSuccess = SetEnemyTarget(enemyID, playerID)

        -- Attach enemy script
        local scriptSuccess = AddScriptComponentToEntity(enemyID, "assets/scripts/EnemyScript.lua")

        if targetSuccess and scriptSuccess then
            Log("   Enemy " .. enemyID .. " configured successfully")
        else
            Log("  ✗ Enemy " .. enemyID .. " configuration failed")
        end
    end

    Log("========================================")
    return true
end

-- ============================================================================
-- HELPER FUNCTIONS - Input Handling
-- ============================================================================

function HandleEditorToggle(dt)
    -- Update cooldown
    if editorToggleCooldown > 0 then
        editorToggleCooldown = editorToggleCooldown - dt
    end

    -- Check for F1 press
    if IsKeyDown("F1") and editorToggleCooldown <= 0 then
        ToggleEditorMode()

        if IsEditorMode() then
            SetEnginePlayState(false)
            Log("EDITOR MODE ON - Camera unlocked")
        else
            SetEnginePlayState(true)
            Log("EDITOR MODE OFF - Camera locked")
        end

        editorToggleCooldown = 0.3
    end
end
