#pragma once

/**
===============================================================================
 File:           Pathfinding.h
 Author:         PADILLA CARL JAMESON Z. 
 Email:          c.padilla@digipen.edu
 Date:           2025/11/02
 Contribution:   
 ------------------------------------------------------------------------------

  Brief:
  - This system makes enemies chase the player intelligently. Enemies use A*
    pathfinding to navigate around obstacles on a grid. When the player moves
    to a new tile, enemies automatically recalculate their path to follow.

  Key features:
  - A* algorithm with "Manhattan" distance heuristic for optimal pathfinding
  - Real-time target tracking with automatic path recalculation
  - Grid-based movement with tile occupancy management
  - EnemyAI component stores per-enemy pathfinding state
  - Spawning enemies at strategic positions (furthest from player)
  - Audio feedback for enemy movement

  The system operates on entities with both EnemyAI and Transform components,
  using a timer-based movement system for smooth, turn-based navigation.
===============================================================================
 */

#include "Grid/GridECS.h"
#include "Grid/Grid.h"
#include "Vector2D.h"
#include "ECSEntity.h"
#include "Component.h"
#include <optional>
#include <vector>

namespace Framework {

    class EntityManager;
    class EntitySpawner;
    class AudioSystem;  // Forward declaration

    /**
    * @brief A* pathfinding node for grid-based pathfinding
    */
    struct PathNode {
        GridCoord coord;
        int gCost;  // Distance from start
        int hCost;  // Heuristic distance to goal
        int fCost() const { return gCost + hCost; }
        GridCoord parent;
        bool hasParent;

        PathNode() : coord{ 0, 0 }, gCost(0), hCost(0), parent{ 0, 0 }, hasParent(false) {}
        PathNode(GridCoord c) : coord(c), gCost(0), hCost(0), parent{ 0, 0 }, hasParent(false) {}
    };

    /**
    * @brief Enemy AI component - stores pathfinding state per enemy
    */
    struct EnemyAI : public Component<EnemyAI> {
        std::vector<GridCoord> currentPath;  // Path to follow
        size_t pathIndex;                     // Current position in path
        float moveTimer;                      // Time until next move
        float moveDelay;                      // Delay between moves (seconds)
        Entity targetEntity;                  // Who to chase (usually player)
        bool hasReachedTarget;                // Flag to prevent spam logs

        EnemyAI()
            : pathIndex(0)
            , moveTimer(0.0f)
            , moveDelay(0.7f)
            , targetEntity(INVALID_ENTITY)
            , hasReachedTarget(false)
        {
        }
    };

    /**
    * @brief system that manages enemy AI pathfinding nav
    */
    class PathfindingSystem : public EngineSystem {
    public:
        PathfindingSystem() : entityManager(nullptr), audioSystem(nullptr) {}
        ~PathfindingSystem() = default;

        void Initialize() override;
        void Update(float dt) override;
        void SendEngineMessage(Message* msg) override;

        void SetEntityManager(EntityManager* em) { entityManager = em; }
        void SetAudioSystem(AudioSystem* audio) { audioSystem = audio; }

        /**
        * @brief Spawn an enemy at the furthest walkable tile from player
        */
        static Entity SpawnEnemyFurthestFromPlayer(
            Entity playerEntity,
            EntityManager* entityManager,
            EntitySpawner* entitySpawner);

    private:
        EntityManager* entityManager;
        AudioSystem* audioSystem;  // Audio system for enemy walking sounds

        /**
        * @brief Calculate A* path from start to goal
        */
        static std::vector<GridCoord> FindPath(
            const GridCoord& start,
            const GridCoord& goal,
            const Grid& grid);

    private:
        EntityManager* entityManager;

        /**
        * @brief Calculate Manhattan distance heuristic
        */
        static int Heuristic(const GridCoord& a, const GridCoord& b);

        /**
        * @brief Get valid neighboring tiles (4-directional)
        */
        static std::vector<GridCoord> GetNeighbors(const GridCoord& coord, const Grid& grid);

        /**
        * @brief Reconstruct path from A* search
        */
        static std::vector<GridCoord> ReconstructPath(
            const std::vector<std::vector<PathNode>>& nodes,
            const GridCoord& start,
            const GridCoord& goal);
    };

} // namespace Framework