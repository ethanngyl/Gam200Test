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
  5. Play audio feedback for enemy movement

  The A* implementation uses a priority queue for efficient node selection
  and Manhattan distance as the heuristic. Paths are cached and only
  recalculated when necessary for performance.

  Enemy spawning places enemies at the furthest walkable tile from the player,
  ensuring challenging gameplay by maximizing initial distance.

 Copyright (C) 2026 DigiPen Institute of Technology.
 Reproduction or disclosure of this file or its contents
 without the prior written consent of DigiPen Institute of
 Technology is prohibited.
===============================================================================
 */

#include "Precompiled.h"
#include "Pathfinding.h"
#include "EntitySpawner.h"
#include "ECSEntityManager.h"
#include "MathABS.h"
#include "Turn.h"
#include "Audio/AudioSystem.h"
#include <queue>
#include <algorithm>
#include "GlobalPauseManager.h"


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
        // PathfindingSystem initialized
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
     * - Plays walking sound effects when enemies move
     */
    void PathfindingSystem::Update(float dt) {

        if (GlobalPause::IsPaused()) {
            return;
        }

        if (!entityManager) return;

        // Only update during enemy turn
        if (!IsEnemyTurn()) return;

        const Grid& grid = GetGrid();
        if (grid.cols <= 0 || grid.rows <= 0 || !grid.em) return;

        // Get global turn info for AP regeneration
        auto& globalTurn = Turn();
        static uint64_t lastEnemyTurnIndex = 0;

        // ========================================================================
        // SEQUENTIAL ENEMY MOVEMENT - Track which enemy is currently acting
        // ========================================================================
        static int currentEnemyIndex = 0;  // Which enemy's turn is it?
        static bool needsReset = true;     // Reset index at start of enemy turn

        // Check if this is a NEW enemy turn (regenerate AP for all enemies)
        if (globalTurn.turnIndex > lastEnemyTurnIndex) {
            for (Entity entity : entityManager->GetAllEntities()) {
                if (entityManager->HasComponent<EnemyAI>(entity) &&
                    entityManager->HasComponent<Health>(entity)  && 
                    entityManager->HasComponent<AP>(entity)) {
                    auto& hp = entityManager->GetComponent<Health>(entity);
					auto& stats = entityManager->GetComponent<AP>(entity);

                    // Skip dead enemies
                    if (hp.isDead || hp.currentHealth <= 0) continue;

                    // Regenerate AP (for attacks) and MP (for movement)
                    stats.actionPoints = stats.maxActionPoints;
                    if (entityManager->HasComponent<EnemyAI>(entity)) {
                        auto& enemyAI = entityManager->GetComponent<EnemyAI>(entity);
                        enemyAI.movePoints = enemyAI.maxMovePoints;
                    }
                }
            }

            lastEnemyTurnIndex = globalTurn.turnIndex;
            currentEnemyIndex = 0;  // Reset to first enemy
            needsReset = false;
        }

        // ========================================================================
        // COLLECT ALL LIVING ENEMIES
        // ========================================================================
        std::vector<Entity> livingEnemies;
        for (Entity entity : entityManager->GetAllEntities()) {
            if (!entityManager->HasComponent<EnemyAI>(entity)) continue;
            if (!entityManager->HasComponent<Transform>(entity)) continue;
            if (!entityManager->HasComponent<AP>(entity)) continue;
            if (!entityManager->HasComponent<Health>(entity)) continue;

            auto& hp = entityManager->GetComponent<Health>(entity);                    
            if (hp.currentHealth > 0 && !hp.isDead) {                                 
                livingEnemies.push_back(entity);
            }
        }

        if (livingEnemies.empty()) {
            LOG_WARN("EnemyTurn", "No living enemies, ending turn");
            EndEnemyTurn();
            return;
        }

        // ========================================================================
        // SEQUENTIAL LOGIC: Only update ONE enemy per frame
        // ========================================================================

        // Check if current enemy finished their turn
        if (currentEnemyIndex >= livingEnemies.size()) {
            // All enemies done, end turn
            EndEnemyTurn();
            currentEnemyIndex = 0;
            needsReset = true;
            return;
        }

        // Get current enemy
        Entity currentEnemy = livingEnemies[currentEnemyIndex];
        auto& ai = entityManager->GetComponent<EnemyAI>(currentEnemy);
        auto& transform = entityManager->GetComponent<Transform>(currentEnemy);
        auto& stats = entityManager->GetComponent<AP>(currentEnemy);
		//auto& hp = entityManager->GetComponent<Health>(currentEnemy);

        // CRITICAL FIX: Initialize delay when switching to a new enemy
        // This prevents all enemies from moving simultaneously on the first frame
        static int lastActiveEnemyIndex = -1;
        if (currentEnemyIndex != lastActiveEnemyIndex) {
            // Just switched to a new enemy - set initial delay
            ai.moveTimer = ai.moveDelay;
            lastActiveEnemyIndex = currentEnemyIndex;
            LOG_INFO("EnemyAI", "Now acting: Enemy %u (index %d/%zu)",
                currentEnemy.GetID(), currentEnemyIndex, livingEnemies.size() - 1);

            // Pan camera to this enemy
            if (graphicsSystem) {
                graphicsSystem->SetFollowTarget(currentEnemy);
                LOG_INFO("EnemyAI", "Camera now following Enemy %u", currentEnemy.GetID());
            }

            return;  // Wait one frame before acting
        }

        // Advance when both AP (attacks) and MP (movement) are exhausted
        if (stats.actionPoints <= 0 && ai.movePoints <= 0) {
            currentEnemyIndex++;  // Move to next enemy
            return;
        }

        // ========================================================================
        // UPDATE CURRENT ENEMY
        // ========================================================================

        // Lua sets blockMovement during ranged attack - skip movement, don't advance
        if (ai.blockMovement) {
            return;
        }

        // Update movement timer
        ai.moveTimer -= dt;
        if (ai.moveTimer > 0.0f) {
            return; // Still waiting for movement delay
        }

        // CRITICAL FIX: Find closest player dynamically instead of using hardcoded target
        // This ensures enemies always chase the actually closest player
        auto enemyTileOpt = WorldToTile(transform.position);
        if (!enemyTileOpt.has_value()) {
            currentEnemyIndex++;
            return;
        }
        GridCoord enemyTile = *enemyTileOpt;

        // Find all players (entities with AP + CircleCollider, but NOT EnemyAI)
        Entity closestPlayer{ INVALID_ENTITY };
        int closestDistance = 999999;
        GridCoord closestPlayerTile{ 0, 0 };

        for (Entity entity : entityManager->GetAllEntities()) {
            if (!entityManager->HasComponent<AP>(entity)) continue;
            if (!entityManager->HasComponent<CircleCollider>(entity)) continue;
            if (entityManager->HasComponent<EnemyAI>(entity)) continue;  // Skip enemies
            if (!entityManager->HasComponent<Transform>(entity)) continue;

            // Skip dead players - they are no longer valid targets
            if (entityManager->HasComponent<Health>(entity)) {
                const auto& health = entityManager->GetComponent<Health>(entity);
                if (health.isDead) {
                    continue;  // Dead player - don't target
                }
            }

            // This is a player - check distance
            auto& playerTransform = entityManager->GetComponent<Transform>(entity);
            auto playerTileOpt = WorldToTile(playerTransform.position);
            if (!playerTileOpt.has_value()) continue;

            GridCoord playerTile = *playerTileOpt;
            int distance = Heuristic(enemyTile, playerTile);

            if (distance < closestDistance) {
                closestDistance = distance;
                closestPlayer = entity;
                closestPlayerTile = playerTile;
            }
        }

        // Validate we found a player
        if (closestPlayer.GetID() == INVALID_ENTITY) {
            LOG_WARN("EnemyAI", "Enemy %u: No players found", currentEnemy.GetID());
            stats.actionPoints = 0;
            currentEnemyIndex++;
            return;
        }

        // Update target if it changed
        if (ai.targetEntity.GetID() != closestPlayer.GetID()) {
            LOG_INFO("EnemyAI", "Enemy %u retargeting: %u -> %u (distance: %d)",
                currentEnemy.GetID(), ai.targetEntity.GetID(), closestPlayer.GetID(), closestDistance);
            ai.targetEntity = closestPlayer;
            ai.currentPath.clear();  // Clear old path when target changes
        }

        GridCoord targetTile = closestPlayerTile;

        // Check if adjacent to target (can attack)
        int distance = Heuristic(enemyTile, targetTile);
        if (distance == 1) {
            // Check if we have enough AP to attack
            const int attackAPCost = 2;
            if (stats.actionPoints < attackAPCost) {
                // Not enough AP to attack, move to next enemy
                LOG_INFO("EnemyAI", "Enemy %u adjacent but only has %d AP (need %d to attack)",
                    currentEnemy.GetID(), stats.actionPoints, attackAPCost);
                stats.actionPoints = 0;
                currentEnemyIndex++;
                return;
            }

            // ========================================================================
            // PLAY ENEMY ATTACK SOUND EFFECT
            // ========================================================================
            if (audioSystem) {
                audioSystem->PlaySound("dmgb", false);  // Play damage sound
            }

            LOG_INFO("EnemyAI", "Enemy %u ATTACKING Player %u", currentEnemy.GetID(), closestPlayer.GetID());

            // ATTACK! Consume 2 AP (attack cost)
            stats.actionPoints -= attackAPCost;
            ai.moveTimer = ai.moveDelay;

            // Deal damage to target
            if (entityManager->HasComponent<Health>(ai.targetEntity)) {
                auto& targetHp = entityManager->GetComponent<Health>(ai.targetEntity);
				targetHp.TakeDamage(1); // Flat 1 damage for now

                // ========================================================================
                // PLAY TAKE DAMAGE SOUND EFFECT (Player gets hit)
                // ========================================================================
                if (audioSystem && !targetHp.isDead) {
                    audioSystem->PlaySound("takedmg", false);  // Play take damage sound
                }

                // Target hit

                if (targetHp.currentHealth <= 0) {
                    targetHp.currentHealth = 0;
                    targetHp.isDead = true;
                    LOG_ERROR("Combat", "Player %u DEFEATED!", ai.targetEntity.GetID());

                    // Clear tile occupancy and destroy dead player entity
                    if (entityManager->HasComponent<Transform>(ai.targetEntity)) {
                        auto& deadTransform = entityManager->GetComponent<Transform>(ai.targetEntity);
                        auto deadTileOpt = WorldToTile(deadTransform.position);
                        if (deadTileOpt.has_value()) {
                            SetOccupant(deadTileOpt.value(), Entity{ INVALID_ENTITY });
                            LOG_INFO("Combat", "  -> Cleared tile occupancy for dead player at (%d, %d)",
                                deadTileOpt.value().x, deadTileOpt.value().y);
                        }
                    }

                    // Destroy the dead player entity
                    LOG_WARN("Combat", "  -> Destroying dead player entity %u", ai.targetEntity.GetID());
                    entityManager->DestroyEntity(ai.targetEntity);

                    // Check if ALL players are dead before triggering game over
                    bool allPlayersDead = true;
                    for (Entity entity : entityManager->GetAllEntities()) {
                        if (!entityManager->HasComponent<AP>(entity)) continue;
                        if (!entityManager->HasComponent<CircleCollider>(entity)) continue;
                        if (entityManager->HasComponent<EnemyAI>(entity)) continue;  // Skip enemies
                        if (!entityManager->HasComponent<Health>(entity)) continue;

                        auto& playerHp = entityManager->GetComponent<Health>(entity);
                        if (playerHp.currentHealth > 0 && !playerHp.isDead) {
                            allPlayersDead = false;
                            break;
                        }
                    }

                    if (allPlayersDead) {
                        GSM_SetNextState(LEVEL_END);
                        LOG_ERROR("Combat", "ALL PLAYERS DEFEATED - GAME OVER!");
                    }
                }
            }

            // If out of AP, move to next enemy
            if (stats.actionPoints <= 0) {
                currentEnemyIndex++;
            }
            return;
        }

        // ========================================================================
        // PATHFINDING: Calculate path to target (need MP to move)
        // ========================================================================
        if (ai.movePoints <= 0) {
            // No MP left, can't move - advance to next enemy
            currentEnemyIndex++;
            return;
        }

        ai.currentPath = FindPath(enemyTile, targetTile, grid);

        if (ai.currentPath.empty()) {
            LOG_WARN("EnemyAI", "Enemy %u: No path found!", currentEnemy.GetID());
            ai.movePoints = 0;
            currentEnemyIndex++;
            return;
        }

        // Skip the starting position if it's in the path
        if (ai.currentPath[0].x == enemyTile.x && ai.currentPath[0].y == enemyTile.y) {
            ai.pathIndex = 1;
        }
        else {
            ai.pathIndex = 0;
        }

        // Path found

        // ========================================================================
        // MOVEMENT: Move to next tile in path
        // ========================================================================
        if (ai.pathIndex < ai.currentPath.size()) {
            GridCoord nextTile = ai.currentPath[ai.pathIndex];

            // Check if tile is passable (allow moving onto target's tile)
            const bool passable =
                (nextTile.x == targetTile.x && nextTile.y == targetTile.y) ||
                IsWalkable(nextTile);

            if (!passable) {
                LOG_WARN("EnemyAI", "Enemy %u: Next tile (%d,%d) blocked!",
                    currentEnemy.GetID(), nextTile.x, nextTile.y);
                ai.currentPath.clear();
                ai.movePoints = 0;
                currentEnemyIndex++;
                return;
            }

            // Update occupancy
            SetOccupant(enemyTile, Entity{ INVALID_ENTITY });

            // Move enemy
            transform.position = TileToWorld(nextTile);
            SetOccupant(nextTile, currentEnemy);

            // ========================================================================
            // PLAY WALKING SOUND EFFECT
            // ========================================================================
            if (audioSystem) {
                audioSystem->PlaySound("walk1", false);
            }

            // Update state - consume MP for movement (AP is for attacks only)
            ai.moveTimer = ai.moveDelay;
            ai.pathIndex++;
            ai.movePoints--;

            // Enemy moved - advance if no MP left
            if (ai.movePoints <= 0) {
                currentEnemyIndex++;
            }
        }
        else {
            // Path exhausted
            ai.movePoints = 0;
            currentEnemyIndex++;
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
    * @param goal Goal coordinate (allowed even if occupied, as it's the destination)
    * @return Vector of walkable neighbor coordinates (up to 4)
    *
    * Checks tiles in 4 directions (up, down, left, right).
    * Only returns neighbors that are within bounds and walkable.
    * The goal tile is allowed even if occupied (it's the target destination).
    */
    std::vector<GridCoord> PathfindingSystem::GetNeighbors(const GridCoord& coord, const Grid& grid, const GridCoord& goal) {
        std::vector<GridCoord> neighbors;
        neighbors.reserve(4);

        const int dx[] = { 0, 0, -1, 1 };
        const int dy[] = { -1, 1, 0, 0 };

        for (int i = 0; i < 4; ++i) {
            GridCoord neighbor{ coord.x + dx[i], coord.y + dy[i] };

            if (!grid.InBounds(neighbor.x, neighbor.y)) {
                continue;
            }

            // CRITICAL FIX: Check BOTH blocked AND occupied status
            // Enemies should NOT path onto tiles occupied by players or other entities
            Entity tileEntity = grid.TileAt(neighbor.x, neighbor.y);
            if (tileEntity.GetID() == INVALID_ENTITY) {
                continue;
            }

            if (!grid.em->HasComponent<GridTiles>(tileEntity)) {
                continue;
            }

            const auto& gridTile = grid.em->GetComponent<GridTiles>(tileEntity);

            // Check if this is the goal tile - allow it even if occupied
            bool isGoal = (neighbor.x == goal.x && neighbor.y == goal.y);

            // Check both physical blocking AND occupancy
            // Allow the goal tile even if occupied (it's the target destination)
            if (!gridTile.blocked && (gridTile.occupant.GetID() == INVALID_ENTITY || isGoal)) {
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

            for (const GridCoord& neighbor : GetNeighbors(current, grid, goal)) {
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
    * @param playerEntity Player�s entity reference
    * @param entityManager Entity manager for component access
    * @param entitySpawner Entity spawner for creating enemies
    * @return Enemy entity with AI component, or INVALID_ENTITY if spawn fails
    *
    * Finds the player�s tile, checks all walkable tiles for the one farthest away
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
            // Error: EntityManager or EntitySpawner is null
            return Entity{ INVALID_ENTITY };
        }

        if (!entityManager->HasComponent<Transform>(playerEntity)) {
            // Error: Player entity has no Transform component
            return Entity{ INVALID_ENTITY };
        }

        // Get grid
        const Grid& grid = GetGrid();
        if (grid.cols <= 0 || grid.rows <= 0) {
            // Error: Grid not initialized
            return Entity{ INVALID_ENTITY };
        }

        // Get player's current tile position
        auto& playerTransform = entityManager->GetComponent<Transform>(playerEntity);
        auto playerTileOpt = WorldToTile(playerTransform.position);

        if (!playerTileOpt.has_value()) {
            // Error: Player is not on a valid grid tile
            return Entity{ INVALID_ENTITY };
        }

        GridCoord playerTile = *playerTileOpt;
        // Player tile position retrieved

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
            // Error: No walkable tiles found on grid
            return Entity{ INVALID_ENTITY };
        }

        // Furthest tile found

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
        ai.moveDelay = 1.2f;  // 1.2 seconds between moves

        // Mark the tile as occupied
        SetOccupant(furthestTile, enemy);

        // Enemy spawned successfully

        return enemy;
    }

} // namespace Framework