--[[
===============================================================================
 File:          ProceduralMapLevel.lua
 Authors:       Josh Ong
 Co-Authors:    -
 Date:          2-5-2026
 Contribution:  100%
 ------------------------------------------------------------------------------

 PROCEDURAL MAP LEVEL - Level Script with Generated Maps

 Brief:
    Level script for procedurally generated tactical maps. Integrates with the
    C++ ProceduralMapLoader to generate dungeon layouts at runtime. Supports
    a 3-character party system with turn-based combat, sequential enemy turns,
    and goal-based level completion.

 Features:
    - Procedural map generation via C++ bridge (LoadProceduralMap)
    - 3-character party spawning at validated positions
    - Party turn management (Warrior -> Mage -> Rogue -> Enemies)
    - Sequential enemy turn system with visual delays
    - Goal detection and level transition
    - Pause menu integration
    - UI system with health, AP, and turn indicators

 Party System:
    - Player 1 (Warrior): First to act each turn
    - Player 2 (Mage): Second to act
    - Player 3 (Rogue): Third to act
    - After all party members act, enemy turn begins

 Usage:
    -- This script is loaded as a level script
    -- Configure in your level loading system to use this file

    -- Map generation is called automatically in OnInit:
    local mapData = LoadProceduralMap(21, 26, "rooms")

    -- Party setup uses pre-validated spawn positions from C++:
    SetupProceduralParty(mapData)


 Copyright (C) 2026 DigiPen Institute of Technology.
 Reproduction or disclosure of this file or its contents
 without the prior written consent of DigiPen Institute of
 Technology is prohibited.
===============================================================================
]]--
local PauseMenu = require("PauseMenu")
local UIManager = require("UIManager")

-- Export UIManager globally so entity scripts can access it via C++ bridge
-- (Entity scripts run in separate Lua states and need global access)
_G.UIManager = UIManager

-- Load Party Turn Manager (REQUIRED for party system)
dofile("assets/scripts/PartyTurnManager.lua")

-- Load Enemy Turn Manager (REQUIRED for sequential enemy turns)
dofile("assets/scripts/EnemyTurnManager.lua")

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
-- GOAL STATE
-- ============================================================================
local goalPosition = nil  -- {gridX, gridY, worldX, worldY}
local goalReached = false
local goalTransitionDelay = 0
--local GOAL_TRANSITION_TIME = 0.5  -- Seconds before transitioning to menu

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
    local mapData = LoadProceduralMap(35, 35, "rooms_arena")

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

    -- ========================================
    -- SPAWN 3 PARTY MEMBERS
    -- ========================================
    if not SetupProceduralParty(mapData) then
        Log("ERROR: Failed to setup party!")
        return
    end

    -- Load animations
    LoadAnimationConfig("assets/JSON/animations.json")
    LoadPlayerAnimation("Idle_front")

    -- Setup enemies
    SetupProceduralEnemies(mapData)
    
    -- Spawn chests and goal
    SpawnProceduralChestsAndGoal(mapData)

    -- Initialize UI system
    UIManager.Init()

    -- Setup Party UI
    SetupPartyUI()

    -- CRITICAL: Disable C++ grid movement (Lua handles movement via PartyTurnManager)
    SetGridMovementEnabled(false)

    -- Remove WASD movement from all players (grid movement handled by Lua scripts)
    for _, pid in ipairs(partyMembers) do
        RemoveMovementComponent(pid)
        Log("Removed Movement component from player " .. pid)
    end

    -- Set camera to follow first party member
    if partyMembers[1] then
        SetCameraFollowTarget(partyMembers[1])
        Log("Camera following player " .. partyMembers[1])
    end

    -- Initialize turn system (Player phase, not busy)
    InitializeTurnSystem()
    Log("Turn system initialized: Player phase")

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

    if not mapData then
        Log("ERROR: mapData is nil!")
        return false
    end

    -- Disable C++ grid movement
    SetGridMovementEnabled(false)

    -- ========================================
    -- USE THE PRE-VALIDATED PARTY SPAWNS FROM C++
    -- ========================================
    if not mapData.partySpawns or #mapData.partySpawns < 3 then
        Log("ERROR: mapData.partySpawns missing or incomplete!")
        Log("Make sure you applied the C++ fix to Lua_LoadProceduralMap")
        return false
    end

    Log("Using C++ validated party spawns:")
    for i, spawn in ipairs(mapData.partySpawns) do
        Log("  Spawn " .. i .. ": grid(" .. spawn.x .. ", " .. spawn.y .. 
            ") -> world(" .. spawn.worldX .. ", " .. spawn.worldY .. ")")
    end

    -- Spawn the 3 players at validated positions
    local player1 = SpawnPlayerAt(mapData.partySpawns[1].worldX, mapData.partySpawns[1].worldY)
    local player2 = SpawnPlayerAt(mapData.partySpawns[2].worldX, mapData.partySpawns[2].worldY)
    local player3 = SpawnPlayerAt(mapData.partySpawns[3].worldX, mapData.partySpawns[3].worldY)

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

    -- Move to grid tiles (registers in spatial partition)
    MoveEntityToTile(player1, mapData.partySpawns[1].x, mapData.partySpawns[1].y)
    MoveEntityToTile(player2, mapData.partySpawns[2].x, mapData.partySpawns[2].y)
    MoveEntityToTile(player3, mapData.partySpawns[3].x, mapData.partySpawns[3].y)

    -- Attach scripts to all 3 players
    for i, playerID in ipairs({player1, player2, player3}) do
        local scriptSuccess = AddScriptComponentToEntity(playerID, "assets/scripts/PlayerScript.lua")

        if scriptSuccess then
            Log("  [Player " .. i .. "] Script attached successfully")
        else
            Log("  [Player " .. i .. "] ERROR: Failed to attach script")
        end

        -- Debug: Check components
        local ap, maxap = GetEntityAP(playerID)
        local hp, maxhp = GetEntityHP(playerID)
        Log("  [Player " .. i .. "] AP: " .. ap .. "/" .. maxap .. ", HP: " .. hp .. "/" .. maxhp)
    end

    -- Initialize party system
    partyMembers = {player1, player2, player3}

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
            
            -- Attach enemy script FIRST (so OnInit runs immediately with this setup)
            AddScriptComponentToEntity(enemyID, "assets/scripts/EnemyScript.lua")

            -- Set target (C++ side)
            SetEnemyTarget(enemyID, playerID)

            Log("  Enemy " .. enemyID .. " script attached + target set to " .. tostring(playerID))

            
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
    --if mapData.chests and #mapData.chests > 0 then
      --  Log("Spawning " .. #mapData.chests .. " chests:")
        --for i, chest in ipairs(mapData.chests) do
          --  local cx = chest.worldX  -- From C++!
            --local cy = chest.worldY
            --local chestID = SpawnChestAt(cx, cy)
            --Log("  Chest " .. i .. " at grid (" .. chest.x .. ", " .. chest.y .. ") -> Entity " .. chestID)
        --end
    --end

    -- Spawn goal
    if mapData.goalX and mapData.goalY then
        local gx = mapData.goalWorldX  -- From C++!
        local gy = mapData.goalWorldY
        local goalID = SpawnGoalAt(gx, gy)

        -- Store goal position for collision checking
        goalPosition = {
            gridX = mapData.goalX,
            gridY = mapData.goalY,
            worldX = gx,
            worldY = gy,
            entityID = goalID
        }

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
-- CHECK GOAL REACHED (NEW)
-- ============================================================================

function CheckGoalReached()
    if not goalPosition then
        return false
    end
    
    -- Check each party member's position against the goal
    for i, playerID in ipairs(partyMembers) do
        if playerID and playerID ~= 0 then
            -- Get player's current world position
            local playerGridX, playerGridY = GetEntityGridPosition(playerID)
            
            if playerGridX and playerGridY then
                -- Convert to grid position
                --local playerGridX, playerGridY = worldToGrid(playerWorldX, playerWorldY)
                
                -- Check if player is on the goal tile
                if playerGridX == goalPosition.gridX and playerGridY == goalPosition.gridY then
                    Log("========================================")
                    Log("GOAL REACHED!")
                    Log("Player " .. i .. " (Entity " .. playerID .. ") reached the goal!")
                    Log("========================================")
                    return true
                end
            end
        end
    end
    
    return false
end

-- ============================================================================
-- HANDLE GOAL TRANSITION (NEW)
-- ============================================================================

function HandleGoalTransition(dt)
    if not goalReached then
        -- Check if any player reached the goal
        if CheckGoalReached() then
            goalReached = true
            
            -- Play victory sound if available
            if PlaySound then
                pcall(function()
                    PlaySound("victory", false, 1.0)
                end)
            end
            
            Log("GOAL REACHED - Transitioning to Main Menu immediately...")
            
            -- Transition to main menu immediately
            if SetNextGameState then
                SetNextGameState("LEVEL_END")
            else
                Log("ERROR: No game state transition function available!")
            end
        end
    end
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

    -- ========================================
    -- CHECK FOR GOAL INTERACTION (NEW)
    -- ========================================
    HandleGoalTransition(dt)
    
    -- If goal is reached, skip normal game logic
    if goalReached then
        return
    end

    -- Debug: Log positions every few frames
--if not goalReached and goalPosition then
  --  for i, playerID in ipairs(partyMembers) do
    --    if playerID and playerID ~= 0 then
      --      local gx, gy = GetEntityGridPosition(playerID)
        --    if gx and gy then
          --      print("Player " .. i .. " at grid (" .. gx .. ", " .. gy .. ") | Goal at (" .. goalPosition.gridX .. ", " .. goalPosition.gridY .. ")")
            --end
        --end
    --end
--end

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

    -- Update enemy turn manager (for sequential enemy turns with delays)
    if UpdateEnemyTurnManager then
        if currentTurn == "Enemy" then
            Log("[Level3Procedural] Calling UpdateEnemyTurnManager(dt=" .. string.format("%.3f", dt) .. ")")
        end
        UpdateEnemyTurnManager(dt)
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

    -- Render UI components (including scroll animation text)
    UIManager.Draw()

    -- Check if IsEditorMode exists
    if IsEditorMode and IsEditorMode() then
        DrawText("Sans48", "EDITOR MODE", 50, 50, 0.8, 1.0, 0.3, 0.3)
    end
    
    -- Show controls
    --[[
    DrawText("Playfair48", "WASD to move, P to pause", 50, 100, 0.8, 0.8, 0.8, 1.0)
    ]]
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
    -- Call PartyTurnManager's ResetPartyTurn to reset party state
    Log("[Level3Procedural] Enemy turn ended - calling ResetPartyTurn()")

    -- Call the global ResetPartyTurn function from PartyTurnManager
    if ResetPartyTurn then
        ResetPartyTurn()
    else
        Log("[Level3Procedural] ERROR: ResetPartyTurn not found!")
    end
end