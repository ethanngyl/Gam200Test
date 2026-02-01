/*
===============================================================================
 ProceduralMapLoader Implementation
===============================================================================
*/

#include "Precompiled.h"
#include "ProceduralMapLoader.h"
#include "Grid/Grid.h"
#include "Grid/GridTile.h"
#include <iostream>

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

        // Spawn tile entities
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

        for (int y = 0; y < map.height; y++) {
            for (int x = 0; x < map.width; x++) {
                MapGen::TileType type = map.getTile(x, y);

                // Calculate world position
                Vector2D worldPos(
                    grid.startPos.x + (x * grid.spacing.x),
                    grid.startPos.y + (y * grid.spacing.y)
                );

                // Choose texture based on type
                std::string texture;
                bool blocked = false;

                if (type == MapGen::TileType::WALL) {
                    texture = "assets/TileMap/Tree_Block.png";
                    blocked = true;
                }
                else {
                    texture = "assets/TileMap/Grass_Block.png";
                    blocked = false;
                }

                // Spawn tile using SpawnSprite
                Vector2D tileScale(0.1f, 0.1f);
                Entity tileEntity = spawner->SpawnSprite(texture, worldPos, tileScale);

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
            }
        }

        std::cout << "[ProceduralMapLoader] Spawned " << grid.tiles.size() << " tiles\n";
    }

} // namespace Framework