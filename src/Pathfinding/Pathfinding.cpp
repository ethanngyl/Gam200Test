/**
===============================================================================
 File:           Pathfinding.cpp
 Author:         PADILLA CARL JAMESON Z.
 Email:          c.padilla@digipen.edu
 Date:           2025/11/02
 Contribution:   100%
 ------------------------------------------------------------------------------

  Design notes:
  Implementation of A* pathfinding system for grid-based enemy AI. This file
  contains the core pathfinding logic and enemy movement update loop.

  The Update() method is the heart of the system, running every frame to:
  1. Check if target (player) has moved to a different tile
  2. Recalculate path if target moved or path is exhausted
  3. Move enemies along their current path based on movement timer
  4. Handle tile occupancy to prevent overlap

  The A* implementation uses a priority queue for efficient node selection
  and Manhattan distance as the heuristic. Paths are cached and only
  recalculated when necessary for performance.

  Enemy spawning places enemies at the furthest walkable tile from the player,
  ensuring challenging gameplay by maximizing initial distance.
===============================================================================
 */

#include "Precompiled.h"
#include "Pathfinding.h"
#include "EntitySpawner.h"
#include "ECSEntityManager.h"
#include "MathABS.h"
#include "Turn.h"
#include <queue>
#include <algorithm>

namespace Framework {
    // ============================================================================
    // SYSTEM LIFECYCLE
    // ============================================================================

        /**
     * @brief Initializes the pathfinding system
     *
     * Called once during system startup. Currently logs initialization status.
     */
    void PathfindingSystem::Initialize() {
        std::cout << "[PathfindingSystem] Initialized\n";
    }

    /**
     * @brief Updates all enemies with AI pathfinding each frame
     * @param dt Delta time in seconds since last frame
     *
     * Main update loop that:
     * - Iterates through all entities with EnemyAI components
     * - Detects when player moves to a new tile
     * - Recalculates paths when needed using A*
     * - Moves enemies along their path based on timer
     * - Updates tile occupancy to prevent overlaps
     * - Stops movement when enemy is adjacent to target
     */
    void PathfindingSystem::Update(float dt) {
        if (!entityManager) return;
        if (!IsEnemyTurn()) return;  

        const Grid& grid = GetGrid();
        if (grid.cols <= 0 || grid.rows <= 0 || !grid.em) return;

        bool enemyMoved = false;

        // Update all enemies with AI component
        for (Entity entity : entityManager->GetAllEntities()) {
            if (!entityManager->HasComponent<EnemyAI>(entity)) continue;
            if (!entityManager->HasComponent<Transform>(entity)) continue;

            auto& ai = entityManager->GetComponent<EnemyAI>(entity);
            auto& transform = entityManager->GetComponent<Transform>(entity);

            // Validate target
            if (ai.targetEntity.GetID() == INVALID_ENTITY) continue;
            if (!entityManager->HasComponent<Transform>(ai.targetEntity)) continue;

            // Get current and target positions
            auto enemyTileOpt = WorldToTile(transform.position);
            if (!enemyTileOpt.has_value()) continue;
            GridCoord enemyTile = *enemyTileOpt;

            auto& targetTransform = entityManager->GetComponent<Transform>(ai.targetEntity);
            auto targetTileOpt = WorldToTile(targetTransform.position);
            if (!targetTileOpt.has_value()) continue;
            GridCoord targetTile = *targetTileOpt;

            // Check if adjacent to target (goal reached)
            int distance = Heuristic(enemyTile, targetTile);
            if (distance == 1) {
                if (!ai.hasReachedTarget) {
                    std::cout << "[EnemyAI] Enemy " << entity.GetID() << " has reached player!\n";
                    ai.hasReachedTarget = true;
                }
                continue;  // Stop moving when adjacent
            }
            ai.hasReachedTarget = false;

            // *** ALWAYS recalculate path every turn ***
        // This ensures the enemy tracks the player's CURRENT position
            std::cout << "[EnemyAI] Recalculating path from (" << enemyTile.x << "," << enemyTile.y
                << ") to player at (" << targetTile.x << "," << targetTile.y << ")\n";

            ai.currentPath = FindPath(enemyTile, targetTile, grid);
        //    ai.pathIndex = 0;

            if (ai.currentPath.empty()) {
                std::cout << "[EnemyAI] No path found for Enemy " << entity.GetID() << "\n";
                EndEnemyTurn();
                return;
            }

            // >>> FIX 1: skip start if present
            if (ai.currentPath[0].x == enemyTile.x && ai.currentPath[0].y == enemyTile.y) {
                ai.pathIndex = 1;
            }
            else {
                ai.pathIndex = 0;
            }

            std::cout << "[EnemyAI] New path has " << ai.currentPath.size() << " tiles\n";

            //*** this follows the original set path first, reaches the end goal then recalculates when path runs out ***
           /* if (ai.currentPath.empty() || ai.pathIndex >= ai.currentPath.size()) {
                ai.currentPath = FindPath(enemyTile, targetTile, grid);
                ai.pathIndex = 0;
            }*/

            //// Update movement timer
            ai.moveTimer -= dt;
            if (ai.moveTimer > 0.0f) continue;

            // Move to next tile in path
            if (ai.pathIndex < ai.currentPath.size()) {
                GridCoord nextTile = ai.currentPath[ai.pathIndex];

                // >>> allow stepping onto the goal tile
                const bool passable =
                    (nextTile.x == targetTile.x && nextTile.y == targetTile.y) || IsWalkable(nextTile);
                if (!passable) {
                    ai.currentPath.clear();
                    continue;
                }

                SetOccupant(enemyTile, Entity{ INVALID_ENTITY });
                transform.position = TileToWorld(nextTile);
                SetOccupant(nextTile, entity);

                ai.moveTimer = ai.moveDelay;
                ai.pathIndex++;
                enemyMoved = true;
                EndEnemyTurn();
                return;
            }
        }
    }

    /**
    * @brief Handles engine messages (currently unused)
    * @param msg Pointer to message to process
    */

    void PathfindingSystem::SendEngineMessage(Message* msg) {
        (void)msg;
    }

    // ============================================================================
    // HELPER FUNCTIONS
    // ============================================================================


    /**
    * @brief Calculates Manhattan distance between two grid coordinates
    * @param a First grid coordinate
    * @param b Second grid coordinate
    * @return Manhattan distance (sum of absolute differences in x and y)
    *
    * Manhattan distance is the sum of horizontal and vertical distances.
    * Used as the heuristic for A* pathfinding on a 4-directional grid.
    */
    int PathfindingSystem::Heuristic(const GridCoord& a, const GridCoord& b) {
        return Abs(a.x - b.x) + Abs(a.y - b.y);
    }


    /**
    * @brief Gets all valid walkable neighboring tiles
    * @param coord Center coordinate to find neighbors of
    * @param grid Grid reference for bounds and walkability checking
    * @return Vector of walkable neighbor coordinates (up to 4)
    *
    * Checks tiles in 4 directions (up, down, left, right).
    * Only returns neighbors that are within bounds and walkable.
    */
    std::vector<GridCoord> PathfindingSystem::GetNeighbors(const GridCoord& coord, const Grid& grid) {
        std::vector<GridCoord> neighbors;
        neighbors.reserve(4);

        const int dx[] = { 0, 0, -1, 1 };
        const int dy[] = { -1, 1, 0, 0 };

        for (int i = 0; i < 4; ++i) {
            GridCoord neighbor{ coord.x + dx[i], coord.y + dy[i] };

            /*if (grid.InBounds(neighbor.x, neighbor.y) && IsWalkable(neighbor)) {
                neighbors.push_back(neighbor);
            }*/

            if (!grid.InBounds(neighbor.x, neighbor.y)) {
                continue;
            }

            // *** KEY FIX: Just check if tile is blocked, ignore occupancy ***
            // We need to allow pathing to occupied tiles (like the player's position)
            Entity tileEntity = grid.TileAt(neighbor.x, neighbor.y);
            if (tileEntity.GetID() == INVALID_ENTITY) {
                continue;
            }

            if (!grid.em->HasComponent<GridTiles>(tileEntity)) {
                continue;
            }

            const auto& gridTile = grid.em->GetComponent<GridTiles>(tileEntity);

            // Only check if physically blocked, NOT if occupied
            if (!gridTile.blocked) {
                neighbors.push_back(neighbor);
            }
        }

        return neighbors;
    }

    /**
    * @brief Reconstructs the final path from A* search results
    * @param nodes 2D array of pathfinding nodes with parent information
    * @param start Starting grid coordinate
    * @param goal Goal grid coordinate
    * @return Vector of coordinates forming the path from start to goal
    *
    * Traces backwards through parent pointers from goal to start,
    * then reverses the result to get a forward-traversal path.
    */
    std::vector<GridCoord> PathfindingSystem::ReconstructPath(
        const std::vector<std::vector<PathNode>>& nodes,
        const GridCoord& start,
        const GridCoord& goal)
    {
        std::vector<GridCoord> path;
        GridCoord current = goal;

        while (!(current.x == start.x && current.y == start.y)) {
            path.push_back(current);
            const PathNode& node = nodes[current.y][current.x];

            if (!node.hasParent) break;
            current = node.parent;
        }

        std::reverse(path.begin(), path.end());
        return path;
    }


    // ============================================================================
    // A* PATHFINDING
    // ============================================================================

    /**
    * @brief Finds optimal path from start to goal using A* algorithm
    * @param start Starting grid coordinate
    * @param goal Target grid coordinate
    * @param grid Grid reference for bounds and neighbor checking
    * @return Vector of grid coordinates forming the optimal path
    *     
    * Implements classic A* pathfinding with:
    * - Priority queue for efficient lowest-cost node selection
    * - Manhattan distance heuristic for 4-directional movement
    * - Closed list to prevent revisiting processed nodes
    * - Early exit when goal is reached
    *
    * Returns empty vector if:
    * - Start or goal coordinates are out of bounds
    * - Start and goal are the same
    * - No valid path exists
    */
    std::vector<GridCoord> PathfindingSystem::FindPath(
        const GridCoord& start,
        const GridCoord& goal,
        const Grid& grid)
    {
        if (!grid.InBounds(start.x, start.y) || !grid.InBounds(goal.x, goal.y)) {
            return {};
        }

        if (start.x == goal.x && start.y == goal.y) {
            return {};
        }

        std::vector<std::vector<PathNode>> nodes(grid.rows, std::vector<PathNode>(grid.cols));
        for (int y = 0; y < grid.rows; ++y) {
            for (int x = 0; x < grid.cols; ++x) {
                nodes[y][x].coord = GridCoord{ x, y };
            }
        }

        auto compare = [&nodes](const GridCoord& a, const GridCoord& b) {
            return nodes[a.y][a.x].fCost() > nodes[b.y][b.x].fCost();
            };
        std::priority_queue<GridCoord, std::vector<GridCoord>, decltype(compare)> openList(compare);

        std::vector<std::vector<bool>> closedList(grid.rows, std::vector<bool>(grid.cols, false));

        nodes[start.y][start.x].gCost = 0;
        nodes[start.y][start.x].hCost = Heuristic(start, goal);
        openList.push(start);

        while (!openList.empty()) {
            GridCoord current = openList.top();
            openList.pop();

            if (closedList[current.y][current.x]) continue;
            closedList[current.y][current.x] = true;

            if (current.x == goal.x && current.y == goal.y) {
                return ReconstructPath(nodes, start, goal);
            }

            for (const GridCoord& neighbor : GetNeighbors(current, grid)) {
                if (closedList[neighbor.y][neighbor.x]) continue;

                int tentativeGCost = nodes[current.y][current.x].gCost + 1;

                PathNode& neighborNode = nodes[neighbor.y][neighbor.x];
                if (!neighborNode.hasParent || tentativeGCost < neighborNode.gCost) {
                    neighborNode.parent = current;
                    neighborNode.hasParent = true;
                    neighborNode.gCost = tentativeGCost;
                    neighborNode.hCost = Heuristic(neighbor, goal);
                    openList.push(neighbor);
                }
            }
        }

        return {};
    }

    // ============================================================================
    // ENEMY SPAWNING
    // ============================================================================

    /**
    * @brief Spawns an enemy at the farthest walkable tile from the player.
    * @param playerEntity Player’s entity reference
    * @param entityManager Entity manager for component access
    * @param entitySpawner Entity spawner for creating enemies
    * @return Enemy entity with AI component, or INVALID_ENTITY if spawn fails
    *
    * Finds the player’s tile, checks all walkable tiles for the one farthest away
    * (by Manhattan distance), then spawns an enemy there with an AI set to chase
    * the player. Returns INVALID_ENTITY if setup or spawn fails.
    */
    Entity PathfindingSystem::SpawnEnemyFurthestFromPlayer(
        Entity playerEntity,
        EntityManager* entityManager,
        EntitySpawner* entitySpawner)
    {
        // Validate inputs
        if (!entityManager || !entitySpawner) {
            std::cout << "[Pathfinding] Error: EntityManager or EntitySpawner is null!\n";
            return Entity{ INVALID_ENTITY };
        }

        if (!entityManager->HasComponent<Transform>(playerEntity)) {
            std::cout << "[Pathfinding] Error: Player entity has no Transform component!\n";
            return Entity{ INVALID_ENTITY };
        }

        // Get grid
        const Grid& grid = GetGrid();
        if (grid.cols <= 0 || grid.rows <= 0) {
            std::cout << "[Pathfinding] Error: Grid not initialized!\n";
            return Entity{ INVALID_ENTITY };
        }

        // Get player's current tile position
        auto& playerTransform = entityManager->GetComponent<Transform>(playerEntity);
        auto playerTileOpt = WorldToTile(playerTransform.position);

        if (!playerTileOpt.has_value()) {
            std::cout << "[Pathfinding] Error: Player is not on a valid grid tile!\n";
            return Entity{ INVALID_ENTITY };
        }

        GridCoord playerTile = *playerTileOpt;
        std::cout << "[Pathfinding] Player at tile (" << playerTile.x << "," << playerTile.y << ")\n";

        // Find the furthest walkable tile from the player
        int maxDistance = -1;
        GridCoord furthestTile{ 0, 0 };
        bool foundValidTile = false;

        for (int row = 0; row < grid.rows; ++row) {
            for (int col = 0; col < grid.cols; ++col) {
                GridCoord candidate{ col, row };

                // Must be walkable
                if (!IsWalkable(candidate)) {
                    continue;
                }

                // Calculate Manhattan distance
                int distance = Heuristic(candidate, playerTile);

                if (distance > maxDistance) {
                    maxDistance = distance;
                    furthestTile = candidate;
                    foundValidTile = true;
                }
            }
        }

        // Check if we found any valid tile
        if (!foundValidTile || maxDistance < 0) {
            std::cout << "[Pathfinding] Error: No walkable tiles found on grid!\n";
            return Entity{ INVALID_ENTITY };
        }

        std::cout << "[Pathfinding] Furthest tile: (" << furthestTile.x << "," << furthestTile.y
            << ") at distance " << maxDistance << "\n";

        // Convert tile coordinate to world position
        Vector2D spawnPos = TileToWorld(furthestTile);

        // Spawn the enemy using EntitySpawner
        Entity enemy = entitySpawner->SpawnEnemy(spawnPos);

        if (enemy.GetID() == INVALID_ENTITY) {
            std::cout << "[Pathfinding] Error: Failed to spawn enemy entity!\n";
            return Entity{ INVALID_ENTITY };
        }

        // Add AI component to make it pathfind
        entityManager->AddComponent<EnemyAI>(enemy);
        auto& ai = entityManager->GetComponent<EnemyAI>(enemy);
        ai.targetEntity = playerEntity;  // Chase the player
        ai.moveDelay = 0.7f;  // 0.7 seconds between moves

        // Mark the tile as occupied
        SetOccupant(furthestTile, enemy);

        std::cout << "[Pathfinding] Successfully spawned enemy at tile ("
            << furthestTile.x << "," << furthestTile.y << ") - will pathfind to player\n";

        return enemy;
    }

} // namespace Framework