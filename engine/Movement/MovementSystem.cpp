#include "Precompiled.h"

namespace Framework
{
    MovementSystem::MovementSystem() : entityManager(nullptr), inputSystem(nullptr)
    {
    }

    MovementSystem::~MovementSystem()
    {
    }

    void MovementSystem::Initialize()
    {
        std::cout << "MovementSystem: Initialized\n";
    }

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
                transform.position += movement.direction * movement.moveSpeed * dt;

                // Optional: Keep on screen
                if (transform.position.x > 1.0f) transform.position.x = 1.0f;
                if (transform.position.x < -1.0f) transform.position.x = -1.0f;
                if (transform.position.y > 1.0f) transform.position.y = 1.0f;
                if (transform.position.y < -1.0f) transform.position.y = -1.0f;
            }
        }
    }
    void MovementSystem::SendEngineMessage(Message* message)
    {
    }
}