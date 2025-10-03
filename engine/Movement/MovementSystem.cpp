/**
 * @file MovementSystem.cpp
 * @author ETHAN NG YONG LE (n.ethanyongle@digipen.edu)
 * @brief Movement system implementation for entity motion control
 * @date 2025-09-30
 *
 * Implements WASD keyboard-based movement for entities with Transform and Movement components.
 * Handles input processing, direction normalization, and boundary clamping to keep entities on screen.
 */
#include "Precompiled.h"

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
				std::cout << "position X is : " << transform.position.x << std::endl;
				std::cout << "position Y is : " << transform.position.y << std::endl;
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