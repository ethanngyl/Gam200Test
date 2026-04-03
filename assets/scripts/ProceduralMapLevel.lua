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
local SkillSwapUI = require("SkillSwapUI")
_G.SkillSwapUI = SkillSwapUI

local BOSS_SCRIPT_PATH = "assets/scripts/BossScript.lua"
local BOSS2_SCRIPT_PATH = "assets/scripts/Boss2OrcShaman.lua"

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

local mapSaveCooldown = 0
local MAP_SAVE_COOLDOWN_TIME = 1.0
local USE_SAVED_MAP = false
local SAVED_MAP_PATH = "assets/maps/my_map.map.json"

-- Party members
local partyMembers = {}  -- {warrior, mage, rogue}
-- Turn tracking
local previousTurn = "Player"

-- Grid configuration (MUST match your TileMapLoader)
local kStartX = -0.6
local kStartY = -0.4
local kSpacingX = 0.1
local kSpacingY = 0.1

-- Audio configuration
local audioConfig = nil

-- Level progression (read from LevelProgress.json)
local currentLevel = 1
local totalLevels  = 3

-- ============================================================================
-- GOAL STATE
-- ============================================================================
local goalPosition = nil  -- {gridX, gridY, worldX, worldY}
local goalReached = false
local goalTransitionDelay = 0

-- Boss tracking - portal only spawns after ALL bosses are defeated
local bossEntityIDs = {}     -- Table of boss entity IDs (supports multiple bosses)
local bossDefeated = false
local pendingGoalData = nil  -- Stores goal spawn data until boss dies
local pendingBossSpawn = nil  -- Stores deferred boss spawn payload
local bossArenaGridX = nil
local bossArenaGridY = nil
local bossArenaWorldX = nil
local bossArenaWorldY = nil

-- ============================================================================
-- LIFECYCLE: OnInit
-- ============================================================================

function OnInit()
    -- Read level progression
    local progress = LoadJSON("assets/JSON/LevelProgress.json")
    if progress then
        currentLevel = progress.currentLevel or 1
        totalLevels  = progress.totalLevels  or 3
    end

    -- Reset skill loadout on level 1 so PlayerScript uses its defaults (2 skills each)
    if currentLevel == 1 then
        os.remove("assets/JSON/SkillLoadout.json")
        Log("[ProceduralMapLevel] Skill loadout reset for new game")
    end

    Log("========================================")
    Log("LEVEL " .. currentLevel .. " / " .. totalLevels .. ": PROCEDURAL MAP")
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
    local mapData = nil
    local mapAlgorithm = "rooms_arena"
    if currentLevel >= 3 then
        mapAlgorithm = "open"
    end

    if USE_SAVED_MAP then
         Log("Loading saved map: " .. SAVED_MAP_PATH)
         mapData = LoadSavedMap(SAVED_MAP_PATH)
         if not mapData then
             Log("ERROR: Failed! Falling back to procedural.")
             mapData = LoadProceduralMap(20, 20, mapAlgorithm)
         end
     else
         mapData = LoadProceduralMap(20, 20, mapAlgorithm)
     end

    -- For open arena levels (level 3+), synthesize arena data covering the whole map
    -- so the boss spawn logic works on the open floor
    if currentLevel >= 3 and mapData and not mapData.hasArena then
        local arenaMinX = 3
        local arenaMinY = 3
        local arenaMaxX = 17   -- 20 - 3
        local arenaMaxY = 17
        local centerX = 10
        local centerY = 10

        mapData.hasArena = true
        mapData.arenaX = centerX
        mapData.arenaY = centerY
        mapData.arenaMinX = arenaMinX
        mapData.arenaMinY = arenaMinY
        mapData.arenaMaxX = arenaMaxX
        mapData.arenaMaxY = arenaMaxY

        -- Convert grid center to world coordinates
        local wx = kStartX + centerX * kSpacingX
        local wy = kStartY + centerY * kSpacingY
        mapData.arenaWorldX = wx
        mapData.arenaWorldY = wy

        Log("Level 3: Open arena - synthesized arena bounds (" ..
            arenaMinX .. "," .. arenaMinY .. ") to (" .. arenaMaxX .. "," .. arenaMaxY .. ")")
    end

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

    -- Setup enemies (skip for level 3 - Warlock boss spawns its own)
    if currentLevel < 3 then
        SetupProceduralEnemies(mapData)
    else
        Log("Level 3: Skipping regular enemies - boss spawns its own")
    end

    -- Queue boss spawn; boss appears only when all surviving players enter arena
    QueueBossSpawnIfArena(mapData)

    -- Spawn chests and goal
    SpawnProceduralChestsAndGoal(mapData)

    -- Initialize UI system
    UIManager.Init({ currentLevel = currentLevel })

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

    SetAnimationPrefix(player2, "Mage_")
    SetAnimationPrefix(player3, "Berserker_")
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

-- Enemy type assignment order: ensures at least one of each new type spawns.
-- Index 1 = Knight Commander (yellow), 2 = Knight, 3 = Mage (blue),
-- 4 = Tank (green). Additional enemies cycle through these types.
-- All enemy types now use EnemyGeneric.lua with JSON configs.
local ENEMY_TYPE_CONFIGS = {
    "knight_commander",  -- 1: Knight Commander
    "knight",            -- 2: Knight
    "mage",              -- 3: Mage
    "tank",              -- 4: Tank
}

local ENEMY_TYPE_NAMES = {
    "Knight Commander", "Knight", "Mage", "Tank"
}

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

    -- Reset global enemy death counter for this level
    _G.EnemiesDeadThisLevel = 0
    _G.KnightCommanderIDs = {}
    _G.RallyingCryActive = false
    _G.RallyingCryPending = false

    Log("Spawning " .. #mapData.enemies .. " enemies:")

    local spawnedEnemies = {}

    for i, enemy in ipairs(mapData.enemies) do
        local ex = enemy.worldX
        local ey = enemy.worldY
        local enemyID = SpawnEnemyAt(ex, ey)

        if enemyID and enemyID ~= 0 then
            -- Assign enemy type: cycle through the 4 types via config
            local typeIndex = ((i - 1) % #ENEMY_TYPE_CONFIGS) + 1
            local configType = ENEMY_TYPE_CONFIGS[typeIndex]
            local typeName = ENEMY_TYPE_NAMES[typeIndex]

            Log("  Enemy " .. i .. " [" .. typeName .. "] at grid (" .. enemy.x .. ", " .. enemy.y .. ") -> Entity " .. enemyID)

            -- Attach unified enemy script with config type
            AddScriptComponentToEntity(enemyID, "assets/scripts/EnemyGeneric.lua", configType)

            -- Set target (C++ side)
            SetEnemyTarget(enemyID, playerID)

            Log("  Enemy " .. enemyID .. " " .. typeName .. " (" .. configType .. ") script attached + target set to " .. tostring(playerID))

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
-- HELPER: Spawn Boss in Arena
-- ============================================================================

function SpawnProceduralBoss(mapData)
    if not mapData.hasArena then
        Log("No boss arena in this level")
        return true
    end

    Log("========================================")
    Log("Spawning boss(es) in arena...")
    Log("========================================")

    local bx = mapData.arenaWorldX
    local by = mapData.arenaWorldY

    if not bx or not by then
        Log("ERROR: Arena world coordinates missing!")
        return false
    end

    local playerID = partyMembers[1]
    if not playerID or playerID == 0 then
        Log("ERROR: Cannot configure boss without player")
        return false
    end

    -- Store arena boundaries in shared C++ store so BossScript can access them
    local arenaMinX = mapData.arenaMinX or (mapData.arenaX - 3)
    local arenaMinY = mapData.arenaMinY or (mapData.arenaY - 3)
    local arenaMaxX = mapData.arenaMaxX or (mapData.arenaX + 4)
    local arenaMaxY = mapData.arenaMaxY or (mapData.arenaY + 4)
    SetSharedInt("arenaMinX", arenaMinX)
    SetSharedInt("arenaMinY", arenaMinY)
    SetSharedInt("arenaMaxX", arenaMaxX)
    SetSharedInt("arenaMaxY", arenaMaxY)
    Log("Arena bounds set: (" .. arenaMinX .. "," .. arenaMinY
        .. ") to (" .. arenaMaxX .. "," .. arenaMaxY .. ")")

    -- Determine how many bosses to spawn based on current level
    -- Level 2 only: 2 bosses. Level 1 and 3: 1 boss.
    local bossCount = 1
    if currentLevel == 2 then
        bossCount = 2
    end

    -- Boss spawn offsets within the arena (grid offsets from center)
    -- For 1 boss: spawn at center. For 2 bosses: offset left and right.
    local bossOffsets = {}
    if bossCount == 1 then
        bossOffsets = { {dx = 0, dy = 0} }
    else
        bossOffsets = { {dx = -2, dy = 0}, {dx = 2, dy = 0} }
    end

    bossEntityIDs = {}
    for i = 1, bossCount do
        local spawnX = bx + (bossOffsets[i].dx * kSpacingX)
        local spawnY = by + (bossOffsets[i].dy * kSpacingY)

        local bossID = SpawnEnemyAt(spawnX, spawnY)

        if not bossID or bossID == 0 then
            Log("ERROR: Failed to spawn boss " .. i .. "!")
            return false
        end

        Log("Boss " .. i .. " spawned at grid offset (" .. bossOffsets[i].dx .. ", " .. bossOffsets[i].dy .. ") -> Entity " .. bossID)

        -- Attach boss script based on current level
        local scriptPath = BOSS_SCRIPT_PATH
        if currentLevel >= 3 then
            scriptPath = BOSS2_SCRIPT_PATH
        end
        AddScriptComponentToEntity(bossID, scriptPath)

        -- Set target (C++ side)
        SetEnemyTarget(bossID, playerID)

        table.insert(bossEntityIDs, bossID)
        Log("Boss " .. i .. " (Entity " .. bossID .. ") script attached (" .. scriptPath .. ") + target set to " .. tostring(playerID))
    end

    -- Track arena position so portal spawns here after all bosses die
    bossArenaGridX = mapData.arenaX
    bossArenaGridY = mapData.arenaY
    bossArenaWorldX = bx
    bossArenaWorldY = by

    Log(bossCount .. " boss(es) spawned in arena")
    Log("========================================")
    return true
end

function QueueBossSpawnIfArena(mapData)
    pendingBossSpawn = nil

    if not mapData.hasArena then
        Log("No boss arena in this level")
        return true
    end

    local bx = mapData.arenaWorldX
    local by = mapData.arenaWorldY
    if not bx or not by then
        Log("ERROR: Arena world coordinates missing! Cannot queue boss spawn.")
        return false
    end

    pendingBossSpawn = {
        arenaX = mapData.arenaX,
        arenaY = mapData.arenaY,
        arenaWorldX = bx,
        arenaWorldY = by,
        arenaMinX = mapData.arenaMinX or (mapData.arenaX - 3),
        arenaMinY = mapData.arenaMinY or (mapData.arenaY - 3),
        arenaMaxX = mapData.arenaMaxX or (mapData.arenaX + 4),
        arenaMaxY = mapData.arenaMaxY or (mapData.arenaY + 4),
        hasArena = true
    }

    bossEntityID = nil
    bossDefeated = false
    Log("Boss spawn queued: boss will appear once all surviving players enter arena")
    return true
end

local function AreAllSurvivingPlayersInArena()
    if not pendingBossSpawn then
        return false
    end

    local aliveCount = 0
    local insideCount = 0

    for _, playerID in ipairs(partyMembers) do
        if playerID and playerID ~= 0 then
            local hp = GetEntityHP(playerID)
            if hp and hp > 0 then
                aliveCount = aliveCount + 1
                local px, py = GetEntityGridPosition(playerID)
                if px and py
                   and px >= pendingBossSpawn.arenaMinX and px < pendingBossSpawn.arenaMaxX
                   and py >= pendingBossSpawn.arenaMinY and py < pendingBossSpawn.arenaMaxY then
                    insideCount = insideCount + 1
                end
            end
        end
    end

    return aliveCount > 0 and insideCount == aliveCount
end

local function TrySpawnQueuedBoss()
    if bossEntityID or bossDefeated or not pendingBossSpawn then
        return
    end

    if not AreAllSurvivingPlayersInArena() then
        return
    end

    -- CRITICAL: Never spawn the boss while the enemy turn is active.
    -- Adding a new enemy entity mid-enemy-turn corrupts EnemyTurnManager's
    -- ActiveEnemyIndex state (the new boss appears in GetAllEnemies() at an
    -- unexpected index, conflicting with the already-running sequential turn).
    -- Defer to the next player turn - players cannot move during the enemy turn,
    -- so they will still be in the arena when this check runs again on the first
    -- frame of the player turn.
    if GetCurrentTurn and GetCurrentTurn() == "Enemy" then
        Log("[ProceduralMapLevel] Boss spawn deferred: enemy turn in progress, will spawn next player-turn frame")
        return
    end

    local spawnData = pendingBossSpawn
    local spawned = SpawnProceduralBoss(spawnData)
    if spawned then
        pendingBossSpawn = nil
    end
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

    -- Defer goal/portal spawn until boss is defeated
    if mapData.goalX and mapData.goalY then
        pendingGoalData = {
            goalX = mapData.goalX,
            goalY = mapData.goalY,
            goalWorldX = mapData.goalWorldX,
            goalWorldY = mapData.goalWorldY
        }

        if #bossEntityIDs > 0 then
            Log("Portal will appear after all " .. #bossEntityIDs .. " boss(es) defeated")
        if bossEntityID or pendingBossSpawn then
            Log("Portal will appear after boss is defeated")
        else
            -- No boss on this level, spawn portal immediately
            SpawnPortalNow()
        end
    end

    Log("========================================")
end

-- ============================================================================
-- HELPER: Setup Party UI
-- ============================================================================


-- ============================================================================
-- PORTAL SPAWN (deferred until boss defeated)
-- ============================================================================

function SpawnPortalNow()
    if not pendingGoalData then return end

    local gx = pendingGoalData.goalWorldX
    local gy = pendingGoalData.goalWorldY
    local goalID = SpawnGoalAt(gx, gy)

    goalPosition = {
        gridX = pendingGoalData.goalX,
        gridY = pendingGoalData.goalY,
        worldX = gx,
        worldY = gy,
        entityID = goalID
    }

    Log("Portal spawned at grid (" .. pendingGoalData.goalX .. ", " .. pendingGoalData.goalY .. ") -> Entity " .. goalID)
    pendingGoalData = nil
end

function CheckBossDefeated()
    if bossDefeated or #bossEntityIDs == 0 then return end

    -- Check if ALL bosses are dead
    for _, bossID in ipairs(bossEntityIDs) do
        if IsEntityValid(bossID) then
            return  -- At least one boss is still alive
        end
    end

    -- All bosses defeated
    bossDefeated = true
    Log("ALL BOSSES DEFEATED - Spawning portal in boss room!")

    -- Spawn portal at boss arena center
    local goalID = SpawnGoalAt(bossArenaWorldX, bossArenaWorldY)
    goalPosition = {
        gridX = bossArenaGridX,
        gridY = bossArenaGridY,
        worldX = bossArenaWorldX,
        worldY = bossArenaWorldY,
        entityID = goalID
    }
    pendingGoalData = nil
    Log("Portal spawned in boss room at grid (" .. bossArenaGridX .. ", " .. bossArenaGridY .. ") -> Entity " .. goalID)
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

            Log("GOAL REACHED! Level " .. currentLevel .. " / " .. totalLevels)

            if currentLevel >= totalLevels then
                -- Final level complete - go to end screen
                Log("All levels complete - going to end screen")
                if SetNextGameState then
                    SetNextGameState("WIN_SCREEN")
                end
            else
                -- Levels 1-2: show skill swap UI then load next level
                -- Level 1 fills slot 3, level 2 fills slot 4
                local skillSlot = currentLevel + 2
                Log("Opening skill swap for slot " .. skillSlot)

                SkillSwapUI.Show(function()
                    -- Advance level progress
                    local nextLevel = currentLevel + 1
                    local f = io.open("assets/JSON/LevelProgress.json", "w")
                    if f then
                        f:write("{\n")
                        f:write("  \"currentLevel\": " .. nextLevel .. ",\n")
                        f:write("  \"totalLevels\": " .. totalLevels .. "\n")
                        f:write("}\n")
                        f:close()
                        Log("[LevelProgress] Advanced to level " .. nextLevel .. " / " .. totalLevels)
                    end

                    Log("Skill swap complete - Loading next procedural map...")
                    if SetNextGameState then
                        SetNextGameState("LEVEL_3")
                    end
                end, skillSlot)
            end
        end
    end
end

-- ============================================================================
-- LIFECYCLE: OnUpdate
-- ============================================================================

function OnUpdate(dt)
    -- Process deferred deaths FIRST (Dark Omens etc.) - must run when Lua stack is clear
    if ProcessDeferredDeaths then
        ProcessDeferredDeaths()
    end

    -- CRITICAL: Force grid movement to stay disabled
    SetGridMovementEnabled(false)

    -- Handle pause menu
    PauseMenu.Update(dt)

    -- Update skill swap UI (runs while paused, handles its own input)
    SkillSwapUI.Update(dt)

    -- Skip game logic if paused (SkillSwapUI pauses the game while active)
    if IsPaused() then
        return
    end

    -- Update audio
    UpdateAudio(dt)

    -- Handle editor toggle
    HandleEditorToggle(dt)

    if IsKeyDown("9") and mapSaveCooldown <= 0 then
        Log("SAVING MAP (9)...")
        local success, filepath = SaveCurrentMap()
        if success then Log("Saved to: " .. tostring(filepath))
        else Log("ERROR: Save failed!") end
        mapSaveCooldown = MAP_SAVE_COOLDOWN_TIME
    end

     -- ========================================
    -- TEST: Skill Swap UI (press 0)
    -- ========================================
    if IsKeyDown("0") and not SkillSwapUI.IsActive() then
        SkillSwapUI.Show(function()
            Log("Skill swap done! Would transition to next level here.")
        end)
    end

    -- ========================================
    -- DEBUG: Jump to specific level (press 8)
    -- Cycles through levels: 1 -> 2 -> 3 -> 1
    -- ========================================
    if IsKeyDown("8") then
        local nextLvl = (currentLevel % totalLevels) + 1
        Log("DEBUG: Jumping to level " .. nextLvl)
        local f = io.open("assets/JSON/LevelProgress.json", "w")
        if f then
            f:write("{\n")
            f:write("  \"currentLevel\": " .. nextLvl .. ",\n")
            f:write("  \"totalLevels\": " .. totalLevels .. "\n")
            f:write("}\n")
            f:close()
        end
        SetNextGameState("LEVEL_3")
        return
    end

    -- ========================================
    -- CHEAT: Skip to lose screen (press 6)
    -- ========================================
    if IsKeyDown("6") then
        SetNextGameState("LOSE_SCREEN")
        return
    end

    -- ========================================
    -- CHEAT: Skip to win screen (press 7)
    -- ========================================
    if IsKeyDown("7") then
        SetNextGameState("WIN_SCREEN")
        return
    end

    -- ========================================
    -- CHEAT: Kill all enemies (press K)
    -- ========================================
    if IsKeyDown("K") and not goalReached then
        local enemies = GetAllEnemies()
        if enemies and #enemies > 0 then
            for _, eid in ipairs(enemies) do
                SetEntityHP(eid, 0, 0)
                DestroyEntity(eid)
            end
            Log("[CHEAT] Killed all " .. #enemies .. " enemies")
        end
    end
    
    -- Map regeneration disabled - SetNextGameState would work but causes level reload issues
    -- HandleMapRegeneration()

    -- ========================================
    -- CHECK BOSS DEFEATED -> SPAWN PORTAL
    -- ========================================
    TrySpawnQueuedBoss()
    CheckBossDefeated()

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

    -- NOTE: Do NOT call ResetPartyTurn/OnEnemyTurnEnded here!
    -- EnemyTurnManager already calls ResetPartyTurn when the last enemy finishes.
    -- A second call on the next frame would reset hasActed and undo stun-skip (Groundshatter bug).

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

    -- Must capture previousTurn BEFORE updating managers, so turn transitions
    -- are correctly detected on the next frame
    previousTurn = currentTurn

    -- Update party turn manager (handles turn transition cooldowns and input timing)
    if UpdatePartyTurnManager then
        UpdatePartyTurnManager(dt)
    end

    -- Update enemy turn manager (for sequential enemy turns with delays)
    if UpdateEnemyTurnManager then
        if currentTurn == "Enemy" then
            Log("[Level3Procedural] Calling UpdateEnemyTurnManager(dt=" .. string.format("%.3f", dt) .. ")")
        end
        UpdateEnemyTurnManager(dt)
    end

    -- Keep enemy indicators in sync (always visible, red arrows above enemies)
    if SyncEnemyIndicators then SyncEnemyIndicators() end
    if UpdateAllEnemyIndicators then UpdateAllEnemyIndicators() end

    -- Update UI system
    UIManager.Update(dt)
end

-- ============================================================================
-- LIFECYCLE: OnDraw
-- ============================================================================

function OnDraw()
    PauseMenu.Draw()

    -- Render skill swap UI overlay (manages its own pause/active state)
    SkillSwapUI.Draw()

    -- When paused, only draw the pause/overlay UIs — skip all game UI
    if IsPaused() then
        return
    end

    -- Render UI components (including scroll animation text)
    UIManager.Draw()

    -- Level indicator (top-right corner)
    local fbW, fbH = GetFramebufferSize()
    if fbW and fbW > 0 then
        local scaleRef = fbW / 1920
        DrawText("Jersey20Regular", "Level " .. currentLevel .. " / " .. totalLevels,
            fbW - 220 * scaleRef, 30 * scaleRef, 0.6 * scaleRef, 0.9, 0.9, 0.7)
    end

    -- Check if IsEditorMode exists
    if IsEditorMode and IsEditorMode() then
        DrawText("Sans48", "EDITOR MODE", 50, 50, 0.8, 1.0, 0.3, 0.3)
    end
end

-- ============================================================================
-- LIFECYCLE: OnDestroy
-- ============================================================================

function OnDestroy()
    Log("========================================")
    Log("Level 3 (Procedural) cleanup...")
    Log("========================================")

    -- Stop audio
    StopMusic(0.5)

    -- Destroy player active-character indicator
    if DestroyActiveCharIndicator then
        DestroyActiveCharIndicator()
        Log("  Destroyed active character indicator")
    end

    -- Destroy all enemy indicators
    if DestroyAllEnemyIndicators then
        DestroyAllEnemyIndicators()
        Log("  Destroyed all enemy indicators")
    end

    -- Force-end enemy turn state so it doesn't carry over
    EnemyTurnActive = false
    ActiveEnemyIndex = 0

    -- Reset party state
    PartyMembers = {}
    ActiveCharacterIndex = 1
    PartyTurnComplete = false

    -- Destroy UI system
    UIManager.Destroy()

    -- Reset state
    audioConfig = nil
    initialized = false
    goalReached = false
    goalPosition = nil
    bossEntityID = nil
    bossDefeated = false
    pendingGoalData = nil
    pendingBossSpawn = nil

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
        PlayMusic(bgmSound.name, 0.8, bgmSound.loop or false)
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
