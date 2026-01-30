-- ============================================================================
-- Level3Procedural.lua
-- PROCEDURAL VERSION - Works with 3-character party system
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

-- Turn tracking
local previousTurn = "Player"

-- Grid configuration (MUST match your TileMapLoader)
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
    Log("LEVEL 3: PROCEDURAL MAP VERSION")
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
        SetEnginePlayState(true)
    end

    -- ========================================
    -- PROCEDURAL MAP GENERATION
    -- ========================================
    Log("Generating procedural map...")
    local mapData = LoadProceduralMap(21, 26, "rooms")

    -- Debug output
    Log("DEBUG: mapData = " .. tostring(mapData))
    if mapData then
        Log("DEBUG: mapData.playerX = " .. tostring(mapData.playerX))
        Log("DEBUG: mapData.playerY = " .. tostring(mapData.playerY))
        if mapData.enemies then
            Log("DEBUG: enemies count = " .. #mapData.enemies)
        else
            Log("DEBUG: enemies is nil!")
        end
    else
        Log("ERROR: mapData is NIL!")
        return
    end
    
    -- Validate mapData has required fields
    if not mapData.playerX or not mapData.playerY then
        Log("ERROR: mapData missing playerX/playerY!")
        return
    end
    
    Log("Map generated successfully!")
    
    -- Load animations
    LoadAnimationConfig("assets/JSON/animations.json")
    LoadPlayerAnimation("Idle_front")

    -- ========================================
    -- SPAWN 3 PARTY MEMBERS
    -- ========================================
    if not SetupProceduralParty(mapData) then
        Log("ERROR: Failed to setup party!")
        return
    end

    -- Setup enemies
    SetupProceduralEnemies(mapData)
    
    -- Spawn chests and goal
    SpawnProceduralChestsAndGoal(mapData)

    -- Initialize UI system
    UIManager.Init()

    -- Setup Party UI
    SetupPartyUI()

    -- CRITICAL: Disable grid movement
    Log("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!")
    Log("!!! DISABLING grid movement !!!")
    Log("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!")
    SetGridMovementEnabled(false)
    
    -- IMPORTANT: Set camera to follow first party member
    -- NOTE: SetCameraFollowEntity may not be registered - check if it exists
    if partyMembers[1] then
        -- Try to use the graphics system's follow target if available
        Log("First party member entity: " .. partyMembers[1])
        -- SetCameraFollowEntity is not exposed to Lua - camera will stay at 0,0
    end

    initialized = true
    Log("========================================")
    Log("Level 3 (Procedural) initialization complete")
    Log("Controls:")
    Log("  - WASD to move ACTIVE character")
    Log("  - Characters take turns: Warrior -> Mage -> Rogue -> Enemies")
    Log("  - Press P/ESC to pause")
    Log("========================================")
end

-- ============================================================================
-- HELPER: Grid to World Conversion (local fallback)
-- ============================================================================

local function gridToWorld(gridX, gridY)
    local worldX = kStartX + (gridX * kSpacingX)
    local worldY = kStartY + (gridY * kSpacingY)
    return worldX, worldY
end

-- ============================================================================
-- HELPER: Setup Procedural Party (3 Characters)
-- ============================================================================

function SetupProceduralParty(mapData)
    Log("========================================")
    Log("Setting up 3-character party...")
    Log("========================================")

    -- Disable C++ grid movement
    SetGridMovementEnabled(false)

    -- Safety check
    if not mapData or not mapData.playerX or not mapData.playerY then
        Log("ERROR: Invalid mapData in SetupProceduralParty!")
        return false
    end

    -- Get world positions using TileToWorld (which uses Grid data)
    local p1x, p1y = TileToWorld(mapData.playerX, mapData.playerY)
    local p2x, p2y = TileToWorld(mapData.playerX + 1, mapData.playerY)
    local p3x, p3y = TileToWorld(mapData.playerX + 2, mapData.playerY)
    
    -- Fallback if TileToWorld returns 0,0 (grid not ready)
    if p1x == 0 and p1y == 0 then
        Log("WARNING: TileToWorld returned 0,0 - using fallback gridToWorld")
        p1x, p1y = gridToWorld(mapData.playerX, mapData.playerY)
        p2x, p2y = gridToWorld(mapData.playerX + 1, mapData.playerY)
        p3x, p3y = gridToWorld(mapData.playerX + 2, mapData.playerY)
    end
    
    Log("Spawning 3 party members:")
    Log("  Player 1 (Warrior) at grid (" .. mapData.playerX .. ", " .. mapData.playerY .. ") -> world (" .. p1x .. ", " .. p1y .. ")")
    Log("  Player 2 (Mage) at grid (" .. (mapData.playerX + 1) .. ", " .. mapData.playerY .. ") -> world (" .. p2x .. ", " .. p2y .. ")")
    Log("  Player 3 (Rogue) at grid (" .. (mapData.playerX + 2) .. ", " .. mapData.playerY .. ") -> world (" .. p3x .. ", " .. p3y .. ")")
    
    -- Spawn the 3 players
    local player1 = SpawnPlayerAt(p1x, p1y)
    local player2 = SpawnPlayerAt(p2x, p2y)
    local player3 = SpawnPlayerAt(p3x, p3y)
    
    if not player1 or player1 == 0 then
        Log("ERROR: Failed to spawn Player 1!")
        return false
    end
    
    if not player2 or player2 == 0 then
        Log("ERROR: Failed to spawn Player 2!")
        return false
    end
    
    if not player3 or player3 == 0 then
        Log("ERROR: Failed to spawn Player 3!")
        return false
    end
    
    Log("  Player 1 (Warrior): Entity " .. player1)
    Log("  Player 2 (Mage):    Entity " .. player2)
    Log("  Player 3 (Rogue):   Entity " .. player3)

    -- Move to grid tiles (this registers them in spatial partition)
    local success1 = MoveEntityToTile(player1, mapData.playerX, mapData.playerY)
    local success2 = MoveEntityToTile(player2, mapData.playerX + 1, mapData.playerY)
    local success3 = MoveEntityToTile(player3, mapData.playerX + 2, mapData.playerY)

    -- Attach scripts to all 3 players
    for i, playerID in ipairs({player1, player2, player3}) do
        local scriptSuccess = AddScriptComponentToEntity(playerID, "assets/scripts/PlayerScript.lua")

        if scriptSuccess then
            Log("  [Player " .. i .. "] Script attached successfully")
        else
            Log("  [Player " .. i .. "] ERROR: Failed to attach script")
            return false
        end

        -- Debug: Check components
        local ap, maxap = GetEntityAP(playerID)
        local hp, maxhp = GetEntityHP(playerID)
        Log("  [Player " .. i .. "] AP: " .. ap .. "/" .. maxap .. ", HP: " .. hp .. "/" .. maxhp)
    end

    -- Initialize party system with all 3 real players
    partyMembers = {player1, player2, player3}

    -- Check if InitializeParty exists (from PartyTurnManager.lua)
    if InitializeParty then
        local partyInitialized = InitializeParty(partyMembers)

        if partyInitialized then
            Log("========================================")
            Log("Party system initialized!")
            Log("  - Character 1: Warrior (Entity " .. player1 .. ")")
            Log("  - Character 2: Mage (Entity " .. player2 .. ")")
            Log("  - Character 3: Rogue (Entity " .. player3 .. ")")
            Log("========================================")
        else
            Log("FAILED to initialize party")
            return false
        end
    else
        Log("WARNING: InitializeParty function not found - PartyTurnManager.lua may not be loaded")
    end

    return true
end

-- ============================================================================
-- HELPER: Setup Procedural Enemies
-- ============================================================================

function SetupProceduralEnemies(mapData)
    Log("========================================")
    Log("Spawning procedural enemies...")
    Log("========================================")

    if not mapData.enemies or #mapData.enemies == 0 then
        Log("No enemies to spawn")
        return true
    end

    local playerID = partyMembers[1]  -- Use first party member as target
    if not playerID or playerID == 0 then
        Log("ERROR: Cannot configure enemies without player")
        return false
    end

    Log("Spawning " .. #mapData.enemies .. " enemies:")

    local spawnedEnemies = {}

    for i, enemy in ipairs(mapData.enemies) do
        local ex = enemy.worldX
        local ey = enemy.worldY
        local enemyID = SpawnEnemyAt(ex, ey)
        
        if enemyID and enemyID ~= 0 then
            Log("  Enemy " .. i .. " at grid (" .. enemy.x .. ", " .. enemy.y .. ") -> Entity " .. enemyID)
            
            -- Set target
            SetEnemyTarget(enemyID, playerID)
            
            -- Attach enemy script
            AddScriptComponentToEntity(enemyID, "assets/scripts/EnemyScript.lua")
            
            table.insert(spawnedEnemies, enemyID)
        else
            Log("  Enemy " .. i .. " FAILED to spawn!")
        end
    end

    Log("Spawned " .. #spawnedEnemies .. " enemies successfully")
    Log("========================================")
    return true
end

-- ============================================================================
-- HELPER: Spawn Chests and Goal
-- ============================================================================

function SpawnProceduralChestsAndGoal(mapData)
    Log("========================================")
    Log("Spawning chests and goal...")
    Log("========================================")

    -- Spawn chests
    if mapData.chests and #mapData.chests > 0 then
        Log("Spawning " .. #mapData.chests .. " chests:")
        for i, chest in ipairs(mapData.chests) do
            local cx = chest.worldX  -- From C++!
            local cy = chest.worldY
            local chestID = SpawnChestAt(cx, cy)
            Log("  Chest " .. i .. " at grid (" .. chest.x .. ", " .. chest.y .. ") -> Entity " .. chestID)
        end
    end

    -- Spawn goal
    if mapData.goalX and mapData.goalY then
        local gx = mapData.goalWorldX  -- From C++!
        local gy = mapData.goalWorldY
        local goalID = SpawnGoalAt(gx, gy)
        Log("Goal spawned at grid (" .. mapData.goalX .. ", " .. mapData.goalY .. ") -> Entity " .. goalID)
    end

    Log("========================================")
end

-- ============================================================================
-- HELPER: Setup Party UI
-- ============================================================================

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
    Log("========================================")
end

-- ============================================================================
-- LIFECYCLE: OnUpdate
-- ============================================================================

function OnUpdate(dt)
    -- CRITICAL: Force grid movement to stay disabled
    SetGridMovementEnabled(false)

    -- Update audio
    UpdateAudio(dt)

    -- Handle pause menu
    PauseMenu.Update(dt)

    -- Update party UI
    if partyUI then
        partyUI:OnUpdate(dt)
    end

    -- Skip game logic if paused
    if IsPaused() then
        return
    end

    -- Handle editor toggle
    HandleEditorToggle(dt)
    
    -- Map regeneration disabled - SetNextGameState would work but causes level reload issues
    -- HandleMapRegeneration()

    -- Party system turn management
    local currentTurn = GetCurrentTurn()

    -- Reset party when enemy turn ends
    if previousTurn == "Enemy" and currentTurn == "Player" then
        Log("[Level3Procedural] Enemy turn ended - resetting party")
        OnEnemyTurnEnded()
    end

    -- Transition to enemy turn when all party members have acted
    if currentTurn == "Player" then
        -- Check if IsPartyTurnComplete exists (from PartyTurnManager.lua)
        if IsPartyTurnComplete and IsPartyTurnComplete() then
            Log("[Level3Procedural] Party turn complete - ending turn")
            if EndPartyTurn then
                EndPartyTurn()
            end
        end
    end

    previousTurn = currentTurn

    -- Update UI system
    UIManager.Update(dt)
end

-- ============================================================================
-- LIFECYCLE: OnDraw
-- ============================================================================

function OnDraw()
    PauseMenu.Draw()

    -- Check if IsEditorMode exists
    if IsEditorMode and IsEditorMode() then
        DrawText("Sans48", "EDITOR MODE", 50, 50, 0.8, 1.0, 0.3, 0.3)
    end
    
    -- Show controls
    DrawText("Sans24", "WASD to move, P to pause", 50, 100, 0.8, 0.8, 0.8, 1.0)
end

-- ============================================================================
-- LIFECYCLE: OnDestroy
-- ============================================================================

function OnDestroy()
    Log("========================================")
    Log("Level 3 (Procedural) cleanup...")
    Log("========================================")

    -- Cleanup party UI
    if partyUI then
        partyUI:OnDestroy()
        partyUI = nil
    end

    -- Stop audio
    StopAllSounds()

    -- Destroy UI system
    UIManager.Destroy()

    -- Reset state
    audioConfig = nil
    initialized = false

    Log("Level 3 (Procedural) cleanup complete")
    Log("========================================")
end

-- ============================================================================
-- HELPER FUNCTIONS - Audio
-- ============================================================================

function InitializeAudio()
    audioConfig = LoadJSON("assets/JSON/AudioConfig.json")

    if not audioConfig then
        Log("ERROR: Failed to load audio configuration!")
        return
    end

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
        PlaySound(bgmSound.name, bgmSound.loop or false, bgmSound.volume or 1.0)
    end
end

-- ============================================================================
-- HELPER FUNCTIONS - Input Handling
-- ============================================================================

function HandleEditorToggle(dt)
    if editorToggleCooldown > 0 then
        editorToggleCooldown = editorToggleCooldown - dt
    end

    if IsKeyDown("F1") and editorToggleCooldown <= 0 then
        ToggleEditorMode()

        -- Check if IsEditorMode exists
        if IsEditorMode then
            if IsEditorMode() then
                SetEnginePlayState(false)
            else
                SetEnginePlayState(true)
            end
        end

        editorToggleCooldown = 0.3
    end
end

-- ============================================================================
-- HELPER: Reset Party Turn (called when enemy turn ends)
-- ============================================================================

function OnEnemyTurnEnded()
    -- This is called by PartyTurnManager when enemy turn ends
    -- Reset all party member states for new player turn
    Log("[Level3Procedural] Resetting party for new turn")
    
    -- Party turn manager handles the reset
    -- Camera following is handled by C++ - no Lua function for this
end