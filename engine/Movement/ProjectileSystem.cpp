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
===============================================================================
 */
#include "Precompiled.h"
#include "PlayerManager.h"
#include "ProjectileSystem.h"
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
        //if (!Framework::CORE || !Framework::CORE->IsPlaying()) return;
        for (Entity entity : entityManager->GetAllEntities())
        {
            if (entityManager->HasComponent<Transform>(entity) &&
                entityManager->HasComponent<ProjectileMovement>(entity))
            {
                auto& transform = entityManager->GetComponent<Transform>(entity);
                auto& movement = entityManager->GetComponent<ProjectileMovement>(entity);

                transform.position.x += movement.direction.x * movement.moveSpeed * dt;
                transform.position.y += movement.direction.y * movement.moveSpeed * dt;

                // Apply movement
                if (!movement.blocked) {
                    transform.position += movement.direction * movement.moveSpeed * dt;
                }
                else {
                    transform.position -= movement.direction * movement.moveSpeed * dt;
                }

                // Optional: Destroy projectiles that go off-screen
                // This prevents memory leaks from projectiles flying forever
                if (transform.position.x > 3.0f || transform.position.x < -3.0f ||
                    transform.position.y > 2.0f || transform.position.y < -2.0f)
                {
                    // Projectile is off-screen, destroy it
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

            // Filter for Enemies
            if (entityManager->HasComponent<Transform>(entity) &&
                entityManager->HasComponent<Health>(entity) &&
                entityManager->HasComponent<BoxCollider>(entity))
            {
                activeEnemies.push_back(entity);
            }
        }

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
            auto& projMovement = entityManager->GetComponent<ProjectileMovement>(projectile);

            Collider projShape = Collider::create_circle(projCollider.radius, projTransform.position);

            for (Framework::Entity enemy : activeEnemies)
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

                // --- FETCH ENEMY COMPONENTS ---
                auto& enemyTransform = entityManager->GetComponent<Transform>(enemy);
                auto& enemyHealth = entityManager->GetComponent<Health>(enemy);
                auto& enemyCollider = entityManager->GetComponent<BoxCollider>(enemy);

                // Create collision shape for enemy
                     Collider enemyShape = Collider::create_rect(
                         enemyCollider.size.x,
                         enemyCollider.size.y,
                         enemyTransform.position
                     );

                // COLLISION CHECK AND EVENT HANDLING
                if (check_collision(projShape, enemyShape))
                {
                    // 1. DEAL DAMAGE and QUEUE EnemyDamagedMessage
                    const int damageDealt = 10;
                    enemyHealth.TakeDamage(damageDealt);

                    if (eventSystem) {
                        eventSystem->QueueMessage(new EnemyDamagedMessage(
                            enemy, projectile, damageDealt, enemyHealth.currentHealth, enemyTransform.position));
                    }

                    // 2. CHECK FOR DEATH
                    if (enemyHealth.isDead)
                    {
                        if (eventSystem) {
                            eventSystem->QueueMessage(new EnemyDeathMessage(
                                enemy, projectile, enemyTransform.position));
                        }

                        entitiesToDestroy.push_back(enemy);
                    }

                    // Mark projectile for deferred destruction
                    entitiesToDestroy.push_back(projectile);
                    break; // Projectile is consumed after one hit
                }
            }
        }

        // 3. EXECUTE DEFERRED DESTRUCTION (Collision Hits)
        for (Framework::Entity entity : entitiesToDestroy)
        {
            // Check for a core component before destroying
            if (entityManager->HasComponent<Transform>(entity))
            {
                entityManager->DestroyEntity(entity);
            }
        }
    }
}