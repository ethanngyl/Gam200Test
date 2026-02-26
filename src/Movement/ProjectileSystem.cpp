/**
===============================================================================
 File:           MovementSystem.cpp
 Author:         Josh Ong
 Email:          josh.o@digipen.edu
 Date:           2025-09-24
 Contribution:   100%
 ------------------------------------------------------------------------------
 Implementation of the MovementSystem class.

  Design notes:
  This file implements the entity movement logic. In its Update() loop, the
  system gets all relevant entities from the EntityManager. For each one, it
  reads WASD input from the InputSystem to create a direction vector.

  A key part of the implementation is normalizing the direction vector. This
  prevents entities from moving faster on the diagonal than they do along
  the cardinal axes. The final position is calculated using the entity's
  speed and delta time, and then clamped to the screen boundaries.

 Copyright (C) 2026 DigiPen Institute of Technology.
 Reproduction or disclosure of this file or its contents
 without the prior written consent of DigiPen Institute of
 Technology is prohibited.
===============================================================================
 */
#include "Precompiled.h"
#include "PlayerManager.h"
#include "ProjectileSystem.h"
#include "Collision/Quadtree.h"
#include "Grid/Grid.h"
#include "Grid/GridECS.h"
#include "Pathfinding/Pathfinding.h"
#include <algorithm>
namespace Framework
{
    /**
     * @brief Constructs the MovementSystem
     *
     * Initializes entity manager and input system pointers to nullptr.
     * These must be set via SetEntityManager() and SetInputSystem() before use.
     */
    ProjectileMovementSystem::ProjectileMovementSystem() : entityManager(nullptr), eventSystem(nullptr)
    {
    }

    /**
     * @brief Destructor for MovementSystem
     */
    ProjectileMovementSystem::~ProjectileMovementSystem()
    {
    }

    /**
     * @brief Initializes the movement system
     *
     * Prints initialization confirmation to console.
     */
    void ProjectileMovementSystem::Initialize()
    {
        std::cout << "Enemy Movement System: Initialized\n";
    }

    /**
     * @brief Updates movement for all entities with Movement components
     * @param dt Delta time since last frame in seconds
     *
     * For each entity with both Transform and Movement components:
     * 1. Reads WASD input to determine movement direction
     * 2. Normalizes diagonal movement to prevent faster diagonal speed
     * 3. Applies velocity-based movement: position += direction * speed * dt
     * 4. Clamps position to screen bounds [-1, 1] in both axes
     *
     * @note Does nothing if entityManager or inputSystem are not set
     */
    void ProjectileMovementSystem::Update(float dt)
    {
        if (!entityManager) return;

        std::vector<Entity> offScreenToDestroy;
        for (Entity entity : entityManager->GetAllEntities())
        {
            if (entityManager->HasComponent<Transform>(entity) &&
                entityManager->HasComponent<ProjectileMovement>(entity))
            {
                auto& transform = entityManager->GetComponent<Transform>(entity);
                auto& movement = entityManager->GetComponent<ProjectileMovement>(entity);

                // Apply movement
                if (!movement.blocked) {
                    transform.position += movement.direction * movement.moveSpeed * dt;
                }
                else {
                    transform.position -= movement.direction * movement.moveSpeed * dt;
                }

                // Check if projectile hit a wall (blocked tile or out-of-bounds)
                // Uses IsTileStaticBlocked instead of IsWalkable so projectiles
                // pass through entity-occupied tiles and only stop on walls.
                auto gridCoord = WorldToTile(transform.position);
                if (!gridCoord.has_value()) {
                    // Out of grid bounds - destroy
                    offScreenToDestroy.push_back(entity);
                }
                else {
                    // Only destroy on static walls, NOT on entity-occupied tiles
                    if (IsTileStaticBlocked(gridCoord.value())) {
                        offScreenToDestroy.push_back(entity);
                    }
                }

                // Fallback: destroy projectiles that go way off-screen
                if (transform.position.x > 50.0f || transform.position.x < -50.0f ||
                    transform.position.y > 50.0f || transform.position.y < -50.0f)
                {
                    offScreenToDestroy.push_back(entity);
                }
            }
        }
        // We run this BEFORE CheckProjectileEnemyCollisions
        // to clean up the entity list used by the collision checker.
        for (Entity entity : offScreenToDestroy)
        {
            if (entityManager->HasComponent<Transform>(entity))
            {
                entityManager->DestroyEntity(entity);
            }
        }

        //if statement to check if entity has enemy component, checks if projectile collides with enemy, sends a msg to enemy that its taking dmg
        // once health gone, destroy enemy entity plus projectile
        CheckProjectileEnemyCollisions();
    }

    /**
     * @brief Handles engine messages
     * @param message Pointer to the message to process
     *
     * Currently does not process any messages.
     */
    void ProjectileMovementSystem::SendEngineMessage(Message* message)
    {
        (void)message;
    }

    void ProjectileMovementSystem::CheckProjectileEnemyCollisions() {
        if (!entityManager || !eventSystem) return;

        // 1. MANUAL FILTERING
        std::vector<Framework::Entity> activeProjectiles;
        std::vector<Framework::Entity> activeEnemies;

        for (Framework::Entity entity : entityManager->GetAllEntities())
        {
            // Filter for Projectiles
            if (entityManager->HasComponent<ProjectileMovement>(entity) &&
                entityManager->HasComponent<Transform>(entity) &&
                entityManager->HasComponent<CircleCollider>(entity))
            {
                activeProjectiles.push_back(entity);
            }

            // Filter for Enemies (must have EnemyAI to exclude players)
            if (entityManager->HasComponent<EnemyAI>(entity) &&
                entityManager->HasComponent<Transform>(entity) &&
                entityManager->HasComponent<Health>(entity) &&
                entityManager->HasComponent<BoxCollider>(entity))
            {
                activeEnemies.push_back(entity);
            }
        }

        // --- BROADPHASE (Quadtree) ---
        AABB worldBounds;
        {
            const Grid& g = GetGrid();
            worldBounds.min = g.worldbound_min;
            worldBounds.max = g.worldbound_max;

            // Fallback if grid bounds are not initialized
            if (worldBounds.min.x == 0.0f && worldBounds.min.y == 0.0f &&
                worldBounds.max.x == 0.0f && worldBounds.max.y == 0.0f)
            {
                worldBounds.min = Vector2D(-100.0f, -100.0f);
                worldBounds.max = Vector2D(100.0f, 100.0f);
            }
        }

        Quadtree enemyQt(worldBounds, 6, 8);

        for (Framework::Entity enemy : activeEnemies)
        {
            // activeEnemies already filtered, but keep this safe
            if (!entityManager->HasComponent<Transform>(enemy) ||
                !entityManager->HasComponent<BoxCollider>(enemy))
            {
                continue;
            }

            auto& enemyTransform = entityManager->GetComponent<Transform>(enemy);
            auto& enemyCollider = entityManager->GetComponent<BoxCollider>(enemy);

            const AABB enemyAABB = MakeAABBFromCenterSize(enemyTransform.position, enemyCollider.size);
            enemyQt.Insert(enemy, enemyAABB);
        }

        std::vector<Framework::Entity> enemyCandidates;
        enemyCandidates.reserve(32);


        // 2. COLLISION AND DEFERRED DESTRUCTION LOGIC
        std::vector<Framework::Entity> entitiesToDestroy;

        for (Framework::Entity projectile : activeProjectiles)
        {
            // Skip if already marked for destruction by collision
            if (std::find(entitiesToDestroy.begin(), entitiesToDestroy.end(), projectile) != entitiesToDestroy.end())
                continue;

            // Checks if components still exist (catches off-screen cleanup)
            if (!entityManager->HasComponent<Transform>(projectile) ||
                !entityManager->HasComponent<CircleCollider>(projectile) ||
                !entityManager->HasComponent<ProjectileMovement>(projectile))
            {
                continue;
            }

            // --- FETCH PROJECTILE COMPONENTS ---
            auto& projTransform = entityManager->GetComponent<Transform>(projectile);
            auto& projCollider = entityManager->GetComponent<CircleCollider>(projectile);

            Collider projShape = Collider::create_circle(projCollider.radius, projTransform.position);

            // Query candidate enemies near this projectile.
            
			enemyCandidates.clear();

            // Match your current projectile shape: center = projTransform.position (offset ignored in your code)
            const AABB projAABB = MakeAABBFromCircle(projTransform.position, projCollider.radius);
            enemyQt.Query(projAABB, enemyCandidates);

            for (Framework::Entity enemy : enemyCandidates)
            {
                // Skip enemy if already marked for destruction
                if (std::find(entitiesToDestroy.begin(), entitiesToDestroy.end(), enemy) != entitiesToDestroy.end())
                    continue;

                // Check enemy components
                if (!entityManager->HasComponent<Transform>(enemy) ||
                    !entityManager->HasComponent<Health>(enemy) ||
                    !entityManager->HasComponent<BoxCollider>(enemy))
                {
                    continue;
                }

                auto& enemyTransform = entityManager->GetComponent<Transform>(enemy);
                auto& enemyHealth = entityManager->GetComponent<Health>(enemy);
                auto& enemyCollider = entityManager->GetComponent<BoxCollider>(enemy);

                Collider enemyShape = Collider::create_rect(
                    enemyCollider.size.x,
                    enemyCollider.size.y,
                    enemyTransform.position
                );

                if (check_collision(projShape, enemyShape))
                {
                    // Use configurable damage from the projectile component
                    auto& projMovement = entityManager->GetComponent<ProjectileMovement>(projectile);
                    const int damageDealt = projMovement.damage;
                    enemyHealth.TakeDamage(damageDealt);

                    if (eventSystem) {
                        eventSystem->QueueMessage(new EnemyDamagedMessage(
                            enemy, projectile, damageDealt, enemyHealth.currentHealth, enemyTransform.position));
                    }

                    if (enemyHealth.isDead)
                    {
                        if (eventSystem) {
                            eventSystem->QueueMessage(new EnemyDeathMessage(
                                enemy, projectile, enemyTransform.position));
                        }
                        entitiesToDestroy.push_back(enemy);
                    }

                    // Pierce: projectile continues through enemies
                    // Non-pierce: projectile destroyed on first hit
                    if (!projMovement.pierce) {
                        entitiesToDestroy.push_back(projectile);
                        break;
                    }
                    // If piercing, continue checking next enemies (don't break)
                }
            }

        }

        // 3. EXECUTE DEFERRED DESTRUCTION (Collision Hits)
        for (Framework::Entity entity : entitiesToDestroy)
        {
            // Check for a core component before destroying
            if (entityManager->HasComponent<Transform>(entity))
            {
                // Clear tile occupancy before destroying so the tile becomes walkable
                SpatialPartitioningRemove(entity);
                entityManager->DestroyEntity(entity);
            }
        }
    }
}