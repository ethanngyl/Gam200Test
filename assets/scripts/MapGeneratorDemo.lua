--[[
===============================================================================
 File:          MapGeneratorDemo.lua
 Authors:       
 Co-Authors:    
 Date:          
 Contribution:  
 ------------------------------------------------------------------------------

 MAP GENERATOR DEMO - Interactive Map Generation Showcase

 Brief:
    Demo script demonstrating the MapGenerator system. Allows switching between
    different generation algorithms in real-time, generating new random maps,
    and saving maps to JSON files. Useful for testing and previewing procedural
    map generation before integrating into gameplay levels.

 Controls:
    1 - Switch to Rooms & Corridors algorithm
    2 - Switch to Cellular Automata algorithm
    3 - Switch to Open Arena algorithm
    G - Generate new random map with current algorithm
    S - Save current map to JSON file
    5 - Return to main menu

 Usage:
    -- Load this script as a level to test map generation
    -- Generated maps are saved to assets/JSON/GeneratedMap_<algorithm>.json


 Copyright (C) 2026 DigiPen Institute of Technology.
 Reproduction or disclosure of this file or its contents
 without the prior written consent of DigiPen Institute of
 Technology is prohibited.
===============================================================================
]]--

local MapGenerator = require("MapGenerator")
local ButtonManager = require("ButtonManager")

-- ============================================================================
-- STATE
-- ============================================================================

local currentMap = nil
local currentAlgorithm = "rooms"
local algorithms = {"rooms", "cellular", "open"}
local algorithmIndex = 1

-- ============================================================================
-- LIFECYCLE: OnInit
-- ============================================================================

function OnInit()
    Log("========================================")
    Log("MAP GENERATOR DEMO")
    Log("========================================")

    SetCameraPosition(0.0, 0.0, 0.0)
    SetCameraZoom(2.0)
    SetEnginePlayState(true)

    -- Generate initial map
    GenerateNewMap()

    Log("========================================")
    Log("Controls:")
    Log("  1 - Rooms & Corridors algorithm")
    Log("  2 - Cellular Automata algorithm")
    Log("  3 - Open Arena algorithm")
    Log("  G - Generate new random map")
    Log("  S - Save current map to JSON")
    Log("  5 - Return to main menu")
    Log("========================================")
end

-- ============================================================================
-- LIFECYCLE: OnUpdate
-- ============================================================================

function OnUpdate(dt)
    -- Algorithm selection
    if IsKeyDown("1") then
        currentAlgorithm = "rooms"
        GenerateNewMap()
    elseif IsKeyDown("2") then
        currentAlgorithm = "cellular"
        GenerateNewMap()
    elseif IsKeyDown("3") then
        currentAlgorithm = "open"
        GenerateNewMap()
    end

    -- Generate new map
    if IsKeyDown("G") then
        GenerateNewMap()
    end

    -- Save current map
    if IsKeyDown("S") then
        if currentMap then
            local filename = "assets/JSON/GeneratedMap_" .. currentAlgorithm .. ".json"
            MapGenerator.SaveToFile(currentMap, filename)
            Log("Map saved to: " .. filename)
        else
            Log("No map to save!")
        end
    end

    -- Return to main menu
    if IsKeyDown("5") then
        SafeSwitchLevel("mainMenu", "Main Menu")
    end
end

-- ============================================================================
-- LIFECYCLE: OnDraw
-- ============================================================================

function OnDraw()
    -- Display current algorithm
    DrawText("Sans48", "Algorithm: " .. currentAlgorithm:upper(), 50, 50, 1.0, 1.0, 1.0, 1.0)
    DrawText("Sans32", "Press 1/2/3 to change algorithm", 50, 100, 0.8, 0.8, 0.8, 1.0)
    DrawText("Sans32", "Press G to generate new map", 50, 130, 0.8, 0.8, 0.8, 1.0)
    DrawText("Sans32", "Press S to save to JSON", 50, 160, 0.8, 0.8, 0.8, 1.0)

    if currentMap then
        local statsY = 220
        DrawText("Sans32", "Map Stats:", 50, statsY, 1.0, 1.0, 0.3, 1.0)
        DrawText("Playfair48", "  Size: " .. currentMap.width .. "x" .. currentMap.height, 50, statsY + 30, 0.8, 0.8, 0.8, 1.0)
        DrawText("Playfair48", "  Enemies: " .. #currentMap.enemyPositions, 50, statsY + 55, 0.8, 0.8, 0.8, 1.0)
        DrawText("Playfair48", "  Chests: " .. #currentMap.chestPositions, 50, statsY + 80, 0.8, 0.8, 0.8, 1.0)
    end
end

-- ============================================================================
-- LIFECYCLE: OnDestroy
-- ============================================================================

function OnDestroy()
    Log("Map Generator Demo destroyed")
    StopAllSounds()
end

-- ============================================================================
-- HELPER FUNCTIONS
-- ============================================================================

function GenerateNewMap()
    Log("Generating new map with algorithm: " .. currentAlgorithm)

    local config = {
        width = 21,
        height = 26,
        algorithm = currentAlgorithm,
        minRoomSize = 3,
        maxRoomSize = 8,
        maxRooms = 10,
        fillProbability = 0.45,
        smoothingIterations = 4,
        minEnemies = 2,
        maxEnemies = 5,
        minChests = 2,
        maxChests = 4,
        forceBorders = true
    }

    currentMap = MapGenerator.Generate(config)

    if currentMap then
        -- Print to console for debugging
        MapGenerator.Print(currentMap)

        -- Load the map into the game
        -- Note: This requires a function to convert map to game tiles
        -- For now, we just print it
        Log("Map generated successfully!")
        Log("Use 'S' key to save to JSON, then load it in Level3")
    else
        Log("ERROR: Map generation failed!")
    end
end

function SafeSwitchLevel(levelName, displayName)
    Log("Switching to level: " .. displayName)
    -- Implementation depends on your level loading system
end
