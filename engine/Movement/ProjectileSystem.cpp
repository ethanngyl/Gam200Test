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
                //if statement to check if entity has enemy component, checks if projectile collides with enemy, sends a msg to enemy that its taking dmg
                // once health gone, destroy enemy entity plus projectile
               
                CheckProjectileEnemyCollisions();

                // Optional: Destroy projectiles that go off-screen
                // This prevents memory leaks from projectiles flying forever
                if (transform.position.x > 3.0f || transform.position.x < -3.0f ||
                    transform.position.y > 2.0f || transform.position.y < -2.0f)
                {
                    // Projectile is off-screen, destroy it
                    entityManager->DestroyEntity(entity);
                }
            }
        }
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
        if (!entityManager) return;

        // Collect all projectiles
        std::vector<Entity> projectiles;
        for (Entity e : entityManager->GetAllEntities())
        {
            if (entityManager->HasComponent<ProjectileMovement>(e) &&
                entityManager->HasComponent<Transform>(e) &&
                entityManager->HasComponent<CircleCollider>(e))
            {
                projectiles.push_back(e);
            }
        }

        // Collect all enemies
        std::vector<Entity> enemies;
        for (Entity e : entityManager->GetAllEntities())
        {
            // *** Check if entity is an enemy ***
            if (entityManager->HasComponent<Transform>(e) &&
                entityManager->HasComponent<BoxCollider>(e) &&
                entityManager->HasComponent<Health>(e))  // Must have health!
            {
                enemies.push_back(e);
            }
        }

        // Check each projectile against each enemy
        for (Entity projectile : projectiles)
        {
            auto& projTransform = entityManager->GetComponent<Transform>(projectile);
            auto& projCollider = entityManager->GetComponent<CircleCollider>(projectile);

            // Create collision shape for projectile
               Collider projShape = Collider::create_circle(
                projCollider.radius,
                projTransform.position
            );

            bool projectileHit = false;

            for (Entity enemy : enemies)
            {
                auto& enemyTransform = entityManager->GetComponent<Transform>(enemy);
                auto& enemyCollider = entityManager->GetComponent<BoxCollider>(enemy);
                auto& enemyHealth = entityManager->GetComponent<Health>(enemy);

                // Skip if enemy is already dead
                if (enemyHealth.isDead) continue;

                // Create collision shape for enemy
                Collider enemyShape = Collider::create_rect(
                    enemyCollider.size.x,
                    enemyCollider.size.y,
                    enemyTransform.position
                );

                // *** CHECK COLLISION ***
                if (check_collision(projShape, enemyShape))
                {
                    std::cout << "[ProjectileSystem] Projectile " << projectile.GetID()
                        << " hit Enemy " << enemy.GetID() << "!\n";

                    // Deal damage to enemy
                    const int PROJECTILE_DAMAGE = 10;
                    enemyHealth.TakeDamage(PROJECTILE_DAMAGE);

                    std::cout << "  Enemy health: " << enemyHealth.currentHealth
                        << "/" << enemyHealth.maxHealth << "\n";

                    // *** PUBLISH ENEMY_DAMAGED EVENT ***
                    if (eventSystem) {
                        eventSystem->QueueMessage(
                            new EnemyDamagedMessage(
                                enemy,
                                projectile,
                                PROJECTILE_DAMAGE,
                                enemyHealth.currentHealth,
                                enemyTransform.position
                            )
                        );
                    }

                    // Check if enemy died
                    if (enemyHealth.isDead)
                    {
                        std::cout << "Enemy " << enemy.GetID() << " DESTROYED!\n";

                        // *** PUBLISH ENEMY_DEATH EVENT ***
                        if (eventSystem) {
                            eventSystem->QueueMessage(
                                new EnemyDeathMessage(
                                    enemy,
                                    projectile,
                                    enemyTransform.position
                                )
                            );
                        }

                        // Destroy the enemy entity
                        entityManager->DestroyEntity(enemy);
                    }

                    // Mark projectile for destruction
                    projectileHit = true;
                    break;  // One projectile can only hit one enemy
                }
            }

            // Destroy projectile if it hit something
            if (projectileHit)
            {
                entityManager->DestroyEntity(projectile);
            }
        }
    }
}