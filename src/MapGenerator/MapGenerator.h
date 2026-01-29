#pragma once
/**
===============================================================================
 MapGenerator.h - Complete Professional Version
 Includes: Overlap detection, corridors, smart entity placement
===============================================================================
*/

#pragma once
#include <vector>
#include <string>
#include <random>

namespace Framework {
    namespace MapGen {

        struct Position {
            int x, y;
            Position(int _x = 0, int _y = 0) : x(_x), y(_y) {}
            bool operator==(const Position& other) const {
                return x == other.x && y == other.y;
            }
        };

        enum class TileType {
            WALL,
            FLOOR
        };

        struct Config {
            int width = 30;
            int height = 20;
            std::string algorithm = "rooms";

            // Rooms
            int minRoomSize = 4;
            int maxRoomSize = 10;
            int maxRooms = 8;

            // Cellular
            float wallChance = 0.45f;
            int smoothPasses = 4;

            // Entity counts
            int minEnemies = 2;
            int maxEnemies = 5;
            int minChests = 1;
            int maxChests = 3;

            // Placement constraints
            int minPlayerEnemyDistance = 5;
            int minEnemyEnemyDistance = 3;
            int minPlayerChestDistance = 2;
            int playerSafeZoneRadius = 2;
            bool ensureGoalIsFar = true;
            int minPlayerGoalDistance = 10;
            int maxPlacementAttempts = 100;
        };

        struct GeneratedMap {
            int width, height;
            std::vector<std::vector<TileType>> tiles;

            // Entity spawns
            Position playerSpawn;
            Position goalSpawn;
            std::vector<Position> enemySpawns;
            std::vector<Position> chestSpawns;

            GeneratedMap(int w = 0, int h = 0);
            bool isValid(int x, int y) const;
            TileType getTile(int x, int y) const;
            void setTile(int x, int y, TileType type);
        };

        class Generator {
        public:
            Generator();
            explicit Generator(unsigned int seed);
            ~Generator();

            GeneratedMap generate(const Config& config);
            static void printMap(const GeneratedMap& map);

        private:
            std::mt19937 rng;

            int randInt(int min, int max);
            float randFloat();
            bool randBool(float probability = 0.5f);

            template<typename T>
            void shuffle(std::vector<T>& vec);

            // Room structure
            struct Room {
                int x, y, width, height;
                Room(int _x = 0, int _y = 0, int _w = 0, int _h = 0)
                    : x(_x), y(_y), width(_w), height(_h) {
                }
                Position center() const {
                    return Position(x + width / 2, y + height / 2);
                }
            };

            // Room generation
            bool roomsOverlap(const Room& r1, const Room& r2) const;
            void carveRoom(GeneratedMap& map, const Room& room);
            void carveHorizontalCorridor(GeneratedMap& map, int x1, int x2, int y);
            void carveVerticalCorridor(GeneratedMap& map, int y1, int y2, int x);
            void createCorridor(GeneratedMap& map, const Room& r1, const Room& r2);
            void generateRooms(GeneratedMap& map, const Config& config);

            // Cellular
            int countAdjacentWalls(const GeneratedMap& map, int x, int y) const;
            void generateCaves(GeneratedMap& map, const Config& config);

            // Arena
            void generateArena(GeneratedMap& map);

            // Entity placement
            bool placeEntities(GeneratedMap& map, const Config& config);
            void makeBordersWalls(GeneratedMap& map);

            // Constraints
            int manhattanDistance(const Position& a, const Position& b) const;
            bool isTooClose(const Position& a, const Position& b, int minDistance) const;
            bool isInSafeZone(const Position& pos, const Position& center, int radius) const;
            bool hasWalkableNeighbors(const GeneratedMap& map, const Position& pos, int radius = 1) const;

            // Validation
            bool isValidPlayerSpawn(const GeneratedMap& map, const Position& pos,
                const std::vector<Position>& enemies, const Config& config);
            bool isValidEnemySpawn(const GeneratedMap& map, const Position& pos,
                const Position& playerPos,
                const std::vector<Position>& otherEnemies,
                const Config& config);
            bool isValidChestSpawn(const GeneratedMap& map, const Position& pos,
                const Position& playerPos,
                const std::vector<Position>& enemies,
                const Config& config);
            bool isValidGoalSpawn(const GeneratedMap& map, const Position& pos,
                const Position& playerPos, const Config& config);
        };

    } // namespace MapGen
} // namespace Framework