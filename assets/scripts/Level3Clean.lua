-- ============================================================================
-- Level3Clean.lua
-- REFACTORED - Clean, modular version of Level3 with 3-character party system
-- ============================================================================
-- This is a refactored version showing proper separation of concerns
-- UI management is delegated to UIManager
-- Level script focuses on level lifecycle and coordination
-- Party system: 3 characters act sequentially before enemy turn
-- ============================================================================

local PauseMenu = require("PauseMenu")
local UIManager = require("UIManager")

-- Load Party Turn Manager (REQUIRED for party system)
dofile("assets/scripts/PartyTurnManager.lua")

-- ============================================================================
-- LEVEL STATE
-- ============================================================================

local initialized = false
local editorToggleCooldown = 0

-- Party members
local partyMembers = {}  -- {warrior, mage, rogue}
local partyUI = nil

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
        -- Don't set playing state in editor mode - let LevelLoader handle it
        -- This allows the editor to control play/stop state
    else
        DisableImGui()
        Log("ImGui disabled (Press F1 to toggle)")
        -- Set engine state only when NOT in editor mode
        SetEnginePlayState(true)
    end

    -- Load tilemap
    if not LoadTileMapData() then
        Log("ERROR: Failed to load tilemap!")
        return
    end

    -- Load animations
    LoadAnimationConfig("assets/JSON/animations.json")
    LoadPlayerAnimation("Idle_front")

    -- Setup entities
    SetupParty()  -- Changed from SetupPlayer() to SetupParty()
    SetupEnemies()

    -- Initialize UI system (replaces 500+ lines of UI code!)
    UIManager.Init()

    -- Setup Party UI (shows all 3 characters)
    SetupPartyUI()

    initialized = true
    Log("========================================")
    Log("Level 3 initialization complete")
    Log("Controls:")
    Log("  - WASD to move ACTIVE character")
    Log("  - Characters take turns: Warrior → Mage → Rogue → Enemies")
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

    -- Update party UI (shows HP/AP for all characters)
    if partyUI then
        partyUI:OnUpdate(dt)
    end

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

    -- Cleanup party UI
    if partyUI then
        partyUI:OnDestroy()
        partyUI = nil
        Log(" Party UI destroyed")
    end

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

function SetupParty()
    Log("========================================")
    Log("Setting up 3-character party...")
    Log("========================================")

    -- Find the original player entity from tilemap
    local originalPlayer = FindPlayer()

    if not originalPlayer or originalPlayer == 0 then
        Log("ERROR: Player not found!")
        return false
    end

    Log(" Found original player (Entity ID: " .. originalPlayer .. ")")

    -- Get the player's starting position
    local startX, startY = GetPlayerGridPosition()

    if not startX or not startY then
        Log("ERROR: Could not get player position!")
        return false
    end

    Log("  Starting position: (" .. startX .. ", " .. startY .. ")")

    -- For now, use the original player as Character 1 (Warrior)
    -- In a full implementation, you'd spawn 3 separate entities
    -- but for testing, we'll start with just enhancing the single player

    -- Disable C++ grid movement (Lua script will handle movement instead)
    SetGridMovementEnabled(false)
    Log("   C++ grid movement disabled")

    -- Attach player movement script to the player
    local scriptSuccess = AddScriptComponentToEntity(originalPlayer, "assets/scripts/PlayerScript.lua")

    if scriptSuccess then
        Log("   PlayerScript.lua attached successfully")
        Log("  Character 1 (Warrior) movement handled by Lua script")
    else
        Log("  ✗ FAILED to attach PlayerScript.lua")
        -- Re-enable C++ movement as fallback
        SetGridMovementEnabled(true)
        Log("  C++ grid movement re-enabled as fallback")
        return false
    end

    -- TODO: Spawn additional party members (Mage and Rogue)
    -- For now, initialize party with just the warrior
    -- This allows the infrastructure to work with 1 character until full implementation

    -- Initialize party system with the warrior
    -- Note: InitializeParty expects 3 entities, so for now we'll use the same entity
    -- In full implementation, replace with actual character entities
    partyMembers = {originalPlayer, originalPlayer, originalPlayer}

    local partyInitialized = InitializeParty(partyMembers)

    if partyInitialized then
        Log("✓ Party system initialized (currently with 1 character)")
        Log("  - Character 1: Warrior (Entity " .. originalPlayer .. ")")
        Log("  - Character 2: [TODO - Not yet spawned]")
        Log("  - Character 3: [TODO - Not yet spawned]")
        Log("")
        Log("NOTE: Party system infrastructure ready, but only 1 character active")
        Log("Full 3-character support coming in next update!")
    else
        Log("✗ FAILED to initialize party")
        return false
    end

    return true
end

function SetupPartyUI()
    Log("========================================")
    Log("Setting up Party UI...")
    Log("========================================")

    -- Load PartyStatusUI
    dofile("assets/scripts/UI/PartyStatusUI.lua")

    -- Create UI instance
    partyUI = PartyStatusUI:new(0)
    partyUI:OnInit()

    Log("✓ Party status UI created")
    Log("  UI will display HP/AP for all party members")
end

-- Legacy function name for compatibility
function SetupPlayer()
    return SetupParty()
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
            Log("  Enemy " .. enemyID .. " configuration failed")
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
