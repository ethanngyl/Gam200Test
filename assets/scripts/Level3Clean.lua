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

-- Turn tracking (for party reset)
local previousTurn = "Player"

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
    Log("  - Characters take turns: Warrior -> Mage -> Rogue -> Enemies")
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

    -- Party system turn management
    local currentTurn = GetCurrentTurn()

    -- Reset party when enemy turn ends and player turn begins
    if previousTurn == "Enemy" and currentTurn == "Player" then
        Log("[Level3Clean] Enemy turn ended - resetting party for new player turn")
        OnEnemyTurnEnded()
    end

    -- Transition to enemy turn when all party members have acted
    if currentTurn == "Player" and IsPartyTurnComplete() then
        Log("[Level3Clean] All party members have acted - transitioning to enemy turn")
        EndPartyTurn()
    end

    -- Update previous turn tracker
    previousTurn = currentTurn

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

    -- TileMap.json now spawns 3 players: P, Q, R at Row24 positions 8, 9, 10
    -- All 3 are created by C++ with full ECS components (AP, Health, Movement, CircleCollider)

    -- Disable C++ grid movement (Lua script will handle movement instead)
    SetGridMovementEnabled(false)
    Log("   C++ grid movement disabled")

    -- Find all player entities from tilemap
    -- TileMap.json Row24: "W1010W01QPR10W010101W" spawns 3 players (Q, P, R)
    local allPlayers = GetAllPlayers()

    if not allPlayers or #allPlayers == 0 then
        Log("ERROR: No players found in tilemap!")
        return false
    end

    Log(" Found " .. #allPlayers .. " player(s) in tilemap:")

    -- Verify we have exactly 3 players
    if #allPlayers < 3 then
        Log("WARNING: Expected 3 players but found " .. #allPlayers)
        Log("Make sure TileMap.json Row24 has P, Q, and R symbols")
        return false
    end

    -- Extract player entity IDs (should be 3)
    local player1 = allPlayers[1]
    local player2 = allPlayers[2]
    local player3 = allPlayers[3]

    Log("  Player 1 (Warrior): Entity " .. player1)
    Log("  Player 2 (Mage):    Entity " .. player2)
    Log("  Player 3 (Rogue):   Entity " .. player3)

    -- Attach scripts to all 3 players
    for i = 1, 3 do
        local playerID = allPlayers[i]
        local scriptSuccess = AddScriptComponentToEntity(playerID, "assets/scripts/PlayerScript.lua")

        if scriptSuccess then
            Log("  [Player " .. i .. "] Script attached successfully")
        else
            Log("  [Player " .. i .. "] ERROR: Failed to attach script")
            SetGridMovementEnabled(true)
            return false
        end

        -- Debug: Check components
        local ap, maxap = GetEntityAP(playerID)
        local hp, maxhp = GetEntityHP(playerID)
        Log("  [Player " .. i .. "] AP: " .. ap .. "/" .. maxap .. ", HP: " .. hp .. "/" .. maxhp)
    end

    -- Initialize party system with all 3 real players
    partyMembers = {player1, player2, player3}

    local partyInitialized = InitializeParty(partyMembers)

    if partyInitialized then
        Log("========================================")
        Log("Party system initialized!")
        Log("  - Character 1: Warrior (Entity " .. player1 .. ") - FUNCTIONAL")
        Log("  - Character 2: Mage (Entity " .. player2 .. ") - PLACEHOLDER")
        Log("  - Character 3: Rogue (Entity " .. player3 .. ") - PLACEHOLDER")
        Log("========================================")
        Log("")
        Log("DEBUG: All 3 should have same entity ID until GetAllPlayers() is implemented")
    else
        Log("FAILED to initialize party")
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

    Log("Party status UI created")
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
