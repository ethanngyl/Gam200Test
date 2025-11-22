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

-- AP Indicator sprites (camera-relative UI)
local apIndicators = {}
local maxAP = 5
local indicatorSize = 0.08
local indicatorSpacing = 0.1
local screenOffsetX = -0.2  -- Offset from camera center (horizontal)
local screenOffsetY = 0.3   -- Offset from camera center (vertical) - TESTING: reduced from 0.85

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
    -- CREATE AP INDICATOR UI (CAMERA-RELATIVE)
    -- ========================================================================
    Log("========================================")
    Log("Creating AP indicators (camera-relative)...")
    Log("========================================")

    -- Get initial camera position
    local camX, camY, camZ = GetCameraPosition()
    Log("Initial camera position: (" .. camX .. ", " .. camY .. ", " .. camZ .. ")")
    Log("Screen offsets: X=" .. screenOffsetX .. ", Y=" .. screenOffsetY)
    Log("Indicator size: " .. indicatorSize .. ", spacing: " .. indicatorSpacing)

    -- Create 5 AP indicator sprites
    for i = 1, maxAP do
        local xPos = camX + screenOffsetX + ((i - 1) * indicatorSpacing)
        local yPos = camY + screenOffsetY

        Log("AP Indicator " .. i .. " - Calculated position: (" .. xPos .. ", " .. yPos .. ")")
        Log("  Calling SpawnSprite with size: " .. indicatorSize .. " x " .. indicatorSize)

        -- SpawnSprite(texture, x, y, width, height, layer)
        -- Using AP Crystal.png for visual AP indicators
        -- Layer 100 = RenderLayers::UI (renders on top)
        local entityID = SpawnSprite(
            "assets/AP Crystal.png",
            xPos,
            yPos,
            indicatorSize,
            indicatorSize,
            100  -- UI layer
        )

        if entityID > 0 then
            -- Store entity ID
            apIndicators[i] = entityID

            -- Set initial color: green = AP available
            SetSpriteColor(entityID, 0.2, 1.0, 0.2, 1.0)

            Log("  ✓ Created AP indicator " .. i .. " (ID: " .. entityID .. ") - Layer: 100 (UI)")
        else
            Log("  ✗ FAILED to create AP indicator " .. i)
        end
    end

    Log("AP indicators created: " .. #apIndicators .. "/" .. maxAP)
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
    -- UPDATE AP INDICATORS (CAMERA-RELATIVE POSITIONING + COLOR UPDATES)
    -- ========================================================================
    if #apIndicators > 0 then
        -- Get current camera position
        local camX, camY, camZ = GetCameraPosition()

        -- Get player's current AP
        local currentAP, maxPlayerAP = GetPlayerAP()

        -- Debug logging (once per second @ 60fps)
        debugFrameCounter = debugFrameCounter + 1
        local shouldDebug = (debugFrameCounter % 60 == 0)

        if shouldDebug then
            Log("[AP DEBUG] Frame " .. debugFrameCounter .. " - Camera: (" .. camX .. ", " .. camY .. ", " .. camZ .. ")")
            Log("[AP DEBUG] Player AP: " .. currentAP .. "/" .. maxPlayerAP)
            Log("[AP DEBUG] Active indicators: " .. #apIndicators)
        end

        -- Update each indicator's position and color
        for i = 1, #apIndicators do
            local entityID = apIndicators[i]

            -- Calculate screen-relative position (follows camera)
            local xPos = camX + screenOffsetX + ((i - 1) * indicatorSpacing)
            local yPos = camY + screenOffsetY

            -- Update position to follow camera
            SetSpritePosition(entityID, xPos, yPos)

            -- Debug first indicator position
            if shouldDebug and i == 1 then
                Log("[AP DEBUG] Indicator #1 (ID " .. entityID .. ") updated to: (" .. xPos .. ", " .. yPos .. ")")
            end

            -- Update color based on current AP
            -- If this indicator index <= currentAP, show as available (green)
            -- Otherwise show as used (dark red)
            if i <= currentAP then
                -- Available AP: bright green
                SetSpriteColor(entityID, 0.2, 1.0, 0.2, 1.0)
                if shouldDebug and i == 1 then
                    Log("[AP DEBUG] Indicator #" .. i .. " - GREEN (available)")
                end
            else
                -- Used AP: dark red
                SetSpriteColor(entityID, 0.3, 0.1, 0.1, 0.7)
                if shouldDebug and i == 1 then
                    Log("[AP DEBUG] Indicator #" .. i .. " - RED (used)")
                end
            end
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

    -- Destroy AP indicator entities
    for i = 1, #apIndicators do
        local entityID = apIndicators[i]
        if entityID > 0 then
            DestroyEntity(entityID)
            Log("  Destroyed AP indicator " .. i .. " (ID: " .. entityID .. ")")
        end
    end
    apIndicators = {}
    Log("AP indicators cleaned up")

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
