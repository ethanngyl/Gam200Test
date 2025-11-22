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

    // Simple helper to strip quotes and whitespace
    std::string TileMapLevelLoader::CleanString(std::string s) {
        // Remove trailing commas, quotes, and whitespace
        s.erase(std::remove_if(s.begin(), s.end(), [](char c) {
            return c == ' ' || c == '\t' || c == '\"' || c == ',';
            }), s.end());
        return s;
    }

    bool TileMapLevelLoader::LoadLevel(const std::string& filepath,
        EntitySpawner* spawner,
        EntityManager* em,
        const Vector2D& startPos,
        const Vector2D& spacing)
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
        grid.tiles.assign(static_cast<size_t>(rows) * cols, Entity{ INVALID_ENTITY });
        LOG_INFO("LevelLoader", "Configured Grid %dx%d", cols, rows);

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
                Entity tileEntity = spawner->SpawnSprite(def.texture, pos, Vector2D(spacing.x * 0.9f, spacing.y * 0.9f));

                // Add GridTiles Component
                em->AddComponent<GridTiles>(tileEntity);
                auto& gridTile = em->GetComponent<GridTiles>(tileEntity);
                auto& mr = em->GetComponent<MeshRenderer>(tileEntity);
                mr.material = GraphicsSystemV2::Material2;

                // Set render layer to prevent Z-fighting
                // Solid tiles (walls) render slightly above ground tiles
                mr.layer = def.solid ? RenderLayers::Props : RenderLayers::Ground;

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

                //  B. Spawn Special Entity (Player, Enemy, etc.) 
                if (!def.entityType.empty()) {
                    Entity specialEntity = { INVALID_ENTITY };

                    if (def.entityType == "Player") {
                        specialEntity = spawner->SpawnPlayer(pos);
                        LOG_INFO("LevelLoader", "Spawned Player at (%d, %d)", c, r);
                    }
                    else if (def.entityType == "Enemy") {
                        specialEntity = spawner->SpawnEnemy(pos);
                        // Add EnemyAI and AP manually 
                        if (!em->HasComponent<EnemyAI>(specialEntity)) {
                            em->AddComponent<EnemyAI>(specialEntity);
                        }
                        if (!em->HasComponent<AP>(specialEntity)) {
                            em->AddComponent<AP>(specialEntity, 2, 3); // 2 HP, 3 AP
                        }
                        LOG_INFO("LevelLoader", "Spawned Enemy at (%d, %d)", c, r);
                    }
                    // Add logic for Chest (S) and Goal (M) here...

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