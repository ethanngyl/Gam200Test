/**
===============================================================================
 File:           PlayerControllerSystem.cpp
 Description:    Player input controller - Implementation
===============================================================================
 */


#include "Precompiled.h"
#include "PlayerManager.h"
#include "EntitySpawner.h"


#include "RenderComponents.h"

#include "Grid\GridECS.h"
#include "Grid\Grid.h"
#include <GLFW/glfw3.h>
#include "Vector2D.h"
#include <cmath>


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

        // One-time tint so the player stands out from the grid
        if (entityManager->HasComponent<Renderable>(playerEntity)) {
            static bool tinted = false;
            if (!tinted) {
                auto& rend = entityManager->GetComponent<Renderable>(playerEntity); 
                rend.tint = glm::vec4(1.0f, 0.85f, 0.2f, 1.0f);    // yellow/gold

                tinted = true; // don’t set it every frame
            }
        }

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

        HandleClickToMove();

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
        if (inputSystem->IsKeyPressed(MOUSE_RIGHT) && shootCooldown <= 0.0f) {
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

    void PlayerControllerSystem::HandleClickToMove() {
        // 1) Basic guards
        if (!inputSystem || !window || !entityManager) return;

        if (!inputSystem->IsKeyPressed(MOUSE_LEFT))   return;

        const Framework::Grid& g = Framework::GetGrid();
        if (g.cols <= 0 || g.rows <= 0) return;

        // 2) Read mouse and convert to world (same path you already used)
        double cx = 0.0, cy = 0.0;            glfwGetCursorPos(window, &cx, &cy);
        int fbW = 0, fbH = 0;                 glfwGetFramebufferSize(window, &fbW, &fbH);
        if (fbW <= 0 || fbH <= 0) return;

        float sx = 1.0f, sy = 1.0f;           glfwGetWindowContentScale(window, &sx, &sy);
        double fx = cx * sx, fy = cy * sy;
        if (sx == 0.0f || sy == 0.0f) {
            int winW = 0, winH = 0; glfwGetWindowSize(window, &winW, &winH);
            if (winW > 0 && winH > 0) {
                fx = cx * (double)fbW / (double)winW;
                fy = cy * (double)fbH / (double)winH;
            }
        }

        const double ndcX = (fx / (double)fbW) * 2.0 - 1.0;
        const double ndcY = 1.0 - (fy / (double)fbH) * 2.0;

        // Use your same world extents
        constexpr float WORLD_X_HALF = 2.0f;
        constexpr float WORLD_Y_HALF = 1.0f;

        const float worldX = (float)ndcX * WORLD_X_HALF;
        const float worldY = (float)ndcY * WORLD_Y_HALF;
        Framework::Vector2D clickWorld{ worldX, worldY };

        // 3) Get player transform
        if (!entityManager->HasComponent<Transform>(playerEntity)) return;
        auto& xform = entityManager->GetComponent<Transform>(playerEntity);

        // 4) Current tile & its exact center
        auto curTile = Framework::WorldToTile(xform.position);
        if (!curTile) return;
        Framework::Vector2D curCenter = Framework::TileToWorld({ curTile->x, curTile->y });

        // 5) If we're not centered, snap once and quit (prevents “slanted” drift)
        const float tolX = 0.25f * g.spacing.x;
        const float tolY = 0.25f * g.spacing.y;
        if (std::fabs(xform.position.x - curCenter.x) > tolX ||
            std::fabs(xform.position.y - curCenter.y) > tolY) {
            xform.position = curCenter;
            return;
        }

        // 6) Decide the step in SCREEN PIXELS (robust against camera/aspect)
        int winW = 0, winH = 0;  glfwGetWindowSize(window, &winW, &winH);
        if (winW <= 0 || winH <= 0) return;

        auto worldToPixel = [&](const Framework::Vector2D& w) -> std::pair<float, float> {
            // world -> NDC
            float x = w.x / WORLD_X_HALF;
            float y = w.y / WORLD_Y_HALF;
            // NDC [-1,+1] -> [0,1]
            float u = (x * 0.5f) + 0.5f;
            float v = (y * 0.5f) + 0.5f;
            // [0,1] -> pixels (v is bottom-up; convert to top-left origin)
            return { u * winW, (1.0f - v) * winH };
            };

        auto [cxPix, cyPix] = worldToPixel(curCenter);
        auto [mxPix, myPix] = worldToPixel(clickWorld);

        float dxPix = mxPix - cxPix;
        float dyPix = myPix - cyPix;

        // deadzone in pixels to ignore tiny jiggle
        constexpr float DEAD_PX = 2.0f;
        if (std::fabs(dxPix) < DEAD_PX && std::fabs(dyPix) < DEAD_PX) return;

        // 7) Dominant-axis step (4-way). Screen Y grows downward:
        //    click above -> dyPix < 0. Flip the sign below if your world’s “up” is opposite.
        int stepCol = 0, stepRow = 0;
        if (std::fabs(dyPix) > std::fabs(dxPix)) {
            stepRow = (dyPix < 0.f) ? 1 : -1;   // if up-is-negative in your world, use ? -1 : 1
        }
        else {
            stepCol = (dxPix > 0.f) ? 1 : -1;
        }

        int nextCol = curTile->x + stepCol;
        int nextRow = curTile->y + stepRow;

        // 8) Bounds + walkable
        if (!Framework::InBounds({ nextCol, nextRow })) return;
        if (!Framework::IsWalkable({ nextCol, nextRow })) return;

        // 9) Move exactly one tile; update occupancy in correct order
        Framework::Vector2D nextCenter = Framework::TileToWorld({ nextCol, nextRow });
        Framework::SetOccupant({ curTile->x, curTile->y }, Framework::Entity{ Framework::INVALID_ENTITY });
        xform.position = nextCenter;
        Framework::SetOccupant({ nextCol, nextRow }, playerEntity);
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
