-- ============================================================================
-- Level3Clean.lua
-- REFACTORED - Clean, modular version of Level3 with 3-character party system
-- ============================================================================
-- This is a refactored version showing proper separation of concerns
-- UI management is delegated to UIManager
-- Level script focuses on level lifecycle and coordination
-- Party system: 3 characters act sequentially before enemy turn
-- ============================================================================

-- CRITICAL: Verify script is loading
print("============================================================")
print("========== Level3Clean.lua SCRIPT LOADING STARTED ==========")
print("============================================================")

local PauseMenu = require("PauseMenu")
local UIManager = require("UIManager")
local PopupManager = require("UI/PopupManager")

-- Export UIManager globally so entity scripts can access it
-- (Entity scripts run in separate Lua states and need global access)
_G.UIManager = UIManager
_G.PopupManager = PopupManager

-- Load Party Turn Manager (REQUIRED for party system)
print("[Level3Clean] Loading PartyTurnManager.lua...")
dofile("assets/scripts/PartyTurnManager.lua")
print("[Level3Clean] PartyTurnManager.lua loaded successfully")

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
    print("============================================================")
    print("========== Level3Clean.lua OnInit() CALLED ==========")
    print("============================================================")

    Log("========================================")
    Log("LEVEL 3: Tactical Grid Level (REFACTORED)")
    Log("========================================")

    print("[Level3Clean] After Log() calls - Log system working")

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

    -- Initialize popup system for one-time animations
    PopupManager.Init()

    -- Setup Party UI (shows all 3 characters)
    SetupPartyUI()

    -- CRITICAL: Re-disable grid movement AFTER all initialization
    -- Some systems (GameStateManager) may re-enable it during setup
    Log("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!")
    Log("!!! RE-DISABLING grid movement after init !!!")
    Log("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!")
    SetGridMovementEnabled(false)
    Log("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!")
    Log("!!! Grid movement RE-DISABLED !!!")
    Log("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!")

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
    -- CRITICAL: Force grid movement to stay disabled every frame
    -- Something keeps re-enabling it, so we force it off continuously
    SetGridMovementEnabled(false)

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
    if currentTurn == "Player" then
        local partyComplete = IsPartyTurnComplete()
        if partyComplete then
            Log("[Level3Clean] !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!")
            Log("[Level3Clean] !!! IsPartyTurnComplete() returned TRUE !!!")
            Log("[Level3Clean] !!! Calling EndPartyTurn() !!!")
            Log("[Level3Clean] !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!")
            EndPartyTurn()
        end
    end

    -- Update previous turn tracker
    previousTurn = currentTurn

    -- Update party turn manager (handles turn transition cooldown)
    UpdatePartyTurnManager(dt)

    -- Update UI system (replaces 300+ lines of UI update code!)
    UIManager.Update(dt)

    -- Update popup animations (damage numbers, status effects, etc.)
    PopupManager.Update(dt)
end

-- ============================================================================
-- LIFECYCLE: OnDraw
-- ============================================================================

function OnDraw()
    -- Render pause menu
    PauseMenu.Draw()

    -- Render popup animations (damage numbers, status effects, etc.)
    PopupManager.Draw()

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

    -- Clear all popup animations
    PopupManager.Clear()

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
    print("============================================================")
    print("========== SetupParty() CALLED ==========")
    print("============================================================")

    Log("========================================")
    Log("Setting up 3-character party...")
    Log("========================================")

    print("[SetupParty] After Log() calls")

    -- Disable C++ grid movement (Lua script will handle movement instead)
    print("[SetupParty] Step 1: Calling SetGridMovementEnabled(false)...")
    SetGridMovementEnabled(false)
    print("[SetupParty] Step 1: DONE")

    -- Find all player entities from tilemap
    print("[SetupParty] Step 2: Calling GetAllPlayers()...")
    local allPlayers = GetAllPlayers()
    print("[SetupParty] Step 2: GetAllPlayers() returned")

    if not allPlayers then
        print("[SetupParty] ERROR: GetAllPlayers() returned nil!")
        return false
    end

    print("[SetupParty] Step 3: Found " .. #allPlayers .. " players")

    if #allPlayers == 0 then
        print("[SetupParty] ERROR: No players in table!")
        return false
    end

    -- Verify we have exactly 3 players
    if #allPlayers < 3 then
        print("[SetupParty] ERROR: Expected 3 players but found " .. #allPlayers)
        return false
    end

    -- Extract player entity IDs (should be 3)
    local player1 = allPlayers[1]
    local player2 = allPlayers[2]
    local player3 = allPlayers[3]

    print("[SetupParty] Step 4: Player entities:")
    print("  Player 1: " .. tostring(player1))
    print("  Player 2: " .. tostring(player2))
    print("  Player 3: " .. tostring(player3))

    -- Attach scripts to all 3 players
    print("[SetupParty] Step 5: Attaching PlayerScript.lua to all 3 players...")
    for i = 1, 3 do
        local playerID = allPlayers[i]
        print("[SetupParty]   Attaching to Player " .. i .. " (Entity " .. playerID .. ")...")
        local scriptSuccess = AddScriptComponentToEntity(playerID, "assets/scripts/PlayerScript.lua")

        if scriptSuccess then
            print("[SetupParty]   SUCCESS: Player " .. i .. " script attached")
        else
            print("[SetupParty]   ERROR: Failed to attach script to Player " .. i)
            SetGridMovementEnabled(true)
            return false
        end

        -- Debug: Check components
        print("[SetupParty]   Checking AP/HP for Player " .. i .. "...")
        local ap, maxap = GetEntityAP(playerID)
        local hp, maxhp = GetEntityHP(playerID)
        print("[SetupParty]   Player " .. i .. " - AP: " .. tostring(ap) .. "/" .. tostring(maxap) .. ", HP: " .. tostring(hp) .. "/" .. tostring(maxhp))
    end

    print("[SetupParty] Step 6: All scripts attached successfully")

    -- Initialize party system with all 3 real players
    partyMembers = {player1, player2, player3}

    print("[SetupParty] Step 7: Calling InitializeParty()...")
    local partyInitialized = InitializeParty(partyMembers)
    print("[SetupParty] Step 7: InitializeParty() returned: " .. tostring(partyInitialized))

    if partyInitialized then
        print("[SetupParty] Step 8: Party system initialized successfully!")
        print("[SetupParty]   Character 1: Warrior (Entity " .. player1 .. ")")
        print("[SetupParty]   Character 2: Mage (Entity " .. player2 .. ")")
        print("[SetupParty]   Character 3: Rogue (Entity " .. player3 .. ")")

        -- DEBUG: Verify GetPartyMembers() returns correct data
        print("[SetupParty] Step 9: Verifying party members...")
        local verifyMembers = GetPartyMembers()
        print("[SetupParty]   GetPartyMembers() returned " .. #verifyMembers .. " members")
        for i = 1, #verifyMembers do
            print("[SetupParty]     Member " .. i .. ": Entity " .. verifyMembers[i])
        end
    else
        print("[SetupParty] ERROR: InitializeParty() FAILED!")
        return false
    end

    print("[SetupParty] Step 10: SetupParty() COMPLETED SUCCESSFULLY")
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
    print("============================================================")
    print("========== SetupEnemies() CALLED ==========")
    print("============================================================")

    print("[SetupEnemies] Step 1: Finding player for enemy targeting...")
    local playerID = FindPlayer()

    if not playerID or playerID == 0 then
        print("[SetupEnemies] ERROR: Cannot configure enemies without player")
        return false
    end

    print("[SetupEnemies] Found player: Entity " .. playerID)

    print("[SetupEnemies] Step 2: Calling GetAllEnemies()...")
    local enemies = GetAllEnemies() or {}
    local enemyCount = #enemies

    print("[SetupEnemies] GetAllEnemies() returned " .. enemyCount .. " enemies")

    if enemyCount == 0 then
        print("[SetupEnemies] WARNING: No enemies found in level")
        return true
    end

    print("[SetupEnemies] Step 3: Configuring " .. enemyCount .. " enemies...")

    -- Configure each enemy
    for i, enemyID in ipairs(enemies) do
        print("[SetupEnemies]   === Configuring Enemy " .. i .. " (Entity " .. enemyID .. ") ===")

        -- Set target
        print("[SetupEnemies]     Calling SetEnemyTarget(" .. enemyID .. ", " .. playerID .. ")...")
        local targetSuccess = SetEnemyTarget(enemyID, playerID)
        print("[SetupEnemies]     SetEnemyTarget result: " .. tostring(targetSuccess))

        -- Attach enemy script
        print("[SetupEnemies]     Calling AddScriptComponentToEntity(" .. enemyID .. ", 'EnemyScript.lua')...")
        local scriptSuccess = AddScriptComponentToEntity(enemyID, "assets/scripts/EnemyScript.lua")
        print("[SetupEnemies]     AddScriptComponentToEntity result: " .. tostring(scriptSuccess))

        if targetSuccess and scriptSuccess then
            print("[SetupEnemies]   ✓ Enemy " .. enemyID .. " configured successfully")
        else
            print("[SetupEnemies]   ✗ Enemy " .. enemyID .. " configuration FAILED")
            print("[SetupEnemies]     targetSuccess: " .. tostring(targetSuccess))
            print("[SetupEnemies]     scriptSuccess: " .. tostring(scriptSuccess))
        end

        -- Check enemy's AP and position
        local enemyX, enemyY = GetEntityGridPosition(enemyID)
        local currentAP, maxAP = GetEntityAP(enemyID)
        print("[SetupEnemies]     Enemy position: (" .. tostring(enemyX) .. ", " .. tostring(enemyY) .. ")")
        print("[SetupEnemies]     Enemy AP: " .. tostring(currentAP) .. "/" .. tostring(maxAP))
    end

    print("[SetupEnemies] Step 4: All enemies configured")
    print("============================================================")
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
