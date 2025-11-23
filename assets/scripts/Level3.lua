-- ============================================================================
-- Level3.lua
-- Turn-based tactical game level with grid-based movement and pathfinding
-- ============================================================================
-- Converted from C++ level3.cpp to Lua for hot-reloadable level design
--
-- Features:
-- - Loads level layout from TileMap.json
-- - Player-controlled character with grid movement
-- - Disabled ImGui/UI for debugging AMD GPU flickering issue
-- - Minimal entities: player + map tiles only
-- ============================================================================

-- ============================================================================
-- LEVEL STATE VARIABLES
-- ============================================================================

local initialized = false
local kStartX = -0.6  -- Grid start position X
local kStartY = -0.4  -- Grid start position Y
local kSpacingX = 0.1 -- Tile spacing X
local kSpacingY = 0.1 -- Tile spacing Y

-- AP Indicator sprites (camera-relative UI) - Two-layer system
local apIndicatorsEmpty = {}   -- Background layer: always visible (5 empty crystals)
local apIndicatorsFilled = {}  -- Foreground layer: destroyed/recreated based on AP
local maxAP = 5
local indicatorSize = 0.08
local indicatorSpacing = 0.1
local screenOffsetX = -0.7  -- Bottom left corner of screen
local screenOffsetY = -0.7  -- Bottom left corner of screen
local lastKnownAP = 0  -- Track AP changes

-- Debug frame counter
local debugFrameCounter = 0

-- ============================================================================
-- LEVEL LIFECYCLE: OnInit
-- ============================================================================

function OnInit()
    Log("========================================")
    Log("LEVEL 3: Tactical Grid Level")
    Log("========================================")

    -- Set camera to default position
    SetCameraPosition(0.0, 0.0, 0.0)
    SetCameraZoom(1.0)
    Log("Camera initialized: pos(0,0,0), zoom=1.0")

    -- Disable ImGui for debugging (isolating player + map for AMD GPU flickering)
    DisableImGui()
    Log("ImGui DISABLED for debugging - only player + map active")

    -- Set engine to playing state (required for physics/movement)
    SetEnginePlayState(false)  -- Start paused for initialization

    -- Load the tilemap from JSON
    -- Parameters: LoadTileMap(jsonPath, startX, startY, spacingX, spacingY)
    Log("Loading tilemap from: assets/scripts/JSON/TileMap.json")
    Log("  Grid start: (" .. kStartX .. ", " .. kStartY .. ")")
    Log("  Tile spacing: (" .. kSpacingX .. ", " .. kSpacingY .. ")")

    local success = LoadTileMap(
        "assets/scripts/JSON/TileMap.json",
        kStartX,
        kStartY,
        kSpacingX,
        kSpacingY
    )

    if not success then
        Log("ERROR: Failed to load tilemap!")
        return
    end

    Log("✓ Tilemap loaded successfully")
    Log("  Player and map tiles spawned from JSON")
    Log("  All enemies disabled for debugging")

    -- Note: Player entity is automatically spawned by the TileMapLoader
    -- PlayerController is configured in C++ (SetPlayerEntity, SetGridMovementEnabled)
    -- Turn phase is set to Player in C++

    -- ========================================================================
    -- CONFIGURE ENEMIES 
    -- ========================================================================
    Log("========================================")
    Log("Configuring Enemy AI...")
    Log("========================================")
    
    -- Find the player entity
    local playerID = FindPlayer()
    
    if not playerID or playerID == 0 then
        Log("ERROR: Player not found! Cannot configure enemies.")
        Log("  Make sure TileMap.json has a 'P' tile for player spawn")
        return
    end
    
    Log("✓ Found Player (Entity ID: " .. playerID .. ")")
    
    -- Find all enemy entities
    local enemies = GetAllEnemies()
    
    if not enemies then
        Log("WARNING: GetAllEnemies() returned nil")
        enemies = {}
    end
    
    local enemyCount = 0
    for _ in pairs(enemies) do
        enemyCount = enemyCount + 1
    end
    
    if enemyCount == 0 then
        Log("WARNING: No enemies found in level")
        Log("  Check if TileMap.json has 'E' tiles for enemy spawns")
        Log("  Check if TileMapLoader.cpp has enemy spawning enabled")
    else
        Log("Found " .. enemyCount .. " enemies")
        
        -- Configure each enemy to target the player
        for i, enemyID in ipairs(enemies) do
            local success = SetEnemyTarget(enemyID, playerID)
            
            if success then
                Log("  ✓ Enemy " .. enemyID .. " configured to chase Player " .. playerID)
            else
                Log("  ✗ FAILED to configure Enemy " .. enemyID)
            end
        end
    end
    
    Log("========================================")

    -- ========================================================================
    -- CREATE AP INDICATOR UI (CAMERA-RELATIVE) - TWO-LAYER SYSTEM
    -- ========================================================================
    Log("========================================")
    Log("Creating AP indicators (two-layer system)...")
    Log("========================================")

    -- Get initial camera position
    local camX, camY, camZ = GetCameraPosition()
    Log("Initial camera position: (" .. camX .. ", " .. camY .. ", " .. camZ .. ")")
    Log("Screen offsets: X=" .. screenOffsetX .. ", Y=" .. screenOffsetY)
    Log("Indicator size: " .. indicatorSize .. ", spacing: " .. indicatorSpacing)

    -- LAYER 1: Create 5 EMPTY AP crystals (background - always visible)
    Log("Creating EMPTY crystal background layer...")
    for i = 1, maxAP do
        local xPos = camX + screenOffsetX + ((i - 1) * indicatorSpacing)
        local yPos = camY + screenOffsetY

        local entityID = SpawnSprite(
            "assets/AP Empty.png",
            xPos,
            yPos,
            indicatorSize,
            indicatorSize,
            0  -- Ground layer
        )

        if entityID > 0 then
            apIndicatorsEmpty[i] = entityID
            Log("  ✓ Empty crystal " .. i .. " (ID: " .. entityID .. ")")
        else
            Log("  ✗ FAILED to create empty crystal " .. i)
        end
    end

    -- LAYER 2: Create 5 FILLED AP crystals (foreground - will be destroyed/recreated)
    Log("Creating FILLED crystal foreground layer...")
    for i = 1, maxAP do
        local xPos = camX + screenOffsetX + ((i - 1) * indicatorSpacing)
        local yPos = camY + screenOffsetY

        local entityID = SpawnSprite(
            "assets/AP Crystal.png",
            xPos,
            yPos,
            indicatorSize,
            indicatorSize,
            0  -- Same layer, rendered on top due to creation order
        )

        if entityID > 0 then
            apIndicatorsFilled[i] = entityID
            Log("  ✓ Filled crystal " .. i .. " (ID: " .. entityID .. ")")
        else
            Log("  ✗ FAILED to create filled crystal " .. i)
        end
    end

    -- Set initial AP tracking
    lastKnownAP = maxAP

    Log("Empty crystals: " .. #apIndicatorsEmpty .. "/" .. maxAP)
    Log("Filled crystals: " .. #apIndicatorsFilled .. "/" .. maxAP)
    Log("========================================")

    initialized = true
    Log("========================================")
    Log("Level 3 initialization complete")
    Log("Controls:")
    Log("  - Click tiles or use arrow keys to move")
    Log("  - Press 5 to return to main menu")
    Log("  - AP indicators shown at top of screen")
    Log("========================================")
end


-- ============================================================================
-- LEVEL LIFECYCLE: OnUpdate
-- ============================================================================

function OnUpdate(dt)
    -- Note: Player controller and pathfinding updates happen in C++ systems
    -- Camera follow is set in C++ (SetFollowTarget on player entity)
    -- Turn system ticks in C++

    -- Check for return to main menu (KEY_5)
    if IsKeyDown("5") then
        Log("KEY_5 pressed - returning to main menu")
        SetNextGameState("mainMenu")
    end

    -- ========================================================================
    -- UPDATE AP INDICATORS (TWO-LAYER SYSTEM)
    -- ========================================================================
    if #apIndicatorsEmpty > 0 then
        -- Get current camera position
        local camX, camY, camZ = GetCameraPosition()

        -- Get player's current AP
        local currentAP, maxPlayerAP = GetPlayerAP()

        -- Debug logging (once per second @ 60fps)
        debugFrameCounter = debugFrameCounter + 1
        local shouldDebug = (debugFrameCounter % 60 == 0)

        if shouldDebug then
            Log("[AP DEBUG] Frame " .. debugFrameCounter .. " - Camera: (" .. camX .. ", " .. camY .. ", " .. camZ .. ")")
            Log("[AP DEBUG] Player AP: " .. currentAP .. "/" .. maxPlayerAP .. " (last known: " .. lastKnownAP .. ")")
            Log("[AP DEBUG] Filled crystals active: " .. #apIndicatorsFilled)
        end

        -- Update positions of EMPTY crystals (background layer - always visible)
        for i = 1, #apIndicatorsEmpty do
            local entityID = apIndicatorsEmpty[i]
            local xPos = camX + screenOffsetX + ((i - 1) * indicatorSpacing)
            local yPos = camY + screenOffsetY
            SetSpritePosition(entityID, xPos, yPos)
        end

        -- Update positions of FILLED crystals (foreground layer)
        for i = 1, #apIndicatorsFilled do
            local entityID = apIndicatorsFilled[i]
            if entityID ~= nil then
                local xPos = camX + screenOffsetX + ((i - 1) * indicatorSpacing)
                local yPos = camY + screenOffsetY
                SetSpritePosition(entityID, xPos, yPos)
            end
        end

        
        -- Handle AP changes: destroy/create filled crystals
        if currentAP ~= lastKnownAP then
            Log("[AP CHANGE] AP changed from " .. lastKnownAP .. " to " .. currentAP)

            if currentAP < lastKnownAP then
                -- AP DECREASED: Destroy filled crystals from the end
                for i = lastKnownAP, currentAP + 1, -1 do
                    if apIndicatorsFilled[i] ~= nil then
                        Log("  Destroying filled crystal #" .. i .. " (ID: " .. apIndicatorsFilled[i] .. ")")
                        DestroyEntity(apIndicatorsFilled[i])
                        apIndicatorsFilled[i] = nil
                    end
                end

                 --  FIX: Verify destruction
                local remainingCount = 0
                for i = 1, maxAP do
                    if apIndicatorsFilled[i] ~= nil then
                        remainingCount = remainingCount + 1
                    end
                end
                Log("  After deletion: " .. remainingCount .. " crystals remaining (expected: " .. currentAP .. ")")
                

            elseif currentAP > lastKnownAP then
                -- AP INCREASED: Create new filled crystals
                for i = lastKnownAP + 1, currentAP do
                    local xPos = camX + screenOffsetX + ((i - 1) * indicatorSpacing)
                    local yPos = camY + screenOffsetY

                    local entityID = SpawnSprite(
                        "assets/AP Crystal.png",
                        xPos,
                        yPos,
                        indicatorSize,
                        indicatorSize,
                        0  -- Ground layer
                    )

                    if entityID > 0 then
                        apIndicatorsFilled[i] = entityID
                        Log("  Created filled crystal #" .. i .. " (ID: " .. entityID .. ")")
                    else
                        Log("  ✗ FAILED to create filled crystal #" .. i)
                    end
                end
            end

            -- Update tracking
            lastKnownAP = currentAP
        end
    end
    -- Engine is set to playing mode in C++ during update loop
    -- Graphics system handles camera follow automatically
end

-- ============================================================================
-- LEVEL LIFECYCLE: OnDraw
-- ============================================================================

function OnDraw()
    -- All rendering handled by C++ graphics system
    -- ImGui is disabled for debugging
    -- PauseSystem drawing is disabled
end

-- ============================================================================
-- LEVEL LIFECYCLE: OnDestroy
-- ============================================================================

function OnDestroy()
    Log("========================================")
    Log("Level 3 cleanup...")
    Log("========================================")

    -- Destroy AP indicator entities (both layers)
    for i = 1, #apIndicatorsEmpty do
        local entityID = apIndicatorsEmpty[i]
        if entityID > 0 then
            DestroyEntity(entityID)
            Log("  Destroyed empty crystal " .. i .. " (ID: " .. entityID .. ")")
        end
    end

    for i = 1, #apIndicatorsFilled do
        local entityID = apIndicatorsFilled[i]
        if entityID ~= nil and entityID > 0 then
            DestroyEntity(entityID)
            Log("  Destroyed filled crystal " .. i .. " (ID: " .. entityID .. ")")
        end
    end

    apIndicatorsEmpty = {}
    apIndicatorsFilled = {}
    Log("AP indicators cleaned up (both layers)")

    -- Note: Entity cleanup, camera reset, and player controller reset
    -- happen in C++ level3_Free() function
    -- This is called by GameStateManager when transitioning levels

    initialized = false

    Log("Level 3 cleanup complete")
end

-- ============================================================================
-- DEBUG HELPERS
-- ============================================================================

function PrintLevelInfo()
    Log("═══════════════════════════════════════")
    Log("Level 3 Configuration:")
    Log("  Initialized: " .. tostring(initialized))
    Log("  Grid Start: (" .. kStartX .. ", " .. kStartY .. ")")
    Log("  Grid Spacing: (" .. kSpacingX .. ", " .. kSpacingY .. ")")
    Log("  ImGui: DISABLED (debugging)")
    Log("  Entities: Player + Map only")
    Log("  Enemies: DISABLED (debugging)")
    Log("═══════════════════════════════════════")
end
