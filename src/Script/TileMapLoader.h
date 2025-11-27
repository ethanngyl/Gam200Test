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

    struct LevelData {
        int rows = 0;
        int cols = 0;
    };

    class TileMapLevelLoader {
    public:
        /**
         * @brief Loads a level from a JSON file and spawns it into the world.
         * * @param filepath Path to the .json file (e.g., "assets/level_map.json")
         * @param spawner Pointer to the EntitySpawner
         * @param em Pointer to the EntityManager
         * @param startPos World position of the top-left corner of the grid
         * @param spacing Spacing between tiles (width, height)
         * @return True if successful, false otherwise
         */
        static bool LoadLevel(const std::string& filepath,
            EntitySpawner* spawner,
            EntityManager* em,
            const Vector2D& startPos,
            const Vector2D& spacing);

    private:
        // Helper struct for tile definitions
        struct TileDef {
            std::string texture;
            bool solid = false;
            std::string entityType;
            int layer = -1;  // Render layer (-1 = use default based on solid flag)
        };

        static std::string CleanString(std::string s);

    };

}