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
-- LOAD MODULES
-- ============================================================================

local PauseMenu = require("PauseMenu")

-- ============================================================================
-- LEVEL STATE VARIABLES
-- ============================================================================

local initialized = false
local editorToggleCooldown = 0  -- Cooldown for F1 editor toggle
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
-- Adjusted for zoom 0.6: offsets scaled to match viewport size
local screenOffsetX = -0.82  -- Bottom left corner (adjusted for zoom)
local screenOffsetY = -0.42  -- Bottom left corner (adjusted for zoom)
local lastKnownAP = 0  -- Track AP changes

-- Chest UI indicators
local chestIndicatorsEmpty = {}
local chestIndicatorsFilled = {}
local chestUIOffsetX = 0.3   -- Adjusted for zoom 0.6
local chestUIOffsetY = -0.42  -- Adjusted for zoom 0.6
local lastKnownChests = 0
local totalChestsRequired = 0

-- NEW: Attack AP Indicator sprites (separate bar near sword icon)
local atkIndicatorsEmpty = {}     -- NEW
local atkIndicatorsFilled = {}    -- NEW
local maxAttackAP = 3             -- NEW (will be overridden by component) 
local atkOffsetX = -0.82          -- NEW (same X as AP; tweak as needed)
local atkOffsetY = -0.30          -- NEW (slightly above AP row)
local lastKnownAttackAP = 0       -- NEW

-- Debug frame counter
local debugFrameCounter = 0

-- Performance optimization: cache camera position to reduce redundant updates
local lastCamX = nil  -- nil = first frame, needs initialization
local lastCamY = nil
local lastCamZ = nil
local cameraMoveThreshold = 0.01  -- Only update UI when camera moves > this amount

-- ============================================================================
-- LEVEL LIFECYCLE: OnInit
-- ============================================================================

function OnInit()
    Log("========================================")
    Log("LEVEL 3: Tactical Grid Level")
    Log("========================================")

    -- Set camera to default position
    SetCameraPosition(0.0, 0.0, 0.0)
    SetCameraZoom(2.0)  -- Zoomed in closer to player (0.5-0.7 recommended for gameplay)
    Log("Camera initialized: pos(0,0,0), zoom=0.6 (closer view)")

    -- Disable ImGui by default (can be toggled with F1)
    DisableImGui()
    Log("ImGui disabled by default - press F1 to toggle editor")

    -- Set engine to playing state (required for physics/movement)
    SetEnginePlayState(false)  -- Start paused for initialization

    -- Load the tilemap from JSON
    -- Parameters: LoadTileMap(jsonPath, startX, startY, spacingX, spacingY)
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

    -- Load animation configuration BEFORE loading player animation
    LoadAnimationConfig("assets/JSON/animations.json")
    Log("✓ Animation config loaded from assets/JSON/animations.json")

    -- Load player animation (after player is spawned by TileMapLoader)
    LoadPlayerAnimation("Idle_front")
    Log("✓ Player animation 'Idle_front' loaded")

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
            "assets/UI/AP_Empty.png",
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
            "assets/UI/AP_Crystal.png",
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

    -- ========================================================================
    -- NEW: CREATE ATTACK AP INDICATOR UI
    -- ========================================================================
    Log("Creating Attack AP indicators...")

    local currentAtkAP, maxPlayerAtkAP = GetPlayerAttackAP()
    if maxPlayerAtkAP ~= nil and maxPlayerAtkAP > 0 then
        maxAttackAP = maxPlayerAtkAP
    end
    lastKnownAttackAP = currentAtkAP or 0

    -- Use same camera position we already read earlier
    -- (camX, camY, camZ are still in scope here)

    -- Empty attack AP icons (background)
    for i = 1, maxAttackAP do
        local xPos = camX + atkOffsetX + ((i - 1) * indicatorSpacing)
        local yPos = camY + atkOffsetY

        local entityID = SpawnSprite(
            "assets/UI/AP_Empty.png",   -- TODO: swap to your ATTACK AP empty sprite
            xPos, yPos,
            indicatorSize, indicatorSize,
            0
        )

        if entityID > 0 then
            atkIndicatorsEmpty[i] = entityID
            Log("  ✓ Empty Attack AP " .. i .. " (ID: " .. entityID .. ")")
        else
            atkIndicatorsEmpty[i] = 0
            Log("  ✗ FAILED to create empty Attack AP " .. i)
        end
    end

    -- Filled attack AP icons (foreground)
    for i = 1, lastKnownAttackAP do
        local xPos = camX + atkOffsetX + ((i - 1) * indicatorSpacing)
        local yPos = camY + atkOffsetY

        local entityID = SpawnSprite(
            "assets/UI/AP_Crystal.png", -- TODO: swap to your ATTACK AP filled sprite
            xPos, yPos,
            indicatorSize, indicatorSize,
            0
        )

        if entityID > 0 then
            atkIndicatorsFilled[i] = entityID
            Log("  ✓ Filled Attack AP " .. i .. " (ID: " .. entityID .. ")")
        else
            atkIndicatorsFilled[i] = 0
            Log("  ✗ FAILED to create filled Attack AP " .. i)
        end
    end

    Log("Empty crystals: " .. #apIndicatorsEmpty .. "/" .. maxAP)
    Log("Filled crystals: " .. #apIndicatorsFilled .. "/" .. maxAP)
    Log("========================================")

    initialized = true
    Log("========================================")
    Log("Level 3 initialization complete")
    Log("Controls:")
    Log("  - Click tiles or use arrow keys to move")
    Log("  - Press P to pause/resume game")
    Log("  - Press F1 to toggle editor")
    Log("  - Press 5 to return to main menu")
    Log("  - AP indicators shown at top of screen")
    Log("========================================")

    -- ========================================================================
    -- CREATE CHEST PROGRESS UI
    -- ========================================================================
    Log("Creating Chest indicators...")
    
    local collected, required = GetChestProgress()
    totalChestsRequired = required
    Log("Chest progress: " .. collected .. "/" .. required)

    -- Create empty chest indicators
    for i = 1, totalChestsRequired do
        local xPos = camX + chestUIOffsetX + ((i - 1) * indicatorSpacing)
        local yPos = camY + chestUIOffsetY

        local entityID = SpawnSprite(
            "assets/UI/AP_Empty.png",
            xPos, yPos,
            indicatorSize, indicatorSize,
            0
        )

        if entityID > 0 then
            chestIndicatorsEmpty[i] = entityID
            Log("  ✓ Empty chest indicator " .. i)
        else
            chestIndicatorsEmpty[i] = 0
        end
    end

    lastKnownChests = 0

    initialized = true
    Log("Level 3 initialization complete!")
end


-- ============================================================================
-- LEVEL LIFECYCLE: OnUpdate
-- ============================================================================

function OnUpdate(dt)
    -- Note: Player controller and pathfinding updates happen in C++ systems
    -- Camera follow is set in C++ (SetFollowTarget on player entity)
    -- Turn system ticks in C++

    -- Toggle ImGui editor with F1
    if IsKeyDown("F1") then
        if editorToggleCooldown <= 0 then
            local newState = ToggleEditor()
            if newState then
                Log("EDITOR ON")
            else
                Log("EDITOR OFF")
            end
            editorToggleCooldown = 0.3
        end
    end

    if editorToggleCooldown > 0 then
        editorToggleCooldown = editorToggleCooldown - dt
    end

    -- Toggle pause with P key
    if IsKeyPressed("P") then
        TogglePause()
        if IsPaused() then
            Log("GAME PAUSED")
        else
            Log("GAME RESUMED")
        end
    end

    -- Check for return to main menu (KEY_5)
    if IsKeyDown("5") then
        Log("KEY_5 pressed - returning to main menu")
        SetNextGameState("mainMenu")
    end

    -- ========================================================================
    -- PERFORMANCE: Cache camera position ONCE per frame
    -- ========================================================================
    local camX, camY, camZ = GetCameraPosition()

    -- Detect if camera moved OR if this is first frame (needs init)
    local needsUpdate = (lastCamX == nil) or
                       (math.abs(camX - lastCamX) > cameraMoveThreshold) or
                       (math.abs(camY - lastCamY) > cameraMoveThreshold) or
                       (math.abs(camZ - lastCamZ) > cameraMoveThreshold)

    if needsUpdate then
        lastCamX = camX
        lastCamY = camY
        lastCamZ = camZ
    end

    -- ========================================================================
    -- UPDATE AP INDICATORS (TWO-LAYER SYSTEM)
    -- ========================================================================
    if #apIndicatorsEmpty > 0 then

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

        -- OPTIMIZATION: Only update sprite positions when camera moves
        if needsUpdate then
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
                        "assets/UI/AP_Crystal.png",
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

    -- ========================================================================
    -- UPDATE ATTACK AP INDICATORS (OPTIMIZED)
    -- ========================================================================
    if #atkIndicatorsEmpty > 0 then
        local currentAtkAP, maxPlayerAtkAP = GetPlayerAttackAP()

        -- OPTIMIZATION: Only update sprite positions when camera moves
        if needsUpdate then
            -- Reposition empty icons
            for i = 1, #atkIndicatorsEmpty do
                local entityID = atkIndicatorsEmpty[i]
                if entityID and entityID > 0 then
                    local xPos = camX + atkOffsetX + ((i - 1) * indicatorSpacing)
                    local yPos = camY + atkOffsetY
                    SetSpritePosition(entityID, xPos, yPos)
                end
            end

            -- Reposition filled icons
            for i = 1, #atkIndicatorsFilled do
                local entityID = atkIndicatorsFilled[i]
                if entityID and entityID > 0 then
                    local xPos = camX + atkOffsetX + ((i - 1) * indicatorSpacing)
                    local yPos = camY + atkOffsetY
                    SetSpritePosition(entityID, xPos, yPos)
                end
            end
        end

        -- React to value change
        if currentAtkAP ~= lastKnownAttackAP then
            Log("[ATK AP] changed from " .. lastKnownAttackAP .. " to " .. currentAtkAP)

            if currentAtkAP < lastKnownAttackAP then
                -- Lose points: destroy filled icons from the end
                for i = lastKnownAttackAP, currentAtkAP + 1, -1 do
                    if atkIndicatorsFilled[i] ~= nil then
                        DestroyEntity(atkIndicatorsFilled[i])
                        atkIndicatorsFilled[i] = nil
                        Log("  Destroyed Attack AP crystal #" .. i)
                    end
                end
            elseif currentAtkAP > lastKnownAttackAP then
                -- Gain points: create new filled icons
                for i = lastKnownAttackAP + 1, currentAtkAP do
                    local xPos = camX + atkOffsetX + ((i - 1) * indicatorSpacing)
                    local yPos = camY + atkOffsetY

                    local entityID = SpawnSprite(
                        "assets/UI/AP_Crystal.png", -- TODO: attack filled sprite
                        xPos, yPos,
                        indicatorSize, indicatorSize,
                        0
                    )

                    if entityID > 0 then
                        atkIndicatorsFilled[i] = entityID
                        Log("  Created Attack AP crystal #" .. i .. " (ID: " .. entityID .. ")")
                    else
                        Log("  ✗ FAILED to create Attack AP crystal #" .. i)
                    end
                end
            end

            lastKnownAttackAP = currentAtkAP
        end
    end

    -- ========================================================================
    -- UPDATE CHEST PROGRESS UI (OPTIMIZED)
    -- ========================================================================
    if #chestIndicatorsEmpty > 0 then
        local collected, required = GetChestProgress()

        -- OPTIMIZATION: Only update sprite positions when camera moves
        if needsUpdate then
            -- Update positions of empty chest indicators
            for i = 1, #chestIndicatorsEmpty do
                local entityID = chestIndicatorsEmpty[i]
                if entityID and entityID > 0 then
                    local xPos = camX + chestUIOffsetX + ((i - 1) * indicatorSpacing)
                    local yPos = camY + chestUIOffsetY
                    SetSpritePosition(entityID, xPos, yPos)
                end
            end

            -- Update positions of filled chest indicators
            for i = 1, #chestIndicatorsFilled do
                local entityID = chestIndicatorsFilled[i]
                if entityID ~= nil and entityID > 0 then
                    local xPos = camX + chestUIOffsetX + ((i - 1) * indicatorSpacing)
                    local yPos = camY + chestUIOffsetY
                    SetSpritePosition(entityID, xPos, yPos)
                end
            end
        end

        -- Handle chest collection
        if collected ~= lastKnownChests then
            Log("[CHEST] Collected " .. collected .. "/" .. required .. " chests!")

            if collected > lastKnownChests then
                -- Create filled chest indicators
                for i = lastKnownChests + 1, collected do
                    local xPos = camX + chestUIOffsetX + ((i - 1) * indicatorSpacing)
                    local yPos = camY + chestUIOffsetY

                    local entityID = SpawnSprite(
                        "assets/UI/AP_Crystal.png",
                        xPos, yPos,
                        indicatorSize, indicatorSize,
                        0
                    )

                    if entityID > 0 then
                        chestIndicatorsFilled[i] = entityID
                        Log("  ✓ Created filled chest indicator " .. i)
                    else
                        chestIndicatorsFilled[i] = 0
                    end
                end
            end

            lastKnownChests = collected

            if collected >= required then
                Log("=== ALL CHESTS COLLECTED! ===")
                Log("Go to the GOAL to complete the level!")
            end
        end
    end
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

    -- Destroy chest indicators (empty)
    for i = 1, #chestIndicatorsEmpty do
        if chestIndicatorsEmpty[i] and chestIndicatorsEmpty[i] > 0 then
            DestroyEntity(chestIndicatorsEmpty[i])
        end
    end

    -- Destroy chest indicators (filled)
    for i = 1, #chestIndicatorsFilled do
        if chestIndicatorsFilled[i] ~= nil and chestIndicatorsFilled[i] > 0 then
            DestroyEntity(chestIndicatorsFilled[i])
        end
    end

     -- Destroy Attack AP indicators (empty)
    for i = 1, #atkIndicatorsEmpty do
        local id = atkIndicatorsEmpty[i]
        if id and id > 0 then
            DestroyEntity(id)
        end
    end

    -- Destroy Attack AP indicators (filled)
    for i = 1, #atkIndicatorsFilled do
        local id = atkIndicatorsFilled[i]
        if id and id > 0 then
            DestroyEntity(id)
        end
    end

    apIndicatorsEmpty = {}
    apIndicatorsFilled = {}
    chestIndicatorsEmpty = {}
    chestIndicatorsFilled = {}
    atkIndicatorsEmpty = {}
    atkIndicatorsFilled = {}
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
