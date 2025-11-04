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
#include "AI/Turn.h"



namespace Framework {

    static Framework::Entity s_pulseEnt;          // INVALID by default
    static float s_savedScaleX = 1.0f, s_savedScaleY = 1.0f;
    static ULONGLONG s_pulseEndMs = 0;

    void PlayerControllerSystem::UpdateTilePulseAnimation() {
        const ULONGLONG nowMs = GetTickCount64();

        // Check if pulse animation should end
        if (s_pulseEnt.GetID() != Framework::INVALID_ENTITY && nowMs >= s_pulseEndMs) {
            if (entityManager && entityManager->HasComponent<Transform>(s_pulseEnt)) {
                auto& tf = entityManager->GetComponent<Transform>(s_pulseEnt);
                tf.scale.x = s_savedScaleX;  // restore original
                tf.scale.y = s_savedScaleY;
            }
            s_pulseEnt = Framework::Entity{}; // clear active pulse
        }
    }

    void PlayerControllerSystem::StartTilePulse(Framework::Entity clickedEnt, float pulseScale, DWORD pulseDurationMs) {
        if (clickedEnt.GetID() == Framework::INVALID_ENTITY ||
            !entityManager->HasComponent<Transform>(clickedEnt)) {
            return;
        }

        auto& tf = entityManager->GetComponent<Transform>(clickedEnt);
        const ULONGLONG now = GetTickCount64();

        // If a different tile was pulsing, restore it first
        if (s_pulseEnt.GetID() != Framework::INVALID_ENTITY &&
            s_pulseEnt.GetID() != clickedEnt.GetID() &&
            entityManager->HasComponent<Transform>(s_pulseEnt))
        {
            auto& prevTf = entityManager->GetComponent<Transform>(s_pulseEnt);
            prevTf.scale.x = s_savedScaleX;
            prevTf.scale.y = s_savedScaleY;
        }

        if (s_pulseEnt.GetID() == clickedEnt.GetID()) {
            // Same tile: don't multiply again — just extend the timer
            s_pulseEndMs = now + pulseDurationMs;
        }
        else {
            // New pulse: capture base scale, set scaled value once, start timer
            s_savedScaleX = tf.scale.x;
            s_savedScaleY = tf.scale.y;
            tf.scale.x = s_savedScaleX * pulseScale;
            tf.scale.y = s_savedScaleY * pulseScale;

            s_pulseEnt = clickedEnt;
            s_pulseEndMs = now + pulseDurationMs;
        }
    }

    // ============================================================================
    // BORDER OUTLINE ANIMATION - Shared static variables
    // ============================================================================

    // Static variables shared between border functions
    static Framework::Entity s_outlineTop, s_outlineBot, s_outlineLeft, s_outlineRight;
    static bool s_outlineInit = false;
    static ULONGLONG s_outlineHideAtMs = 0;

    void PlayerControllerSystem::UpdateBorderOutlineAnimation() {
        const ULONGLONG nowMs = GetTickCount64();

        // Auto-hide any visible outline when its time expires
        if (s_outlineInit && s_outlineHideAtMs && nowMs >= s_outlineHideAtMs) {
            auto hideStrip = [&](Framework::Entity e) {
                if (e.GetID() != Framework::INVALID_ENTITY &&
                    entityManager && entityManager->HasComponent<Transform>(e)) {
                    auto& tf = entityManager->GetComponent<Transform>(e);
                    tf.scale.x = 0.0f;
                    tf.scale.y = 0.0f;
                }
                };
            hideStrip(s_outlineTop);
            hideStrip(s_outlineBot);
            hideStrip(s_outlineLeft);
            hideStrip(s_outlineRight);
            s_outlineHideAtMs = 0;
        }
    }

    void PlayerControllerSystem::ShowBorderOutline(const Framework::GridCoord& tgt, float thicknessFraction, DWORD durationMs) {
        const Framework::Grid& g = Framework::GetGrid();
        const float tileW = g.spacing.x;
        const float tileH = g.spacing.y;
        const Framework::Vector2D center = Framework::TileToWorld(tgt);

        // Lazy-create strips once
        if (!s_outlineInit) {
            auto makeStrip = [&](Framework::Entity& out) {
                out = entityManager->CreateEntity();
                entityManager->AddComponent<Transform>(out, center);
                entityManager->AddComponent<Sprite>(out);
                entityManager->GetComponent<Sprite>(out).texturePath = "quad";
                };
            makeStrip(s_outlineTop);
            makeStrip(s_outlineBot);
            makeStrip(s_outlineLeft);
            makeStrip(s_outlineRight);
            s_outlineInit = true;
        }

        const float thick = thicknessFraction * (tileW < tileH ? tileW : tileH);

        auto place = [&](Framework::Entity e, float cx, float cy, float sx, float sy) {
            if (entityManager->HasComponent<Transform>(e)) {
                auto& tf = entityManager->GetComponent<Transform>(e);
                tf.position.x = cx; tf.position.y = cy;
                tf.scale.x = sx;    tf.scale.y = sy;
            }
            };

        // Top/bottom (horizontal strips)
        place(s_outlineTop, center.x, center.y + tileH * 0.5f, tileW, thick);
        place(s_outlineBot, center.x, center.y - tileH * 0.5f, tileW, thick);
        // Left/right (vertical strips)
        place(s_outlineLeft, center.x - tileW * 0.5f, center.y, thick, tileH);
        place(s_outlineRight, center.x + tileW * 0.5f, center.y, thick, tileH);

        s_outlineHideAtMs = GetTickCount64() + durationMs;
    }
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
        DBG_SCOPE_SYS("Player Controller System", eng::debug::Subsystem::Gameplay);

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
                rend.visible = true;
                rend.layer = 1;

                //rend.tint = glm::vec4(1.0f, 0.85f, 0.2f, 1.0f);    // yellow/gold

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

    void PlayerControllerSystem::HandleClickToMove() {
        if (!entityManager || !inputSystem) return;

        // Only on the press frame
        if (!inputSystem->IsKeyPressed(MOUSE_LEFT)) return;

        // Prefer window-relative cursor → consistent with your system (you store GLFWwindow* already)
        if (!window) return;
        double cx = 0.0, cy = 0.0;
        glfwGetCursorPos(window, &cx, &cy);

        int winW = 0, winH = 0;
        glfwGetWindowSize(window, &winW, &winH);
        if (winW <= 0 || winH <= 0) return;

        // Map to your existing world range (matches your mouse-shoot code)
        float worldX = static_cast<float>((cx / double(winW)) * 4.0 - 2.0);
        float worldY = static_cast<float>(-((cy / double(winH)) * 2.0 - 1.0)); // flip Y

        // World -> Tile
        auto maybeTile = WorldToTile(Vector2D{ worldX, worldY });
        if (!maybeTile.has_value()) {
            std::cout << "[ClickMove] outside grid\n";
            return;
        }
        GridCoord target = *maybeTile;

        // Only move to walkable tiles
        if (!IsWalkable(target)) {
            std::cout << "[ClickMove] blocked (" << target.x << "," << target.y << ")\n";
            return;
        }

        // Ensure player entity & transform exist
        if (!entityManager->HasComponent<Transform>(playerEntity)) return;
        auto& xform = entityManager->GetComponent<Transform>(playerEntity);

        // Clear previous occupant (if any)
        if (auto prev = WorldToTile(xform.position); prev.has_value()) {
            SetOccupant(*prev, Entity{ INVALID_ENTITY });
        }

        // Snap to tile center + set occupancy
        Vector2D snapped = TileToWorld(target);
        xform.position = snapped;
        SetOccupant(target, playerEntity);

        std::cout << "[ClickMove] moved to (" << target.x << "," << target.y << ")\n";
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

    void PlayerControllerSystem::HandleArrowKeyMovement() {
        // Guards
        if (!inputSystem || !entityManager) return;
        if (!entityManager->HasComponent<Transform>(playerEntity)) return;
        if (!IsPlayerTurn()) return;

        const Framework::Grid& g = Framework::GetGrid();
        if (g.cols <= 0 || g.rows <= 0) return;

        // Get current player position
        auto& xform = entityManager->GetComponent<Transform>(playerEntity);
        auto curTileOpt = Framework::WorldToTile(xform.position);
        if (!curTileOpt) return;

        Framework::GridCoord cur = *curTileOpt;

        // Recentre player on current tile if drifted
        const Framework::Vector2D curCenter = Framework::TileToWorld(cur);
        const float tolX = 0.25f * g.spacing.x;
        const float tolY = 0.25f * g.spacing.y;
        if (std::fabs(xform.position.x - curCenter.x) > tolX ||
            std::fabs(xform.position.y - curCenter.y) > tolY) {
            xform.position = curCenter;
        }

        // Check for Arrow Key input (use IsKeyPressed for one-time press detection)
        int stepX = 0, stepY = 0;

        if (inputSystem->IsKeyPressed(KEY_UP)) {
            stepY = 1;  // Move up (increase Y)
            std::cout << "[Arrow] Moving UP\n";
        }
        else if (inputSystem->IsKeyPressed(KEY_DOWN)) {
            stepY = -1; // Move down (decrease Y)
            std::cout << "[Arrow] Moving DOWN\n";
        }
        else if (inputSystem->IsKeyPressed(KEY_LEFT)) {
            stepX = -1; // Move left (decrease X)
            std::cout << "[Arrow] Moving LEFT\n";
        }
        else if (inputSystem->IsKeyPressed(KEY_RIGHT)) {
            stepX = 1;  // Move right (increase X)
            std::cout << "[Arrow] Moving RIGHT\n";
        }
        else {
            return; // No movement input
        }

        // Calculate next tile
        Framework::GridCoord next{ cur.x + stepX, cur.y + stepY };

        // Check if next tile is valid and walkable
        if (!Framework::InBounds(next)) {
            std::cout << "[WASD] Out of bounds! Current(" << cur.x << "," << cur.y
                << ") -> Next(" << next.x << "," << next.y << ")\n";
            return;
        }

        if (!Framework::IsWalkable(next)) {
            std::cout << "[WASD] Tile blocked! (" << next.x << "," << next.y << ")\n";
            return;
        }

        // Visual feedback for target tile
        ShowBorderOutline(next, 0.22f, 200);
        const auto& gridRef = Framework::GetGrid();
        Framework::Entity targetEnt = gridRef.TileAt(next.x, next.y);
        StartTilePulse(targetEnt, 1.15f, 150);

        // Update occupancy
        Framework::SetOccupant(cur, Framework::Entity{ Framework::INVALID_ENTITY });

        // Move player
        xform.position = Framework::TileToWorld(next);
        Framework::SetOccupant(next, playerEntity);

        std::cout << "[WASD] Moved from (" << cur.x << "," << cur.y
            << ") to (" << next.x << "," << next.y << ")\n";

        EndPlayerTurn();
    }


} // namespace Framework
