/*
===============================================================================
 File:          TileMapLoader.h
 Author:        JOSH ONG
 Email:         josh.o@digipen.edu
 Date:          2025-10-31
 Contribution:  100%
 ------------------------------------------------------------------------------
 Tile Map Loader Header

 Overview:
    The TileMapLevelLoader is a static utility class designed to load level
    geometry and game objects from JSON configuration files. It populates
    the game world with entities based on a grid-based map layout.

 Key Features:
    - JSON file parsing for TileDefinitions and Map Layout
    - Integration with GridECS for pathfinding and spatial queries
    - Entity spawning factory support
    - Automatic world-space calculation from grid coordinates

===============================================================================
*/

#pragma once
#include "Precompiled.h"
#include "ECSEntity.h"
#include "Grid/GridECS.h"
#include "EntitySpawner.h"
#include "Vector2D.h"
#include <string>

namespace Framework {

    class EntitySpawner;
    class EntityManager;

    /**
     * @struct LevelData
     * @brief Simple container for grid dimensions
     */
    struct LevelData {
        int rows = 0;
        int cols = 0;
    };

    /**
     * @class TileMapLevelLoader
     * @brief Static class for loading and instantiating tilemap levels
     *
     * This class reads a custom JSON format that defines:
     * 1. TileDefinitions: Characters mapped to textures and properties (solid, entityType)
     * 2. Tiles: Rows of characters representing the map layout
     *
     * Usage Pattern:
     * @code
     * TileMapLevelLoader::LoadLevel("assets/level1.json", spawner, em, startPos, spacing, size);
     * @endcode
     */
    class TileMapLevelLoader {
    public:
        /**
         * @brief Loads a level from a JSON file and spawns it into the world.
         * @param filepath Path to the .json file (e.g., "assets/level_map.json")
         * @param spawner Pointer to the EntitySpawner for creating objects
         * @param em Pointer to the EntityManager for component attachment
         * @param startPos World position of the top-left corner of the grid
         * @param spacing Spacing between tiles (width, height)
         * @param tileSize Visual size of the tiles
         * @return True if successful, false if file missing or parse error
         *
         * Loading Process:
         * 1. Opens file and runs manual JSON parse loop
         * 2. Configures the global Grid singleton with dimensions
         * 3. Iterates over map data to spawn Tiles
         * 4. Spawns special entities (Player, Enemy, Chests) on top of tiles
         */
        static bool LoadLevel(const std::string& filepath,
            EntitySpawner* spawner,
            EntityManager* em,
            const Vector2D& startPos,
            const Vector2D& spacing,
            const Vector2D& tileSize);

    private:
        /**
         * @struct TileDef
         * @brief Internal helper to store properties of a specific tile character
         */
        struct TileDef {
            std::string texture;      // Texture asset name
            bool solid = false;       // If true, tile blocks movement
            std::string entityType;   // "Player", "Enemy", "Chest", etc.
            int layer = -1;           // Render layer (-1 = use default based on solid flag)
        };

        /**
         * @brief Helper to remove quotes and whitespace from JSON strings
         */
        static std::string CleanString(std::string s);

    };

}
