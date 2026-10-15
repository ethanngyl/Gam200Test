#pragma once
/**
===============================================================================
 File:          ProceduralMapLoader.h
 Author:        Josh Ong
 Email:         josh.o@digipen.edu
 Date:          2-5-2026
 Contribution:  100%
 ------------------------------------------------------------------------------

 PROCEDURAL MAP LOADER - Grid System Integration

 Brief:
    Bridges the MapGenerator output with the Grid system and EntitySpawner.
    Takes a generated map and spawns tile entities into the game world,
    configuring the Grid for pathfinding and gameplay systems. Includes
    automatic tile variation (dark/light grass) and decorative rock placement.

 Features:
    - Configures Grid dimensions and world bounds
    - Spawns tile entities with proper render layers
    - Random grass variation (dark/light) for visual interest
    - Decorative rock spawning on floor tiles
    - GridTiles component setup for pathfinding

 Usage:
    // Configure map generation
    MapGen::Config config;
    config.width = 30;
    config.height = 20;
    config.algorithm = "rooms";

    // Load into game world
    MapGen::GeneratedMap map = ProceduralMapLoader::LoadProceduralLevel(
        config,
        entitySpawner,
        entityManager,
        Vector2D(0, 0),      // Start position
        Vector2D(1, 1),      // Tile spacing
        Vector2D(1, 1)       // Tile size
    );

    // Use spawn positions
    SpawnPlayer(map.playerSpawn);
    for (auto& pos : map.enemySpawns) { SpawnEnemy(pos); }


 Copyright (C) 2026 DigiPen Institute of Technology.
 Reproduction or disclosure of this file or its contents
 without the prior written consent of DigiPen Institute of
 Technology is prohibited.
===============================================================================
*/

#pragma once
#include "Precompiled.h"
#include "MapGenerator.h"
#include "EntitySpawner.h"
#include "ECSEntityManager.h"
#include "Vector2D.h"

namespace Framework {

    class ProceduralMapLoader {
    public:
        /**
         * @brief Load generated map into Grid system
         * @param config Map generation settings
         * @param spawner Your EntitySpawner
         * @param em Your EntityManager
         * @param startPos Top-left corner world position
         * @param spacing Tile spacing
         * @param tileSize Tile visual size
         * @return The generated map (for debugging)
         */
        static MapGen::GeneratedMap LoadProceduralLevel(
            const MapGen::Config& config,
            EntitySpawner* spawner,
            EntityManager* em,
            const Vector2D& startPos,
            const Vector2D& spacing,
            const Vector2D& tileSize
        );

    private:
        static void ConfigureGrid(
            const MapGen::GeneratedMap& map,
            EntityManager* em,
            const Vector2D& startPos,
            const Vector2D& spacing,
            const Vector2D& tileSize
        );

        static void SpawnTiles(
            const MapGen::GeneratedMap& map,
            EntitySpawner* spawner,
            EntityManager* em
        );
    };

} // namespace Framework