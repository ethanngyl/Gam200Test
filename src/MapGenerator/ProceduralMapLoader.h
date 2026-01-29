#pragma once
/*
===============================================================================
 ProceduralMapLoader - Spawns Generated Map into Grid
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