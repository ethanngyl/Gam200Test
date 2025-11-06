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
    ProjectileMovementSystem::ProjectileMovementSystem() : entityManager(nullptr)
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
                    transform.position -= movement.direction * movement.moveSpeed * dt * 10;
                }

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
}