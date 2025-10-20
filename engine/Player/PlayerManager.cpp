/**
===============================================================================
 File:           PlayerControllerSystem.cpp
 Description:    Player input controller - Implementation
===============================================================================
 */

#include "Precompiled.h"
#include "PlayerManager.h"
#include "EntitySpawner.h"

namespace Framework {

    // ============================================================================
    // CONSTRUCTOR / DESTRUCTOR
    // ============================================================================

    PlayerControllerSystem::PlayerControllerSystem()
        : spawner(nullptr)
        , entityManager(nullptr)
        , inputSystem(nullptr)
        , window(nullptr)
        , playerEntity(0)
        , shootCooldown(0.0f)
        , shootCooldownTime(0.2f)
        , projectileSpeed(0.5f)
    {
    }

    PlayerControllerSystem::~PlayerControllerSystem()
    {
    }

    // ============================================================================
    // ENGINE SYSTEM INTERFACE
    // ============================================================================

    void PlayerControllerSystem::Initialize()
    {
        std::cout << "[PlayerController] Initialized\n";
    }

    void PlayerControllerSystem::Update(float dt)
    {
        // Check if we have all required systems
        if (!spawner || !entityManager || !inputSystem) {
            return;
        }

        // Check if player entity is valid
        if (!entityManager->HasComponent<Transform>(playerEntity)) {
            return;
        }

        // Get player position
        auto& playerTransform = entityManager->GetComponent<Transform>(playerEntity);
        Vector2D playerPos = playerTransform.position;

        // Update shooting cooldown
        if (shootCooldown > 0.0f) {
            shootCooldown -= dt;
        }

        // ====================================================================
        // SHOOTING INPUT - Use InputSystem
        // ====================================================================

        HandleShootUp(playerPos);
        HandleShootDown(playerPos);
        HandleShootAtMouse(playerPos);

        // ====================================================================
        // SPAWNING INPUT (Debug/Testing) - Use InputSystem
        // ====================================================================

        HandleSpawnEnemy();
        HandleSpawnObstacle();
        HandleSpawnPickup();
    }

    void PlayerControllerSystem::SendEngineMessage(Message* msg)
    {
        (void)msg;
    }

    // ============================================================================
    // SETTERS
    // ============================================================================

    void PlayerControllerSystem::SetEntitySpawner(EntitySpawner* s)
    {
        spawner = s;
    }

    void PlayerControllerSystem::SetEntityManager(EntityManager* em)
    {
        entityManager = em;
    }

    void PlayerControllerSystem::SetInputSystem(InputSystem* is)
    {
        inputSystem = is;
    }

    void PlayerControllerSystem::SetPlayerEntity(Entity player)
    {
        playerEntity = player;
    }

    void PlayerControllerSystem::SetWindow(GLFWwindow* win)
    {
        window = win;
    }

    void PlayerControllerSystem::SetShootCooldown(float cooldown)
    {
        shootCooldownTime = cooldown;
    }

    void PlayerControllerSystem::SetProjectileSpeed(float speed)
    {
        projectileSpeed = speed;
    }

    // ============================================================================
    // SHOOTING HANDLERS - Using InputSystem
    // ============================================================================

    void PlayerControllerSystem::HandleShootUp(const Vector2D& playerPos)
    {
        // Use InputSystem's IsKeyPressed - no redundant input checking!
        if (inputSystem->IsKeyPressed(KEY_SPACE) && shootCooldown <= 0.0f) {
            spawner->SpawnProjectile(
                Vector2D(playerPos.x, playerPos.y + 0.15f),
                Vector2D(0.0f, 1.0f),  // Shoot up
                projectileSpeed
            );
            shootCooldown = shootCooldownTime;
            std::cout << "[PlayerController] Shoot up!\n";
        }
    }

    void PlayerControllerSystem::HandleShootDown(const Vector2D& playerPos)
    {
        // Using InputSystem - checks if SHIFT was just pressed
        if (inputSystem->IsKeyPressed(KEY_SHIFT) && shootCooldown <= 0.0f) {
            spawner->SpawnProjectile(
                Vector2D(playerPos.x, playerPos.y - 0.15f),
                Vector2D(0.0f, -1.0f),  // Shoot down
                projectileSpeed
            );
            shootCooldown = shootCooldownTime;
            std::cout << "[PlayerController] Shoot down!\n";
        }
    }

    void PlayerControllerSystem::HandleShootAtMouse(const Vector2D& playerPos)
    {
        // Using InputSystem for mouse input
        if (inputSystem->IsKeyPressed(MOUSE_LEFT) && shootCooldown <= 0.0f) {
            // Get mouse position from InputSystem
            float mouseX, mouseY;
            inputSystem->GetMousePosition(mouseX, mouseY);

            // Convert screen to world coordinates (simplified - adjust for your camera)
            // Assuming 800x600 window, adjust these values for your actual window size
            float worldX = (mouseX / 800.0f) * 4.0f - 2.0f;
            float worldY = -((mouseY / 600.0f) * 2.0f - 1.0f);  // Flip Y

            // Calculate direction from player to mouse
            Vector2D direction(worldX - playerPos.x, worldY - playerPos.y);

            // Normalize direction
            float length = sqrtf(direction.x * direction.x + direction.y * direction.y);
            if (length > 0.01f) {
                direction.x /= length;
                direction.y /= length;

                spawner->SpawnProjectile(
                    Vector2D(playerPos.x, playerPos.y),
                    direction,
                    projectileSpeed
                );
                shootCooldown = shootCooldownTime;
                std::cout << "[PlayerController] Shoot at mouse!\n";
            }
        }
    }

    // ============================================================================
    // ENTITY SPAWNING HANDLERS (Debug/Testing) - Using InputSystem
    // ============================================================================

    void PlayerControllerSystem::HandleSpawnEnemy()
    {
        if (inputSystem->IsKeyPressed(KEY_E)) {
            // Spawn enemy above screen
            float randomX = -1.5f + (rand() / (float)RAND_MAX) * 3.0f;
            spawner->SpawnEnemy(Vector2D(randomX, 0.8f), 0.1f);
            std::cout << "[PlayerController] Spawned enemy\n";
        }
    }

    void PlayerControllerSystem::HandleSpawnObstacle()
    {
        // Note: KEY_Q triggers quit in Input.cpp, so using KEY_O instead
        if (inputSystem->IsKeyPressed(KEY_O)) {
            // Spawn obstacle at random position
            float randomX = -1.5f + (rand() / (float)RAND_MAX) * 3.0f;
            float randomY = -0.5f + (rand() / (float)RAND_MAX) * 1.0f;
            spawner->SpawnObstacle(
                Vector2D(randomX, randomY),
                Vector2D(0.3f, 0.3f)
            );
            std::cout << "[PlayerController] Spawned obstacle\n";
        }
    }

    void PlayerControllerSystem::HandleSpawnPickup()
    {
        if (inputSystem->IsKeyPressed(KEY_R)) {
            // Spawn pickup at random position
            float randomX = -1.5f + (rand() / (float)RAND_MAX) * 3.0f;
            float randomY = -0.5f + (rand() / (float)RAND_MAX) * 1.0f;
            spawner->SpawnSprite("circle", Vector2D(randomX, randomY), Vector2D(0.15f, 0.15f));
            std::cout << "[PlayerController] Spawned pickup\n";
        }
    }

} // namespace Framework
