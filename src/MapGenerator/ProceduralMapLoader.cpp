/**
===============================================================================
 File:          ProceduralMapLoader.cpp
 Author:        Josh Ong
 Email:         josh.o@digipen.edu
 Date:          2-5-2026
 Contribution:  100%
 ------------------------------------------------------------------------------

 PROCEDURAL MAP LOADER - Implementation

 Brief:
    Implementation of the procedural map loading system. Configures Grid
    dimensions, spawns tile entities with render layer assignments, and
    adds decorative elements (grass variation, rocks) for visual variety.


 Copyright (C) 2026 DigiPen Institute of Technology.
 Reproduction or disclosure of this file or its contents
 without the prior written consent of DigiPen Institute of
 Technology is prohibited.
===============================================================================
*/

#include "Precompiled.h"
#include "ProceduralMapLoader.h"
#include "Grid/Grid.h"
#include "Grid/GridTile.h"
#include <iostream>
#include <random>

namespace Framework {

    MapGen::GeneratedMap ProceduralMapLoader::LoadProceduralLevel(
        const MapGen::Config& config,
        EntitySpawner* spawner,
        EntityManager* em,
        const Vector2D& startPos,
        const Vector2D& spacing,
        const Vector2D& tileSize
    ) {
        std::cout << "[ProceduralMapLoader] Loading procedural level...\n";

        // Generate the map
        MapGen::Generator generator;
        MapGen::GeneratedMap map = generator.generate(config);

        // Print to console for debugging
        MapGen::Generator::printMap(map);

        // Configure Grid system
        ConfigureGrid(map, em, startPos, spacing, tileSize);

        // Spawn tile entities with decorations
        SpawnTiles(map, spawner, em);

        std::cout << "[ProceduralMapLoader] Level loaded!\n";
        return map;
    }

    void ProceduralMapLoader::ConfigureGrid(
        const MapGen::GeneratedMap& map,
        EntityManager* em,
        const Vector2D& startPos,
        const Vector2D& spacing,
        const Vector2D& tileSize
    ) {
        Grid& grid = GetGrid();
        grid.em = em;
        grid.rows = map.height;
        grid.cols = map.width;
        grid.startPos = startPos;
        grid.spacing = spacing;
        grid.tileSize = tileSize;

        grid.worldbound_min = startPos;
        grid.worldbound_max = Vector2D(
            startPos.x + (grid.cols * spacing.x),
            startPos.y + (grid.rows * spacing.y)
        );

        grid.tiles.clear();
        grid.tiles.resize(grid.rows * grid.cols);

        std::cout << "[ProceduralMapLoader] Grid configured: "
            << grid.rows << "x" << grid.cols << "\n";
    }

    void ProceduralMapLoader::SpawnTiles(
        const MapGen::GeneratedMap& map,
        EntitySpawner* spawner,
        EntityManager* em
    ) {
        Grid& grid = GetGrid();

        // Random number generator for tile variation
        std::random_device rd;
        std::mt19937 rng(rd());
        std::uniform_int_distribution<int> grassTypeDist(0, 1);      // 0 = dark, 1 = light
        std::uniform_int_distribution<int> rockChanceDist(0, 100);   // For % chance
        std::uniform_int_distribution<int> rockTypeDist(1, 3);       // Rock 1, 2, or 3

        // ========================================
        // TEXTURE PATHS - Update these to match your asset locations
        // ========================================
        const std::string grassDark = "assets/TileMap/Grass_Block_dark_2.png";
        const std::string grassLight = "assets/TileMap/Grass_Block_light_1.png";
        const std::string wallTexture = "assets/TileMap/Tree_Block.png";

        // Rock textures - dark variants match dark grass, light match light grass
        const std::string rocksDark[3] = {
            "assets/TileMap/Rock_1_dark_1.png",
            "assets/TileMap/Rock_2_dark_2.png",
            "assets/TileMap/Rock_3_dark_1.png"
        };
        const std::string rocksLight[3] = {
            "assets/TileMap/Rock_1_light_1.png",
            "assets/TileMap/Rock_2_light_1.png",
            "assets/TileMap/Rock_3_light_1.png"
        };

        // ========================================
        // CONFIGURATION
        // ========================================
        const int ROCK_CHANCE_PERCENT = 15;  // 15% chance for a rock decoration

        int rockCount = 0;
        int darkGrassCount = 0;
        int lightGrassCount = 0;

        for (int y = 0; y < map.height; y++) {
            for (int x = 0; x < map.width; x++) {
                MapGen::TileType type = map.getTile(x, y);

                // Calculate world position
                Vector2D worldPos(
                    grid.startPos.x + (x * grid.spacing.x),
                    grid.startPos.y + (y * grid.spacing.y)
                );

                std::string texture;
                bool blocked = false;
                bool isDarkGrass = false;

                if (type == MapGen::TileType::WALL) {
                    texture = wallTexture;
                    blocked = true;
                }
                else {
                    // Randomly choose dark or light grass (50/50)
                    isDarkGrass = (grassTypeDist(rng) == 0);
                    texture = isDarkGrass ? grassDark : grassLight;
                    blocked = false;

                    if (isDarkGrass) darkGrassCount++;
                    else lightGrassCount++;
                }

                // Spawn the base tile
                Vector2D tileScale(0.1f, 0.1f);
                Entity tileEntity = spawner->SpawnSprite(texture, worldPos, tileScale);

                // Set tile to Ground layer (layer 0)
                if (em->HasComponent<MeshRenderer>(tileEntity)) {
                    auto& mr = em->GetComponent<MeshRenderer>(tileEntity);
                    mr.layer = RenderLayers::Ground;
                }

                // Store in grid
                grid.tiles[grid.Index(x, y)] = tileEntity;

                // Add GridTiles component
                if (!em->HasComponent<GridTiles>(tileEntity)) {
                    em->AddComponent<GridTiles>(tileEntity);
                }

                auto& gridTile = em->GetComponent<GridTiles>(tileEntity);
                gridTile.x = x;
                gridTile.y = y;
                gridTile.tileId = static_cast<int>(grid.Index(x, y));
                gridTile.entity = tileEntity;
                gridTile.centerWorld = worldPos;
                gridTile.tileW = grid.tileSize.x;
                gridTile.tileH = grid.tileSize.y;
                gridTile.blocked = blocked;
                gridTile.occupant = INVALID_ENTITY;

                // ========================================
                // DECORATIVE ROCKS (only on floor tiles)
                // ========================================
                if (type == MapGen::TileType::FLOOR) {
                    int roll = rockChanceDist(rng);

                    if (roll < ROCK_CHANCE_PERCENT) {
                        // Pick a random rock type (1-3)
                        int rockType = rockTypeDist(rng) - 1;  // 0, 1, or 2 for array index

                        // Match rock variant to grass type (dark rocks on dark grass, etc.)
                        std::string rockTexture = isDarkGrass ? rocksDark[rockType] : rocksLight[rockType];

                        // Spawn rock decoration at same position
                        Entity rockEntity = spawner->SpawnSprite(rockTexture, worldPos, tileScale);

                        // Set rock to Props layer (layer 1) - above Ground, below Enemies/Player
                        if (em->HasComponent<MeshRenderer>(rockEntity)) {
                            auto& rockMr = em->GetComponent<MeshRenderer>(rockEntity);
                            rockMr.layer = RenderLayers::Props;
                        }

                        rockCount++;
                    }
                }
            }
        }

        std::cout << "[ProceduralMapLoader] Spawned " << grid.tiles.size() << " tiles\n";
        std::cout << "[ProceduralMapLoader]   Dark grass: " << darkGrassCount << "\n";
        std::cout << "[ProceduralMapLoader]   Light grass: " << lightGrassCount << "\n";
        std::cout << "[ProceduralMapLoader]   Rock decorations: " << rockCount << "\n";
    }

} // namespace Framework