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
namespace Framework
{
    /**
     * @brief Constructs the MovementSystem
     *
     * Initializes entity manager and input system pointers to nullptr.
     * These must be set via SetEntityManager() and SetInputSystem() before use.
     */
    MovementSystem::MovementSystem() : entityManager(nullptr), inputSystem(nullptr)
    {
    }

    /**
     * @brief Destructor for MovementSystem
     */
    MovementSystem::~MovementSystem()
    {
    }

    /**
     * @brief Initializes the movement system
     *
     * Prints initialization confirmation to console.
     */
    void MovementSystem::Initialize()
    {
        std::cout << "MovementSystem: Initialized\n";
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
    void MovementSystem::Update(float dt)
    {
        if (!entityManager || !inputSystem) return;
        //if (!Framework::CORE || !Framework::CORE->IsPlaying()) return;

        for (Entity entity : entityManager->GetAllEntities())
        {
            if (entityManager->HasComponent<Transform>(entity) &&
                entityManager->HasComponent<Movement>(entity))
            {
                auto& transform = entityManager->GetComponent<Transform>(entity);
                auto& movement = entityManager->GetComponent<Movement>(entity);

                // Get input direction from WASD
                Vector2D inputDir(0, 0);

                if (inputSystem->IsKeyDown(KEY_W)) inputDir.y += 1.0f;
                if (inputSystem->IsKeyDown(KEY_S)) inputDir.y -= 1.0f;
                if (inputSystem->IsKeyDown(KEY_A)) inputDir.x -= 1.0f;
                if (inputSystem->IsKeyDown(KEY_D)) inputDir.x += 1.0f;

                // === SCALING ===
                if (inputSystem->IsKeyDown(KEY_5)) {
                    transform.scale.x += 1.0f * dt;
                    transform.scale.y += 1.0f * dt;
                }

                if (inputSystem->IsKeyDown(KEY_6)) {
                    transform.scale.x -= 1.0f * dt;
                    transform.scale.y -= 1.0f * dt;
                }

                // === Scale Clamping ===
                transform.scale.x = std::clamp(transform.scale.x, transform.lowerLimit, transform.upperLimit);
                transform.scale.y = std::clamp(transform.scale.y, transform.lowerLimit, transform.upperLimit);

                // === ROTATION ===
                if (inputSystem->IsKeyDown(KEY_3)) {
                    transform.rotation += 90.0f * dt;
                }

                if (inputSystem->IsKeyDown(KEY_4)) {
                    transform.rotation -= 90.0f * dt;
                }


                // Normalize diagonal movement
                if (inputDir.length() > 0.0f) {
                    inputDir.normalize();
                }

                // Update movement direction
                movement.direction = inputDir;

                // Apply movement
                if (!movement.blocked) {
                    transform.position += movement.direction * movement.moveSpeed * dt;
                }
                else {
                    transform.position -= movement.direction * movement.moveSpeed * dt*10;
                }

                //// Optional: Keep on screen
                if (transform.position.x > 2.0f) transform.position.x = 2.0f;
                if (transform.position.x < -2.0f) transform.position.x = -2.0f;
                if (transform.position.y > 1.0f) transform.position.y = 1.0f;
                if (transform.position.y < -1.0f) transform.position.y = -1.0f;
            }
        }
    }

    /**
     * @brief Handles engine messages
     * @param message Pointer to the message to process
     *
     * Currently does not process any messages.
     */
    void MovementSystem::SendEngineMessage(Message* message)
    {
        (void)message;
    }
}