/**
===============================================================================
 File:          MapSerializer.cpp
 Author:        Josh Ong
 Date:          2026
 ------------------------------------------------------------------------------

 MAP SERIALIZER - Implementation

 Brief:
    Saves and loads GeneratedMap data as JSON files using nlohmann/json.
    The tile grid is stored as a compact string-per-row format (W/F characters)
    to keep file sizes reasonable for large maps.

 JSON Structure:
    {
      "version": 1,
      "dimensions": { "width": 30, "height": 20 },
      "algorithm": "rooms",
      "tiles": [ "WWWWWW...", "W....W...", ... ],   // One string per row
      "playerSpawn": { "x": 5, "y": 3 },
      "goalSpawn": { "x": 25, "y": 18 },
      "enemySpawns": [ {"x":10,"y":5}, ... ],
      "chestSpawns": [ {"x":8,"y":12}, ... ],
      "arena": { "hasArena": true, "center": {...}, "min": {...}, "max": {...} },
      "config": { ... }   // Original generation config for reference
    }


 Copyright (C) 2026 DigiPen Institute of Technology.
 Reproduction or disclosure of this file or its contents
 without the prior written consent of DigiPen Institute of
 Technology is prohibited.
===============================================================================
*/

#include "Precompiled.h"
#include "MapSerializer.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <filesystem>

using json = nlohmann::json;
namespace fs = std::filesystem;

namespace Framework {

    // ========================================================================
    // HELPER: Position to/from JSON
    // ========================================================================

    static json PositionToJson(const MapGen::Position& pos) {
        return json{ {"x", pos.x}, {"y", pos.y} };
    }

    static MapGen::Position PositionFromJson(const json& j) {
        return MapGen::Position(
            j.value("x", 0),
            j.value("y", 0)
        );
    }

    // ========================================================================
    // HELPER: Tile grid to/from compact string format
    // ========================================================================

    // Each row becomes a string: 'W' = WALL, 'F' = FLOOR
    // This is much more compact than storing individual tile objects
    static std::vector<std::string> TilesToStrings(const MapGen::GeneratedMap& map) {
        std::vector<std::string> rows;
        rows.reserve(map.height);

        for (int y = 0; y < map.height; y++) {
            std::string row;
            row.reserve(map.width);
            for (int x = 0; x < map.width; x++) {
                row += (map.getTile(x, y) == MapGen::TileType::WALL) ? 'W' : 'F';
            }
            rows.push_back(row);
        }
        return rows;
    }

    static bool StringsToTiles(const std::vector<std::string>& rows, MapGen::GeneratedMap& map) {
        if (rows.empty()) return false;

        int height = static_cast<int>(rows.size());
        int width = static_cast<int>(rows[0].size());

        // Validate all rows have the same width
        for (const auto& row : rows) {
            if (static_cast<int>(row.size()) != width) {
                std::cout << "[MapSerializer] ERROR: Inconsistent row widths in tile data!\n";
                return false;
            }
        }

        // Rebuild the map tiles
        map.width = width;
        map.height = height;
        map.tiles.resize(height);

        for (int y = 0; y < height; y++) {
            map.tiles[y].resize(width);
            for (int x = 0; x < width; x++) {
                char c = rows[y][x];
                if (c == 'W') {
                    map.tiles[y][x] = MapGen::TileType::WALL;
                }
                else if (c == 'F') {
                    map.tiles[y][x] = MapGen::TileType::FLOOR;
                }
                else {
                    std::cout << "[MapSerializer] WARNING: Unknown tile char '" << c
                        << "' at (" << x << "," << y << "), defaulting to WALL\n";
                    map.tiles[y][x] = MapGen::TileType::WALL;
                }
            }
        }

        return true;
    }

    // ========================================================================
    // HELPER: Config to/from JSON
    // ========================================================================

    static json ConfigToJson(const MapGen::Config& config) {
        json j;
        j["width"] = config.width;
        j["height"] = config.height;
        j["algorithm"] = config.algorithm;

        // Room settings
        j["minRoomSize"] = config.minRoomSize;
        j["maxRoomSize"] = config.maxRoomSize;
        j["maxRooms"] = config.maxRooms;

        // Cellular settings
        j["wallChance"] = config.wallChance;
        j["smoothPasses"] = config.smoothPasses;

        // Entity counts
        j["minEnemies"] = config.minEnemies;
        j["maxEnemies"] = config.maxEnemies;
        j["minChests"] = config.minChests;
        j["maxChests"] = config.maxChests;

        // Placement constraints
        j["minPlayerEnemyDistance"] = config.minPlayerEnemyDistance;
        j["minEnemyEnemyDistance"] = config.minEnemyEnemyDistance;
        j["minPlayerChestDistance"] = config.minPlayerChestDistance;
        j["playerSafeZoneRadius"] = config.playerSafeZoneRadius;
        j["ensureGoalIsFar"] = config.ensureGoalIsFar;
        j["minPlayerGoalDistance"] = config.minPlayerGoalDistance;
        j["maxPlacementAttempts"] = config.maxPlacementAttempts;

        // Arena settings
        j["arenaWidth"] = config.arenaWidth;
        j["arenaHeight"] = config.arenaHeight;
        j["minArenaDistFromPlayer"] = config.minArenaDistFromPlayer;

        return j;
    }

    static MapGen::Config ConfigFromJson(const json& j) {
        MapGen::Config config;

        config.width = j.value("width", 30);
        config.height = j.value("height", 20);
        config.algorithm = j.value("algorithm", std::string("rooms"));

        config.minRoomSize = j.value("minRoomSize", 4);
        config.maxRoomSize = j.value("maxRoomSize", 10);
        config.maxRooms = j.value("maxRooms", 8);

        config.wallChance = j.value("wallChance", 0.45f);
        config.smoothPasses = j.value("smoothPasses", 4);

        config.minEnemies = j.value("minEnemies", 2);
        config.maxEnemies = j.value("maxEnemies", 5);
        config.minChests = j.value("minChests", 1);
        config.maxChests = j.value("maxChests", 3);

        config.minPlayerEnemyDistance = j.value("minPlayerEnemyDistance", 5);
        config.minEnemyEnemyDistance = j.value("minEnemyEnemyDistance", 3);
        config.minPlayerChestDistance = j.value("minPlayerChestDistance", 2);
        config.playerSafeZoneRadius = j.value("playerSafeZoneRadius", 2);
        config.ensureGoalIsFar = j.value("ensureGoalIsFar", true);
        config.minPlayerGoalDistance = j.value("minPlayerGoalDistance", 10);
        config.maxPlacementAttempts = j.value("maxPlacementAttempts", 100);

        config.arenaWidth = j.value("arenaWidth", 9);
        config.arenaHeight = j.value("arenaHeight", 9);
        config.minArenaDistFromPlayer = j.value("minArenaDistFromPlayer", 8);

        return config;
    }

    // ========================================================================
    // SAVE
    // ========================================================================

    bool MapSerializer::Save(
        const MapGen::GeneratedMap& map,
        const MapGen::Config& config,
        const std::string& filepath
    ) {
        std::cout << "[MapSerializer] Saving map to: " << filepath << "\n";

        try {
            json root;

            // Metadata
            root["version"] = SERIALIZER_VERSION;

            // Timestamp
            auto now = std::chrono::system_clock::now();
            auto time = std::chrono::system_clock::to_time_t(now);
            struct tm timeInfo;
            localtime_s(&timeInfo, &time);
            std::stringstream ss;
            ss << std::put_time(&timeInfo, "%Y-%m-%d %H:%M:%S");
            root["savedAt"] = ss.str();

            // Dimensions
            root["dimensions"] = json{
                {"width", map.width},
                {"height", map.height}
            };

            // Algorithm (from config)
            root["algorithm"] = config.algorithm;

            // Tile grid (compact string format)
            root["tiles"] = TilesToStrings(map);

            // Entity spawns
            root["playerSpawn"] = PositionToJson(map.playerSpawn);
            root["goalSpawn"] = PositionToJson(map.goalSpawn);

            // Enemy spawns array
            json enemies = json::array();
            for (const auto& pos : map.enemySpawns) {
                enemies.push_back(PositionToJson(pos));
            }
            root["enemySpawns"] = enemies;

            // Chest spawns array
            json chests = json::array();
            for (const auto& pos : map.chestSpawns) {
                chests.push_back(PositionToJson(pos));
            }
            root["chestSpawns"] = chests;

            // Arena data
            json arena;
            arena["hasArena"] = map.hasArena;
            if (map.hasArena) {
                arena["center"] = PositionToJson(map.arenaCenter);
                arena["min"] = PositionToJson(map.arenaMin);
                arena["max"] = PositionToJson(map.arenaMax);
            }
            root["arena"] = arena;

            // Original generation config (for reference / re-tweaking)
            root["config"] = ConfigToJson(config);

            // Ensure directory exists
            fs::path dir = fs::path(filepath).parent_path();
            if (!dir.empty() && !fs::exists(dir)) {
                fs::create_directories(dir);
                std::cout << "[MapSerializer] Created directory: " << dir.string() << "\n";
            }

            // Write to file (pretty-printed with 2-space indent)
            std::ofstream file(filepath);
            if (!file.is_open()) {
                std::cout << "[MapSerializer] ERROR: Could not open file for writing: " << filepath << "\n";
                return false;
            }

            file << root.dump(2);
            file.close();

            std::cout << "[MapSerializer] Map saved successfully! ("
                << map.width << "x" << map.height << ", "
                << map.enemySpawns.size() << " enemies, "
                << map.chestSpawns.size() << " chests)\n";

            return true;
        }
        catch (const std::exception& e) {
            std::cout << "[MapSerializer] ERROR: Failed to save: " << e.what() << "\n";
            return false;
        }
    }

    // ========================================================================
    // LOAD
    // ========================================================================

    bool MapSerializer::Load(
        const std::string& filepath,
        MapGen::GeneratedMap& outMap,
        MapGen::Config& outConfig
    ) {
        std::cout << "[MapSerializer] Loading map from: " << filepath << "\n";

        try {
            // Read file
            std::ifstream file(filepath);
            if (!file.is_open()) {
                std::cout << "[MapSerializer] ERROR: Could not open file: " << filepath << "\n";
                return false;
            }

            json root = json::parse(file);
            file.close();

            // Version check
            int version = root.value("version", 0);
            if (version != SERIALIZER_VERSION) {
                std::cout << "[MapSerializer] WARNING: File version " << version
                    << " differs from current version " << SERIALIZER_VERSION << "\n";
                // Continue anyway � we use .value() with defaults so older formats
                // will gracefully fall back
            }

            // Load dimensions
            int width = root["dimensions"].value("width", 30);
            int height = root["dimensions"].value("height", 20);

            // Load tile grid
            if (!root.contains("tiles") || !root["tiles"].is_array()) {
                std::cout << "[MapSerializer] ERROR: Missing or invalid 'tiles' array!\n";
                return false;
            }

            std::vector<std::string> tileStrings = root["tiles"].get<std::vector<std::string>>();

            // Rebuild GeneratedMap from tile strings
            if (!StringsToTiles(tileStrings, outMap)) {
                std::cout << "[MapSerializer] ERROR: Failed to parse tile data!\n";
                return false;
            }

            // Verify dimensions match
            if (outMap.width != width || outMap.height != height) {
                std::cout << "[MapSerializer] WARNING: Dimension mismatch! Header says "
                    << width << "x" << height << " but tiles are "
                    << outMap.width << "x" << outMap.height << "\n";
                // Use the tile-derived dimensions (they're the truth)
            }

            // Load entity spawns
            if (root.contains("playerSpawn")) {
                outMap.playerSpawn = PositionFromJson(root["playerSpawn"]);
            }

            if (root.contains("goalSpawn")) {
                outMap.goalSpawn = PositionFromJson(root["goalSpawn"]);
            }

            // Enemy spawns
            outMap.enemySpawns.clear();
            if (root.contains("enemySpawns") && root["enemySpawns"].is_array()) {
                for (const auto& e : root["enemySpawns"]) {
                    outMap.enemySpawns.push_back(PositionFromJson(e));
                }
            }

            // Chest spawns
            outMap.chestSpawns.clear();
            if (root.contains("chestSpawns") && root["chestSpawns"].is_array()) {
                for (const auto& c : root["chestSpawns"]) {
                    outMap.chestSpawns.push_back(PositionFromJson(c));
                }
            }

            // Arena data
            outMap.hasArena = false;
            if (root.contains("arena")) {
                const auto& arena = root["arena"];
                outMap.hasArena = arena.value("hasArena", false);
                if (outMap.hasArena) {
                    if (arena.contains("center")) outMap.arenaCenter = PositionFromJson(arena["center"]);
                    if (arena.contains("min"))    outMap.arenaMin = PositionFromJson(arena["min"]);
                    if (arena.contains("max"))    outMap.arenaMax = PositionFromJson(arena["max"]);
                }
            }

            // Load config (for reference)
            if (root.contains("config")) {
                outConfig = ConfigFromJson(root["config"]);
            }

            std::cout << "[MapSerializer] Map loaded successfully!\n";
            std::cout << "[MapSerializer]   Size: " << outMap.width << "x" << outMap.height << "\n";
            std::cout << "[MapSerializer]   Player: (" << outMap.playerSpawn.x << ", " << outMap.playerSpawn.y << ")\n";
            std::cout << "[MapSerializer]   Goal: (" << outMap.goalSpawn.x << ", " << outMap.goalSpawn.y << ")\n";
            std::cout << "[MapSerializer]   Enemies: " << outMap.enemySpawns.size() << "\n";
            std::cout << "[MapSerializer]   Chests: " << outMap.chestSpawns.size() << "\n";
            std::cout << "[MapSerializer]   Arena: " << (outMap.hasArena ? "yes" : "no") << "\n";

            return true;
        }
        catch (const json::parse_error& e) {
            std::cout << "[MapSerializer] ERROR: JSON parse error: " << e.what() << "\n";
            return false;
        }
        catch (const std::exception& e) {
            std::cout << "[MapSerializer] ERROR: Failed to load: " << e.what() << "\n";
            return false;
        }
    }

    // ========================================================================
    // QUICK SAVE (auto-named with timestamp)
    // ========================================================================

    std::string MapSerializer::QuickSave(
        const MapGen::GeneratedMap& map,
        const MapGen::Config& config,
        const std::string& directory
    ) {
        // Generate filename: map_20260205_143052.map.json
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);

        std::stringstream ss;
        ss << directory;

        // Ensure directory ends with separator
        if (!directory.empty() && directory.back() != '/' && directory.back() != '\\') {
            ss << '/';
        }

        struct tm timeInfo;
        localtime_s(&timeInfo, &time);
        ss << "map_" << std::put_time(&timeInfo, "%Y%m%d_%H%M%S")
            << ".map.json";

        std::string filepath = ss.str();

        if (Save(map, config, filepath)) {
            return filepath;
        }

        return "";
    }

    // ========================================================================
    // VALIDATION
    // ========================================================================

    bool MapSerializer::IsValidMapFile(const std::string& filepath) {
        try {
            std::ifstream file(filepath);
            if (!file.is_open()) return false;

            json root = json::parse(file);
            file.close();

            // Check required fields
            return root.contains("version") &&
                root.contains("dimensions") &&
                root.contains("tiles") &&
                root["tiles"].is_array() &&
                !root["tiles"].empty();
        }
        catch (...) {
            return false;
        }
    }

    // ========================================================================
    // LIST SAVED MAPS
    // ========================================================================

    std::vector<std::string> MapSerializer::ListSavedMaps(const std::string& directory) {
        std::vector<std::string> maps;

        try {
            if (!fs::exists(directory)) {
                return maps;
            }

            for (const auto& entry : fs::directory_iterator(directory)) {
                if (entry.is_regular_file()) {
                    std::string filename = entry.path().filename().string();
                    // Check for .map.json extension
                    if (filename.size() > 9 &&
                        filename.substr(filename.size() - 9) == ".map.json") {
                        maps.push_back(entry.path().string());
                    }
                }
            }

            // Sort alphabetically (newest timestamps last)
            std::sort(maps.begin(), maps.end());
        }
        catch (const std::exception& e) {
            std::cout << "[MapSerializer] ERROR listing maps: " << e.what() << "\n";
        }

        return maps;
    }

} // namespace Framework