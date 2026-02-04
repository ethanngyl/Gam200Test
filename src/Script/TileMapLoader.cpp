/*
===============================================================================
 File:          TileMapLoader.cpp
 Author:        JOSH ONG
 Co-authors:    SIM KAH YAN
 Email:         josh.o@digipen.edu, kahyan.sim@digipen.edu
 Date:          2025-10-31
 Contribution:  JOSH ONG 95% SIM KAH YAN 5%
 ------------------------------------------------------------------------------
  Tile Map Loader Implementation

 Overview:
    The TileMapLevelLoader is responsible for parsing custom JSON-formatted
    level files and generating the runtime game world. It bridges the gap
    between data (text files) and the ECS (Entities/Components).

  Design notes:
     - Uses a custom state-machine parser for JSON (std::ifstream) to avoid
       heavy dependencies for simple level data.
     - Performs a two-pass load:
       1. Parse definitions and map layout strings
       2. Instantiate entities and configure the Grid system
     - Handles Z-layering automatically based on "solid" properties.

 Copyright (C) 2025 DigiPen Institute of Technology.
 Reproduction or disclosure of this file or its contents
 without the prior written consent of DigiPen Institute of
 Technology is prohibited.
===============================================================================
*/

#include "Precompiled.h"
#include "LevelLoader.h"
#include <fstream>
#include <sstream>
#include <map>
#include <algorithm>
#include "RenderComponents.h"
#include "Pathfinding.h"
#include "TileMapLoader.h"
#include "Graphics/RenderLayers.h"

namespace Framework {

    /**
     * @brief Helper utility to sanitize string inputs from file
     * @param s Raw string from file
     * @return std::string Cleaned string
     *
     * Implementation details:
     * - Removes double quotes, commas, tabs, and spaces
     * - Used to parse JSON keys and values manually
     */
    std::string TileMapLevelLoader::CleanString(std::string s) {
        // Remove trailing commas, quotes, and whitespace
        s.erase(std::remove_if(s.begin(), s.end(), [](char c) {
            return c == ' ' || c == '\t' || c == '\"' || c == ',';
            }), s.end());
        return s;
    }

    /**
     * @brief Main loading function to parse file and spawn world entities
     * @param filepath Path to the JSON level file
     * @param spawner Factory for creating entities
     * @param em Entity Manager for component assignment
     * @param startPos World coordinates for the top-left of the grid
     * @param spacing Width/Height of each cell
     * @param tileSize Visual size of the tiles
     * @return true if level loaded successfully
     *
     * Implementation details:
     * - Phase 1: File Parsing
     * - reads line-by-line using a state machine (inTileDefinitions vs inTiles)
     * - Maps characters (e.g., 'W', 'G') to TileDef structs
     * - Phase 2: Grid Configuration
     * - Calculates world bounds based on row/col count and tile size
     * - Initializes the global Grid singleton
     * - Phase 3: Entity Spawning
     * - Iterates through the parsed string map
     * - Spawns base tiles (ground/walls) via EntitySpawner
     * - Spawns special occupants (Players, Chests, Goals)
     * - Sets render layers: Solid tiles render above ground to prevent Z-fighting
     */
    bool TileMapLevelLoader::LoadLevel(const std::string& filepath,
        EntitySpawner* spawner,
        EntityManager* em,
        const Vector2D& startPos,
        const Vector2D& spacing,
        const Vector2D& tileSize)
    {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            LOG_ERROR("LevelLoader", "Failed to open level file: %s", filepath.c_str());
            return false;
        }

        LOG_INFO("LevelLoader", "Loading level from %s...", filepath.c_str());

        std::string line;
        std::map<char, TileMapLevelLoader::TileDef> tileDefs;
        std::vector<std::string> rowStrings;

        // --- 1. Parse the JSON (Manual Parsing) ---
        bool inTileDefinitions = false;
        bool inTiles = false;
        char currentDefKey = 0;
        int nextId = 0;

        while (std::getline(file, line)) {
            // Simple state machine for parsing
            if (line.find("\"TileDefinitions\":") != std::string::npos) { inTileDefinitions = true; inTiles = false; continue; }
            if (line.find("\"Tiles\":") != std::string::npos) { inTileDefinitions = false; inTiles = true; continue; }

            if (inTileDefinitions) {
                // Check for new definition key like "W": {
                size_t quotePos = line.find("\"");
                if (quotePos != std::string::npos && line.find(":") != std::string::npos) {
                    std::string keyStr = line.substr(quotePos + 1, 1);
                    if (line.find("{") != std::string::npos) {
                        currentDefKey = keyStr[0];
                    }
                }

                // Parse properties
                if (currentDefKey != 0) {
                    if (line.find("\"texture\":") != std::string::npos) {
                        size_t valPos = line.find(":");
                        tileDefs[currentDefKey].texture = CleanString(line.substr(valPos + 1));
                    }
                    if (line.find("\"solid\":") != std::string::npos) {
                        size_t valPos = line.find(":");
                        std::string val = CleanString(line.substr(valPos + 1));
                        // Sets solid flag based on '1' vs '0'
                        tileDefs[currentDefKey].solid = (val == "1");
                    }
                    if (line.find("\"entityType\":") != std::string::npos) {
                        size_t valPos = line.find(":");
                        tileDefs[currentDefKey].entityType = CleanString(line.substr(valPos + 1));
                    }
                    if (line.find("\"layer\":") != std::string::npos) {
                        size_t valPos = line.find(":");
                        std::string val = CleanString(line.substr(valPos + 1));
                        tileDefs[currentDefKey].layer = std::stoi(val);
                    }
                }
            }

            if (inTiles) {
                // Parse Rows like "Row0": "WW..."
                if (line.find("\"Row") != std::string::npos) {
                    size_t colonPos = line.find(":");
                    if (colonPos != std::string::npos) {
                        std::string rowData = CleanString(line.substr(colonPos + 1));
                        if (!rowData.empty()) {
                            rowStrings.push_back(rowData);
                        }
                    }
                }
            }
        }

        // --- 2. Initialize Grid Configuration ---
        int rows = static_cast<int>(rowStrings.size());
        int cols = rows > 0 ? static_cast<int>(rowStrings[0].length()) : 0;

        if (rows == 0 || cols == 0) {
            LOG_ERROR("LevelLoader", "Invalid map data: 0 rows or cols.");
            return false;
        }

        // Setup global grid structure 
        Grid& grid = GetGrid();
        grid.rows = rows;
        grid.cols = cols;
        grid.startPos = startPos;
        grid.spacing = spacing;
        grid.em = em;
        grid.tileSize = tileSize;
        // ========================================================================
        // Compute the world-space minimum corner of the grid.
        // Offset by half a tile so the grid aligns correctly around its starting position.
        // Author: Sim Kah Yan
        // ========================================================================
        grid.worldbound_min = Vector2D{ startPos.x - tileSize.x * 0.5f, startPos.y - tileSize.y * 0.5f };
        grid.worldbound_max = Vector2D{ startPos.x - tileSize.x * 0.5f + cols * tileSize.x, startPos.y - tileSize.y * 0.5f + rows * tileSize.y };

        grid.tiles.assign(static_cast<size_t>(rows) * cols, Entity{ INVALID_ENTITY });
        LOG_INFO("LevelLoader", "Configured Grid %dx%d", cols, rows);

        auto baseTileFor = [&](const TileDef& def) -> const TileDef& {
            const bool isEntity = !def.entityType.empty();
            // '0' is guaranteed in your JSON; it’s the grass/ground tile.
            const TileDef& ground = tileDefs.at('0');
            return isEntity ? ground : def;
            };

        // ========================================================================
        // CHEST TRACKING - Count total chests in level
        // ========================================================================
        int totalChests = 0;
        int nextChestID = 1;

        // First pass: count chests
        for (int r = 0; r < rows; ++r) {
            std::string rowStr = rowStrings[r];
            for (int c = 0; c < cols; ++c) {
                char tileChar = (c < rowStr.length()) ? rowStr[c] : '0';
                if (tileChar == 'S') {  // 'S' = Chest in JSON
                    totalChests++;
                }
            }
        }

        LOG_INFO("LevelLoader", "Found %d chests in level", totalChests);

        // --- 3. Spawn Entities ---

        for (int r = 0; r < rows; ++r) {
            std::string rowStr = rowStrings[r];

            for (int c = 0; c < cols; ++c) {
                char tileChar = (c < rowStr.length()) ? rowStr[c] : '0';

                TileDef def = tileDefs['0'];
                if (tileDefs.find(tileChar) != tileDefs.end()) {
                    def = tileDefs[tileChar];
                }

                // Calculate Tile Center Position
                Vector2D pos(
                    startPos.x + c * spacing.x,
                    startPos.y + r * spacing.y
                );

                //  A. Spawn the Base Tile Entity
                const TileDef& baseDef = baseTileFor(def);
                Entity tileEntity = spawner->SpawnSprite(baseDef.texture, pos, tileSize);

                // Add GridTiles Component
                em->AddComponent<GridTiles>(tileEntity);
                auto& gridTile = em->GetComponent<GridTiles>(tileEntity);
                auto& mr = em->GetComponent<MeshRenderer>(tileEntity);
                mr.material = GraphicsSystemV2::Material2;

                // Set render layer to prevent Z-fighting
                // Use explicit layer if specified in JSON, otherwise use default based on solid flag
                if (def.layer != -1) {
                    // Explicit layer specified in tile definition
                    mr.layer = def.layer;
                }
                else {
                    // Default behavior: solid tiles (walls) render slightly above ground tiles
                    mr.layer = def.solid ? RenderLayers::Props : RenderLayers::Ground;
                }

                // Check if the tile definition marks it as solid
                gridTile.blocked = def.solid; //  THIS IS WHERE WALL BLOCKING IS SET 

                // Optional: Update rendering material here if needed
                // if (em->HasComponent<MeshRenderer>(tileEntity)) { ... }

                gridTile.tileId = nextId++;
                gridTile.x = c;
                gridTile.y = r;
                gridTile.entity = tileEntity;
                gridTile.centerWorld = pos;
                gridTile.tileW = spacing.x;
                gridTile.tileH = spacing.y;
                gridTile.occupant = INVALID_ENTITY;

                // Store in Grid for fast lookups
                grid.tiles[grid.Index(c, r)] = tileEntity;

                //  B. Spawn Special Entity (Player, Enemy, etc.) - ENEMY SPAWNING DISABLED FOR DEBUGGING
                if (!def.entityType.empty()) {
                    Entity specialEntity = { INVALID_ENTITY };

                    if (def.entityType == "Player") {
                        specialEntity = spawner->SpawnPlayer(pos);
                        if (!em->HasComponent<Inventory>(specialEntity)) {
                            em->AddComponent<Inventory>(specialEntity);
                        }
                        LOG_INFO("LevelLoader", "Spawned Player at (%d, %d)", c, r);
                    }
                    else if (def.entityType == "Enemy") {
                        // COMMENTED OUT FOR DEBUGGING - DISABLE ENEMY SPAWNING

                        specialEntity = spawner->SpawnEnemy(pos);
                        // Add EnemyAI and AP manually
                        if (!em->HasComponent<EnemyAI>(specialEntity)) {
                            em->AddComponent<EnemyAI>(specialEntity);
                        }
                        if (!em->HasComponent<AP>(specialEntity)) {
                            em->AddComponent<AP>(specialEntity, 3); // 3 AP
                        }

                        if (em->HasComponent<Renderable>(specialEntity)) {
                            auto& rend = em->GetComponent<Renderable>(specialEntity);
                            rend.visible = true;
                            rend.layer = RenderLayers::Enemies;
                        }

                        LOG_INFO("LevelLoader", "Spawned Enemy at (%d, %d)", c, r);

                        LOG_INFO("LevelLoader", "Skipped Enemy spawn at (%d, %d) - DEBUG MODE", c, r);
                    }
                    // ============================================================
                    // CHEST - Blocks enemies, collectable by player
                    // ============================================================
                    else if (def.entityType == "Chest") {
                        // Spawn 
                        // entity
                        specialEntity = spawner->SpawnSprite(
                            def.texture,
                            pos,
                            Vector2D(spacing.x * 0.8f, spacing.y * 0.8f)
                        );

                        // Add Chest component
                        em->AddComponent<Chest>(specialEntity, nextChestID);

                        // CRITICAL: Mark tile as BLOCKED for enemies
                        //gridTile.blocked = true;

                        LOG_INFO("LevelLoader", "Spawned Chest %d at (%d, %d) - BLOCKED for enemies",
                            nextChestID, c, r);

                        nextChestID++;
                    }
                    // ============================================================
                    // GOAL - Level exit, requires all chests
                    // ============================================================
                    else if (def.entityType == "Goal") {
                        // Spawn goal entity
                        specialEntity = spawner->SpawnSprite(
                            def.texture,
                            pos,
                            Vector2D(spacing.x * 0.9f, spacing.y * 0.9f)
                        );

                        // Add Goal component
                        em->AddComponent<Goal>(specialEntity, totalChests);

                        // CRITICAL: Mark tile as BLOCKED for enemies
                        //gridTile.blocked = true;

                        LOG_INFO("LevelLoader", "Spawned Goal at (%d, %d) - Requires %d chests - BLOCKED for enemies",
                            c, r, totalChests);
                    }

                    if (specialEntity.GetID() != INVALID_ENTITY) {
                        gridTile.occupant = specialEntity;
                    }
                }
            }
        }

        LOG_INFO("LevelLoader", "Level Loaded Successfully.");
        return true;
    }
}