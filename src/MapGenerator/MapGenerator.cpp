/**
===============================================================================
 File:          MapGenerator.cpp
 Author:        Josh Ong
 Email:         josh.o@digipen.edu
 Date:          2-5-2026
 Contribution:  100%
 ------------------------------------------------------------------------------

 MAP GENERATOR - Implementation

 Brief:
    Implementation of the procedural map generation system. Contains algorithms
    for room-based dungeons, cellular automata caves, and open arenas. Includes
    smart entity placement with distance constraints and reachability validation
    using flood fill pathfinding.


 Copyright (C) 2026 DigiPen Institute of Technology.
 Reproduction or disclosure of this file or its contents
 without the prior written consent of DigiPen Institute of
 Technology is prohibited.
===============================================================================
*/

#include "Precompiled.h"
#include "MapGenerator.h"
#include <iostream>
#include <algorithm>
#include <cmath>

namespace Framework {
    namespace MapGen {

        // ============================================================================
        // GeneratedMap Implementation
        // ============================================================================

        GeneratedMap::GeneratedMap(int w, int h)
            : width(w), height(h) {
            tiles.resize(h);
            for (int y = 0; y < h; y++) {
                tiles[y].resize(w, TileType::WALL);
            }
        }

        bool GeneratedMap::isValid(int x, int y) const {
            return x >= 0 && x < width && y >= 0 && y < height;
        }

        TileType GeneratedMap::getTile(int x, int y) const {
            if (!isValid(x, y)) return TileType::WALL;
            return tiles[y][x];
        }

        void GeneratedMap::setTile(int x, int y, TileType type) {
            if (isValid(x, y)) {
                tiles[y][x] = type;
            }
        }

        // ============================================================================
        // Generator - Random Utilities
        // ============================================================================

        Generator::Generator() {
            rng.seed(std::random_device{}());
        }

        Generator::Generator(unsigned int seed) {
            rng.seed(seed);
        }

        Generator::~Generator() {
        }

        int Generator::randInt(int min, int max) {
            std::uniform_int_distribution<int> dist(min, max);
            return dist(rng);
        }

        float Generator::randFloat() {
            std::uniform_real_distribution<float> dist(0.0f, 1.0f);
            return dist(rng);
        }

        bool Generator::randBool(float probability) {
            return randFloat() < probability;
        }

        template<typename T>
        void Generator::shuffle(std::vector<T>& vec) {
            std::shuffle(vec.begin(), vec.end(), rng);
        }

        // ============================================================================
        // ALGORITHM 1: ROOMS (With Overlap Detection & Corridors)
        // ============================================================================

        bool Generator::roomsOverlap(const Room& r1, const Room& r2) const {
            // Check if rectangles overlap (with 1-tile buffer between rooms)
            // This prevents rooms from touching each other
            return !(r1.x + r1.width < r2.x - 1 ||
                r2.x + r2.width < r1.x - 1 ||
                r1.y + r1.height < r2.y - 1 ||
                r2.y + r2.height < r1.y - 1);
        }

        void Generator::carveRoom(GeneratedMap& map, const Room& room) {
            for (int y = room.y; y < room.y + room.height; y++) {
                for (int x = room.x; x < room.x + room.width; x++) {
                    if (map.isValid(x, y)) {
                        map.setTile(x, y, TileType::FLOOR);
                    }
                }
            }
        }

        void Generator::carveHorizontalCorridor(GeneratedMap& map, int x1, int x2, int y) {
            int xStart = min(x1, x2);
            int xEnd = max(x1, x2);
            for (int x = xStart; x <= xEnd; x++) {
                if (map.isValid(x, y)) {
                    map.setTile(x, y, TileType::FLOOR);
                }
            }
        }

        void Generator::carveVerticalCorridor(GeneratedMap& map, int y1, int y2, int x) {
            int yStart = min(y1, y2);
            int yEnd = max(y1, y2);
            for (int y = yStart; y <= yEnd; y++) {
                if (map.isValid(x, y)) {
                    map.setTile(x, y, TileType::FLOOR);
                }
            }
        }

        void Generator::createCorridor(GeneratedMap& map, const Room& r1, const Room& r2) {
            Position c1 = r1.center();
            Position c2 = r2.center();

            // Randomly choose L-shaped corridor direction
            if (randBool()) {
                // Horizontal then vertical
                carveHorizontalCorridor(map, c1.x, c2.x, c1.y);
                carveVerticalCorridor(map, c1.y, c2.y, c2.x);
            }
            else {
                // Vertical then horizontal
                carveVerticalCorridor(map, c1.y, c2.y, c1.x);
                carveHorizontalCorridor(map, c1.x, c2.x, c2.y);
            }
        }

        void Generator::generateRooms(GeneratedMap& map, const Config& config) {
            std::cout << "[MapGen] Generating rooms with overlap detection...\n";

            // Safety check for small maps
            int minMapSize = config.maxRoomSize + 4;
            if (map.width < minMapSize || map.height < minMapSize) {
                std::cout << "[MapGen] WARNING: Map too small for configured room sizes!\n";
                std::cout << "[MapGen]   Map: " << map.width << "x" << map.height << "\n";
                std::cout << "[MapGen]   Need: " << minMapSize << "x" << minMapSize << "\n";
                std::cout << "[MapGen] Using smaller room instead...\n";

                int safeSize = min(map.width, map.height) / 3;
                if (safeSize < 3) safeSize = 3;

                int w = min(safeSize, map.width - 4);
                int h = min(safeSize, map.height - 4);
                int x = (map.width - w) / 2;
                int y = (map.height - h) / 2;

                Room centerRoom(x, y, w, h);
                carveRoom(map, centerRoom);

                std::cout << "[MapGen] Placed 1 room (adjusted for small map)\n";
                return;
            }

            std::vector<Room> rooms;

            // Try to place rooms
            for (int attempt = 0; attempt < config.maxRooms * 10; attempt++) {
                if (rooms.size() >= static_cast<size_t>(config.maxRooms)) break;

                // Pick random size
                int w = randInt(config.minRoomSize, config.maxRoomSize);
                int h = randInt(config.minRoomSize, config.maxRoomSize);

                // Safety check - room fits in map
                if (w >= map.width - 4 || h >= map.height - 4) {
                    continue;
                }

                int maxX = map.width - w - 2;
                int maxY = map.height - h - 2;

                if (maxX < 2 || maxY < 2) {
                    continue;
                }

                // Pick random position
                int x = randInt(2, maxX);
                int y = randInt(2, maxY);

                Room newRoom(x, y, w, h);

                // Check overlap with existing rooms
                bool overlaps = false;
                for (const auto& room : rooms) {
                    if (roomsOverlap(newRoom, room)) {
                        overlaps = true;
                        break;
                    }
                }

                if (!overlaps) {
                    // No overlap - place the room!
                    carveRoom(map, newRoom);

                    // Connect to previous room with corridor
                    if (!rooms.empty()) {
                        createCorridor(map, rooms.back(), newRoom);
                    }

                    rooms.push_back(newRoom);
                }
            }

            std::cout << "[MapGen] Placed " << rooms.size() << " non-overlapping rooms\n";
        }

        // ============================================================================
        // ALGORITHM 2: CAVES (Cellular Automata)
        // ============================================================================

        int Generator::countAdjacentWalls(const GeneratedMap& map, int x, int y) const {
            int count = 0;
            for (int dy = -1; dy <= 1; dy++) {
                for (int dx = -1; dx <= 1; dx++) {
                    if (dx == 0 && dy == 0) continue;

                    int nx = x + dx;
                    int ny = y + dy;

                    if (!map.isValid(nx, ny) || map.getTile(nx, ny) == TileType::WALL) {
                        count++;
                    }
                }
            }
            return count;
        }

        void Generator::generateCaves(GeneratedMap& map, const Config& config) {
            std::cout << "[MapGen] Generating caves with cellular automata...\n";

            // Step 1: Random fill
            for (int y = 1; y < map.height - 1; y++) {
                for (int x = 1; x < map.width - 1; x++) {
                    if (randBool(config.wallChance)) {
                        map.setTile(x, y, TileType::WALL);
                    }
                    else {
                        map.setTile(x, y, TileType::FLOOR);
                    }
                }
            }

            // Step 2: Smoothing iterations
            for (int pass = 0; pass < config.smoothPasses; pass++) {
                auto oldTiles = map.tiles;

                for (int y = 1; y < map.height - 1; y++) {
                    for (int x = 1; x < map.width - 1; x++) {
                        int wallCount = countAdjacentWalls(map, x, y);

                        // Rule: If 5+ walls nearby, become wall; otherwise floor
                        if (wallCount >= 5) {
                            map.setTile(x, y, TileType::WALL);
                        }
                        else {
                            map.setTile(x, y, TileType::FLOOR);
                        }
                    }
                }
            }

            std::cout << "[MapGen] Cave generation complete!\n";
        }

        // ============================================================================
        // ALGORITHM 3: ARENA
        // ============================================================================

        void Generator::generateArena(GeneratedMap& map) {
            std::cout << "[MapGen] Generating open arena...\n";

            for (int y = 3; y < map.height - 3; y++) {
                for (int x = 3; x < map.width - 3; x++) {
                    map.setTile(x, y, TileType::FLOOR);
                }
            }

            std::cout << "[MapGen] Arena complete!\n";
        }

        // ============================================================================
        // ENTITY PLACEMENT - Distance & Constraint Checking
        // ============================================================================

        int Generator::manhattanDistance(const Position& a, const Position& b) const {
            return std::abs(a.x - b.x) + std::abs(a.y - b.y);
        }

        bool Generator::isTooClose(const Position& a, const Position& b, int minDistance) const {
            return manhattanDistance(a, b) < minDistance;
        }

        bool Generator::isInSafeZone(const Position& pos, const Position& center, int radius) const {
            return manhattanDistance(pos, center) <= radius;
        }

        bool Generator::hasWalkableNeighbors(const GeneratedMap& map, const Position& pos, int radius) const {
            int walkableCount = 0;
            int totalTiles = 0;

            for (int dy = -radius; dy <= radius; dy++) {
                for (int dx = -radius; dx <= radius; dx++) {
                    if (dx == 0 && dy == 0) continue;

                    int nx = pos.x + dx;
                    int ny = pos.y + dy;

                    if (map.isValid(nx, ny)) {
                        totalTiles++;
                        if (map.getTile(nx, ny) == TileType::FLOOR) {
                            walkableCount++;
                        }
                    }
                }
            }

            return totalTiles > 0 && walkableCount >= totalTiles / 2;
        }


        /**
         * @brief Check if a position is reachable from another using flood fill
         * @param map The generated map
         * @param from Starting position
         * @param to Target position
         * @return true if 'to' is reachable from 'from'
         */
        bool Generator::isReachable(const GeneratedMap& map, const Position& from, const Position& to) const {
            if (map.getTile(from.x, from.y) != TileType::FLOOR) return false;
            if (map.getTile(to.x, to.y) != TileType::FLOOR) return false;

            // Quick check: if same position
            if (from == to) return true;

            // BFS flood fill
            std::vector<std::vector<bool>> visited(map.height, std::vector<bool>(map.width, false));
            std::queue<Position> queue;

            queue.push(from);
            visited[from.y][from.x] = true;

            const int dx[] = { 0, 0, 1, -1 };
            const int dy[] = { 1, -1, 0, 0 };

            while (!queue.empty()) {
                Position current = queue.front();
                queue.pop();

                // Found target!
                if (current == to) {
                    return true;
                }

                // Check all 4 neighbors
                for (int i = 0; i < 4; i++) {
                    int nx = current.x + dx[i];
                    int ny = current.y + dy[i];

                    if (map.isValid(nx, ny) &&
                        !visited[ny][nx] &&
                        map.getTile(nx, ny) == TileType::FLOOR) {

                        visited[ny][nx] = true;
                        queue.push(Position(nx, ny));
                    }
                }
            }

            // Could not reach target
            return false;
        }

        /**
         * @brief Get all tiles reachable from a position
         * @param map The generated map
         * @param from Starting position
         * @return Vector of all reachable floor positions
         */
        std::vector<Position> Generator::getReachableTiles(const GeneratedMap& map, const Position& from) const {
            std::vector<Position> reachable;

            if (map.getTile(from.x, from.y) != TileType::FLOOR) {
                return reachable;
            }

            std::vector<std::vector<bool>> visited(map.height, std::vector<bool>(map.width, false));
            std::queue<Position> queue;

            queue.push(from);
            visited[from.y][from.x] = true;
            reachable.push_back(from);

            const int dx[] = { 0, 0, 1, -1 };
            const int dy[] = { 1, -1, 0, 0 };

            while (!queue.empty()) {
                Position current = queue.front();
                queue.pop();

                for (int i = 0; i < 4; i++) {
                    int nx = current.x + dx[i];
                    int ny = current.y + dy[i];

                    if (map.isValid(nx, ny) &&
                        !visited[ny][nx] &&
                        map.getTile(nx, ny) == TileType::FLOOR) {

                        visited[ny][nx] = true;
                        queue.push(Position(nx, ny));
                        reachable.push_back(Position(nx, ny));
                    }
                }
            }

            return reachable;
        }

        // ============================================================================
        // ENTITY PLACEMENT - Validation Functions
        // ============================================================================

        bool Generator::isValidPlayerSpawn(const GeneratedMap& map, const Position& pos,
            const std::vector<Position>& enemies, const Config& config) {

            // Must be floor tile
            if (map.getTile(pos.x, pos.y) != TileType::FLOOR) return false;

            // Must have walkable neighbors for pathfinding
            if (!hasWalkableNeighbors(map, pos)) return false;

            // Not in enemy safe zone
            for (const auto& e : enemies) {
                if (isTooClose(pos, e, config.minPlayerEnemyDistance)) return false;
            }

            // NEW: Must have at least 2 adjacent floor tiles for party members
            int adjacentFloors = 0;
            const int offsets[][2] = { {1,0}, {-1,0}, {0,1}, {0,-1}, {1,1}, {-1,1}, {1,-1}, {-1,-1} };

            for (const auto& offset : offsets) {
                int nx = pos.x + offset[0];
                int ny = pos.y + offset[1];
                if (map.isValid(nx, ny) && map.getTile(nx, ny) == TileType::FLOOR) {
                    adjacentFloors++;
                    if (adjacentFloors >= 2) break;  // Need at least 2 for 3-member party
                }
            }

            if (adjacentFloors < 2) {
                return false;  // Not enough space for party
            }

            return true;
        }

        bool Generator::isValidEnemySpawn(
            const GeneratedMap& map,
            const Position& pos,
            const Position& playerPos,
            const std::vector<Position>& otherEnemies,
            const Config& config) {

            // Must be floor tile
            if (map.getTile(pos.x, pos.y) != TileType::FLOOR) return false;

            // Not in player safe zone
            if (isInSafeZone(pos, playerPos, config.playerSafeZoneRadius)) return false;

            // Not too close to player
            if (isTooClose(pos, playerPos, config.minPlayerEnemyDistance)) return false;

            // Not too close to other enemies
            for (const auto& other : otherEnemies) {
                if (isTooClose(pos, other, config.minEnemyEnemyDistance)) {
                    return false;
                }
            }

            // Must be reachable from player
            if (!isReachable(map, playerPos, pos)) return false;

            return true;
        }

        bool Generator::isValidChestSpawn(
            const GeneratedMap& map,
            const Position& pos,
            const Position& playerPos,
            const std::vector<Position>& enemies,
            const Config& config) {

            // Must be floor tile
            if (map.getTile(pos.x, pos.y) != TileType::FLOOR) return false;

            // Not too close to player
            if (isTooClose(pos, playerPos, config.minPlayerChestDistance)) return false;

            // Not in safe zone
            if (isInSafeZone(pos, playerPos, config.playerSafeZoneRadius)) return false;

            return true;
        }

        bool Generator::isValidGoalSpawn(
            const GeneratedMap& map,
            const Position& pos,
            const Position& playerPos,
            const Config& config) {

            // Must be floor tile
            if (map.getTile(pos.x, pos.y) != TileType::FLOOR) return false;

            // Should be far from player
            if (config.ensureGoalIsFar) {
                if (manhattanDistance(pos, playerPos) < config.minPlayerGoalDistance) {
                    return false;
                }
            }

            // CRITICAL: Must be reachable from player
            if (!isReachable(map, playerPos, pos)) return false;

            return true;
        }

        // ============================================================================
        // ENTITY PLACEMENT - Main Function
        // ============================================================================

        bool Generator::placeEntities(GeneratedMap& map, const Config& config) {
            std::cout << "[MapGen] Placing entities with constraints...\n";

            // Collect all walkable tiles
            std::vector<Position> walkable;
            for (int y = 1; y < map.height - 1; y++) {
                for (int x = 1; x < map.width - 1; x++) {
                    if (map.getTile(x, y) == TileType::FLOOR) {
                        walkable.push_back(Position(x, y));
                    }
                }
            }

            if (walkable.size() < 10) {
                std::cout << "[MapGen] WARNING: Not enough walkable tiles!\n";
                return false;
            }

            std::cout << "[MapGen] Found " << walkable.size() << " walkable tiles\n";

            // Shuffle for randomness
            shuffle(walkable);

            // ========================================
            // PLACE PLAYER
            // ========================================
            bool playerPlaced = false;
            for (int attempt = 0; attempt < config.maxPlacementAttempts; attempt++) {
                if (attempt >= static_cast<int>(walkable.size())) break;

                Position candidate = walkable[attempt];

                if (isValidPlayerSpawn(map, candidate, {}, config)) {
                    map.playerSpawn = candidate;
                    playerPlaced = true;
                    std::cout << "[MapGen] Player spawn: (" << candidate.x << ", " << candidate.y << ")\n";
                    break;
                }
            }

            if (!playerPlaced) {
                std::cout << "[MapGen] WARNING: Failed to place player!\n";
                return false;
            }

            // ========================================
            // PLACE ENEMIES
            // ========================================
            int enemyCount = randInt(config.minEnemies, config.maxEnemies);
            std::vector<Position> placedEnemies;

            std::cout << "[MapGen] Placing " << enemyCount << " enemies:\n";

            for (int i = 0; i < enemyCount; i++) {
                bool enemyPlaced = false;

                for (int attempt = 0; attempt < config.maxPlacementAttempts; attempt++) {
                    Position candidate = walkable[randInt(0, static_cast<int>(walkable.size()) - 1)];

                    if (isValidEnemySpawn(map, candidate, map.playerSpawn, placedEnemies, config)) {
                        map.enemySpawns.push_back(candidate);
                        placedEnemies.push_back(candidate);
                        enemyPlaced = true;
                        std::cout << "[MapGen]   Enemy " << (i + 1) << ": (" << candidate.x << ", " << candidate.y << ")\n";
                        break;
                    }
                }

                if (!enemyPlaced) {
                    std::cout << "[MapGen]   Enemy " << (i + 1) << ": Failed to place\n";
                }
            }

            // ========================================
            // PLACE CHESTS
            // ========================================
            int chestCount = randInt(config.minChests, config.maxChests);

            std::cout << "[MapGen] Placing " << chestCount << " chests:\n";

            for (int i = 0; i < chestCount; i++) {
                bool chestPlaced = false;

                for (int attempt = 0; attempt < config.maxPlacementAttempts; attempt++) {
                    Position candidate = walkable[randInt(0, static_cast<int>(walkable.size()) - 1)];

                    if (isValidChestSpawn(map, candidate, map.playerSpawn, placedEnemies, config)) {
                        map.chestSpawns.push_back(candidate);
                        chestPlaced = true;
                        std::cout << "[MapGen]   Chest " << (i + 1) << ": (" << candidate.x << ", " << candidate.y << ")\n";
                        break;
                    }
                }

                if (!chestPlaced) {
                    std::cout << "[MapGen]   Chest " << (i + 1) << ": Failed to place\n";
                }
            }

            // ========================================
            // PLACE GOAL (farthest REACHABLE position)
            // ========================================
            std::cout << "[MapGen] Placing goal (far from player, must be reachable):\n";

            bool goalPlaced = false;

            // Get all tiles reachable from player spawn
            std::vector<Position> reachable = getReachableTiles(map, map.playerSpawn);
            std::cout << "[MapGen]   Reachable tiles from player: " << reachable.size() << "\n";

            if (reachable.empty()) {
                std::cout << "[MapGen] ERROR: No reachable tiles from player!\n";
                map.goalSpawn = map.playerSpawn;
                return false;
            }

            // Sort reachable tiles by distance from player (farthest first)
            std::vector<std::pair<int, Position>> candidateGoals;
            for (const auto& pos : reachable) {
                int dist = manhattanDistance(map.playerSpawn, pos);
                candidateGoals.push_back({ dist, pos });
            }

            std::sort(candidateGoals.begin(), candidateGoals.end(),
                [](const auto& a, const auto& b) { return a.first > b.first; });

            // Try to find a valid goal position
            for (const auto& [dist, candidate] : candidateGoals) {
                // Skip player spawn position
                if (candidate == map.playerSpawn) continue;

                // Skip positions too close (if configured)
                if (config.ensureGoalIsFar && dist < config.minPlayerGoalDistance) {
                    continue;
                }

                // Skip enemy positions
                bool isEnemyPos = false;
                for (const auto& enemy : map.enemySpawns) {
                    if (enemy == candidate) {
                        isEnemyPos = true;
                        break;
                    }
                }
                if (isEnemyPos) continue;

                // Skip chest positions
                bool isChestPos = false;
                for (const auto& chest : map.chestSpawns) {
                    if (chest == candidate) {
                        isChestPos = true;
                        break;
                    }
                }
                if (isChestPos) continue;

                // Valid goal position found!
                map.goalSpawn = candidate;
                goalPlaced = true;
                std::cout << "[MapGen]   Goal placed at: (" << candidate.x << ", " << candidate.y
                    << ") [distance: " << dist << "]\n";
                break;
            }

            // Fallback: if no position meets distance requirement, use farthest reachable tile
            if (!goalPlaced && !candidateGoals.empty()) {
                std::cout << "[MapGen]   WARNING: No position meets distance requirement, using farthest reachable\n";

                for (const auto& [dist, candidate] : candidateGoals) {
                    if (candidate == map.playerSpawn) continue;

                    map.goalSpawn = candidate;
                    goalPlaced = true;
                    std::cout << "[MapGen]   Goal placed at: (" << candidate.x << ", " << candidate.y
                        << ") [distance: " << dist << "] (fallback)\n";
                    break;
                }
            }

            if (!goalPlaced) {
                std::cout << "[MapGen] CRITICAL: Failed to place goal! Using player spawn as fallback.\n";
                map.goalSpawn = map.playerSpawn;
            }

            std::cout << "[MapGen] Entity placement complete!\n";
            return goalPlaced;
        }

        // ============================================================================
        // HELPER: Make Borders
        // ============================================================================

        void Generator::makeBordersWalls(GeneratedMap& map) {
            for (int x = 0; x < map.width; x++) {
                map.setTile(x, 0, TileType::WALL);
                map.setTile(x, map.height - 1, TileType::WALL);
            }

            for (int y = 0; y < map.height; y++) {
                map.setTile(0, y, TileType::WALL);
                map.setTile(map.width - 1, y, TileType::WALL);
            }
        }

        // ============================================================================
        // MAIN GENERATION FUNCTION
        // ============================================================================

        GeneratedMap Generator::generate(const Config& config) {
            std::cout << "\n========================================\n";
            std::cout << "[MapGen] Generating " << config.algorithm << " map\n";
            std::cout << "[MapGen] Size: " << config.width << "x" << config.height << "\n";
            std::cout << "========================================\n";

            GeneratedMap map(config.width, config.height);

            // Generate map layout
            if (config.algorithm == "rooms") {
                generateRooms(map, config);
            }
            else if (config.algorithm == "cellular") {
                generateCaves(map, config);
            }
            else if (config.algorithm == "open") {
                generateArena(map);
            }
            else {
                std::cout << "[MapGen] Unknown algorithm: " << config.algorithm << "\n";
                std::cout << "[MapGen] Using 'open' instead\n";
                generateArena(map);
            }

            // Ensure borders are walls
            makeBordersWalls(map);

            // Place entities with constraints
            if (!placeEntities(map, config)) {
                std::cout << "[MapGen] WARNING: Entity placement had issues\n";
            }

            std::cout << "[MapGen] Generation complete!\n";
            std::cout << "========================================\n\n";

            return map;
        }

        // ============================================================================
        // DEBUG: Print Map
        // ============================================================================

        void Generator::printMap(const GeneratedMap& map) {
            std::cout << "\n[MapGen] Map Preview:\n";
            std::cout << "W = Wall, . = Floor, P = Player, E = Enemy, C = Chest, G = Goal\n\n";

            for (int y = 0; y < map.height; y++) {
                for (int x = 0; x < map.width; x++) {
                    char c = '?';

                    // Check if entity spawn
                    Position pos(x, y);
                    if (pos == map.playerSpawn) {
                        c = 'P';
                    }
                    else if (pos == map.goalSpawn) {
                        c = 'G';
                    }
                    else {
                        bool isEnemy = false;
                        for (const auto& e : map.enemySpawns) {
                            if (e == pos) { c = 'E'; isEnemy = true; break; }
                        }
                        if (!isEnemy) {
                            for (const auto& ch : map.chestSpawns) {
                                if (ch == pos) { c = 'C'; break; }
                            }
                            if (c == '?') {
                                c = (map.getTile(x, y) == TileType::WALL) ? 'W' : '.';
                            }
                        }
                    }

                    std::cout << c;
                }
                std::cout << "\n";
            }
            std::cout << "\n";
        }

    } // namespace MapGen
} // namespace Framework