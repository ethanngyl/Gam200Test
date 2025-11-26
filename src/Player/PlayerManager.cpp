/******************************************************************************
===============================================================================
 File:           PlayerManager.cpp
 Author:         ETHAN NG YONG LE
 Co-authors:     PADILLA CARL JAMESON Z.
 Email:          <main.author@digipen.edu>
 Date:           2025/11/07
 Contribution:   ETHAN NG YONG LE: 65%, PADILLA CARL JAMESON Z.: 35%
 ------------------------------------------------------------------------------
  Description:
  Implements player grid movement and on-tile feedback. Mouse clicks map to a
  target tile for click-to-move; arrow keys step a single tile. Both paths run
  validation (in-bounds, walkable, occupancy), update player/tile state, and
  coordinate with the turn system. Visual confirmation uses two timed effects:
  a one-shot tile pulse (scale up then restore) and a temporary border outline
  (four strips) that auto-hides on timeout.

  Design notes:
  - Input → grid mapping → validated move → state/turn updates
  - TilePulse: start/extend, tracked by a countdown; restores scale on expiry
  - BorderOutline: show with configurable thickness; hides on timeout
  - Frame-friendly: effects update via lightweight per-frame timers only
===============================================================================
******************************************************************************/

#include "Precompiled.h"
#include "PlayerManager.h"
#include "EntitySpawner.h"
#include "Audio/AudioSystem.h"
#include "RenderComponents.h"
#include "Graphics/RenderLayers.h"
#include "MathABS.h"
#include "AnimationSystem.h"

#include "Grid\GridECS.h"
#include "Grid\Grid.h"
#include "Vector2D.h"
#include "Pathfinding.h"
#include "Turn.h"
#include <Windows.h>

namespace Framework {

    /**
    * @brief Ends an active tile pulse when its timer expires; restores original scale.
    */

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

    /**
    * @brief Starts/extends a tile pulse by scaling once and timing its restore.
    * @param tileEntity     Tile entity to pulse
    * @param pulseScale     One-shot scale multiplier
    * @param pulseDurationMs Duration before restoring original scale
    */
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

    /**
    * @brief Arrow-key single-step grid movement (validates bounds/walkability).
    *        Shows feedback (border + pulse), updates occupancy, and ends player turn.
    */
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

    /**
    * @brief Shows a rectangular border around a target tile using four thin sprites.
    * @param tile               Target grid coordinate
    * @param thicknessFraction  Fraction of tile size to use for strip thickness
    * @param durationMs         Duration before auto-hide
    */
    void PlayerControllerSystem::ShowBorderOutline(const Framework::GridCoord& tgt, float thicknessFraction, DWORD durationMs) {
        // DEBUGGING: Disable outline entities
        (void)tgt; (void)thicknessFraction; (void)durationMs;
        return;

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
        , audioSystem(nullptr)  // Fix: Initialize to nullptr to prevent garbage pointer
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

        if (!Framework::CORE || !Framework::CORE->IsPlaying()) return;
        // Check if we have all required systems
        if (!spawner || !entityManager || !inputSystem) {
            return;
        }

        // Check if player entity is valid
        if (!entityManager->HasComponent<Transform>(playerEntity)) {
            return;
        }

        //// --- NEW, SIMPLER TURN LOGIC ---
        //if (IsPlayerTurn())
        //{
        //    // Check if player is out of actions and auto-end turn
        //    if (entityManager->HasComponent<AP>(playerEntity))
        //    {
        //        auto& stats = entityManager->GetComponent<AP>(playerEntity);
        //        if (stats.actionPoints <= 0)
        //        {
        //            EndPlayerTurn();
        //            LOG_INFO("PlayerTurn", "Player out of AP. Auto-ending turn.");
        //        }
        //    }
        //}

        if (IsPlayerTurn()) {
            // Get global turn info
            auto& globalTurn = Turn();

            // 1. AP Regeneration Check: If the global turn number is newer than the last one we processed
            if (globalTurn.turnIndex > lastTurnIndex)
            {
                // It's a brand new turn! Regen AP.
                if (entityManager->HasComponent<AP>(playerEntity))
                {
                    auto& stats = entityManager->GetComponent<AP>(playerEntity);
                    stats.actionPoints = stats.maxActionPoints;
                    LOG_INFO("PlayerTurn", "New Turn Started! AP Refilled to %d", stats.actionPoints);
                }

                //Regen attack AP
                if (entityManager->HasComponent<AttackAP>(playerEntity)) {                  
                    auto& aap = entityManager->GetComponent<AttackAP>(playerEntity);        
                    aap.points = aap.maxPoints;                                             
                    LOG_INFO("PlayerTurn", "Attack AP refilled to %d", aap.points);         
                }

                // Update our tracker so we don't regen again this turn
                lastTurnIndex = globalTurn.turnIndex;
            }

            // 2. Auto-End Turn Check: Check if player is out of AP and auto-end turn
            if (entityManager->HasComponent<AP>(playerEntity))
            {
                auto& stats = entityManager->GetComponent<AP>(playerEntity);
                if (stats.actionPoints <= 0)
                {
                    EndPlayerTurn();
                    LOG_INFO("PlayerTurn", "Player out of AP. Auto-ending turn.");
                    // Return here to stop further actions for this frame
                    return;
                }
            }

            HandleAttackAction();

            //// --- (Optional) PLAYER ATTACK on SPACE ---
            //if (inputSystem->IsKeyPressed(KEY_SPACE))
            //{
            //    if (entityManager->HasComponent<AP>(playerEntity))
            //    {
            //        auto& stats = entityManager->GetComponent<AP>(playerEntity);
            //        if (stats.actionPoints > 0)
            //        {
            //            stats.actionPoints--; // Spend 1 AP to attack
            //            LOG_INFO("Player", "Player ATTACKS! AP remaining: %d", stats.actionPoints);
            //            // !! Add attack logic here (find nearby enemy, deal damage) !!
            //        }
            //    }
            //}
        } // --- End IsPlayerTurn() ---


        // Get player position
        auto& playerTransform = entityManager->GetComponent<Transform>(playerEntity);
        Vector2D playerPos = playerTransform.position;

        // One-time tint so the player stands out from the grid
        if (entityManager->HasComponent<Renderable>(playerEntity)) {
            static bool tinted = false;
            if (!tinted) {
                auto& rend = entityManager->GetComponent<Renderable>(playerEntity);
                rend.visible = true;
                rend.layer = RenderLayers::Player;  // Use standard player layer (4)

                //rend.tint = glm::vec4(1.0f, 0.85f, 0.2f, 1.0f);    // yellow/gold

                tinted = true; // don't set it every frame
            }
        }

        // Update shooting cooldown
        if (shootCooldown > 0.0f) {
            shootCooldown -= dt;
        }

        // Update arrow movement cooldown
        if (arrowMoveCooldown > 0.0f) {
            arrowMoveCooldown -= dt;
            if (arrowMoveCooldown < 0.0f) {
                arrowMoveCooldown = 0.0f;
            }
        }

        // ====================================================================
        // SHOOTING INPUT - Use InputSystem
        // ====================================================================

        /*HandleShootUp(playerPos);
        HandleShootDown(playerPos);
        HandleShootAtMouse(playerPos);*/


        // ====================================================================
        // MOVEMENT INPUT - Click to move
        // ====================================================================

        UpdateTilePulseAnimation();
        UpdateBorderOutlineAnimation();

        //Added grid movement with arrow keys
        HandleArrowKeyMovement();

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

    void PlayerControllerSystem::SetAudioSystem(AudioSystem* audio)
    {
        audioSystem = audio;
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

            // Play shooting sound effect
            if (audioSystem) {
                audioSystem->PlaySound("shooting", false);
                std::cout << "[PlayerController] Shoot up with SFX!\n";
            }
            else {
                std::cout << "[PlayerController] Shoot up! (No audio system)\n";
            }
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
            // Play shooting sound effect
            if (audioSystem) {
                audioSystem->PlaySound("shooting", false);
                std::cout << "[PlayerController] Shoot up with SFX!\n";
            }
            else {
                std::cout << "[PlayerController] Shoot up! (No audio system)\n";
            }
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
                // Play shooting sound effect
                if (audioSystem) {
                    audioSystem->PlaySound("shooting", false);
                    std::cout << "[PlayerController] Shoot up with SFX!\n";
                }
                else {
                    std::cout << "[PlayerController] Shoot up! (No audio system)\n";
                }
                shootCooldown = shootCooldownTime;
                std::cout << "[PlayerController] Shoot at mouse!\n";
            }
        }
    }

    /**
    * @brief Arrow-key single-step grid movement (validates bounds/walkability).
    *        Shows feedback (border + pulse), updates occupancy, and ends player turn.
    */
    void PlayerControllerSystem::HandleArrowKeyMovement() {
        // DEBUG: Log entry to this function
        static bool firstCall = true;
        static int callCounter = 0;
        if (firstCall) {
            LOG_INFO("PlayerManager", "=== HandleArrowKeyMovement FIRST CALL ===");
            firstCall = false;
        }

        callCounter++;
        LOG_INFO("PlayerManager", ">>> HandleArrowKeyMovement called (call #%d this frame)", callCounter);

        // FIX: Check cooldown to prevent double AP consumption from same key press
        if (arrowMoveCooldown > 0.0f) {
            LOG_INFO("PlayerManager", "Arrow movement blocked by cooldown (%.3fs remaining)", arrowMoveCooldown);
            return;
        }

        // Guards: need input + entity systems, player Transform, and it must be player's turn
        if (!gridMovementEnabled) {
            LOG_WARN("PlayerManager", "Arrow keys BLOCKED: gridMovementEnabled = false");
            return;
        }

        if (!inputSystem || !entityManager) {
            LOG_ERROR("PlayerManager", "Arrow keys BLOCKED: inputSystem=%p entityManager=%p", inputSystem, entityManager);
            return;
        }

        if (!entityManager->HasComponent<Transform>(playerEntity)) {
            LOG_ERROR("PlayerManager", "Arrow keys BLOCKED: Player missing Transform component");
            return;
        }

        if (!IsPlayerTurn()) {
            static int logThrottle = 0;
            if (logThrottle++ % 60 == 0) { // Log every 60 frames
                LOG_WARN("PlayerManager", "Arrow keys BLOCKED: Not player's turn (throttled log)");
            }
            return;
        }

        if (!entityManager->HasComponent<AP>(playerEntity))
        {
            LOG_ERROR("PlayerManager", "Arrow keys BLOCKED: Player has no AP component!");
            return;
        }
        auto& stats = entityManager->GetComponent<AP>(playerEntity);

        // Check if player is out of moves
        if (stats.actionPoints <= 0)
        {
            static int apLogThrottle = 0;
            if (apLogThrottle++ % 60 == 0) { // Log every 60 frames
                LOG_WARN("PlayerManager", "Arrow keys BLOCKED: Player out of AP (AP=%d, throttled log)", stats.actionPoints);
            }
            return;
        }

        const Framework::Grid& g = Framework::GetGrid();
        if (g.cols <= 0 || g.rows <= 0) {
            LOG_ERROR("PlayerManager", "Arrow keys BLOCKED: Invalid grid (cols=%d, rows=%d)", g.cols, g.rows);
            return;
        }

        LOG_INFO("PlayerManager", "Arrow key handler ACTIVE: AP=%d/%d, Turn=Player, Grid=%dx%d",
                 stats.actionPoints, stats.maxActionPoints, g.cols, g.rows);

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
        std::string animationName = "";
        bool flipAnimation = false;

        if (inputSystem->IsKeyPressed(KEY_UP)) {
            stepY = 1;  // Move up (increase Y)
            animationName = "Idle_back";
            LOG_INFO("PlayerManager", "<<< KEY PRESS DETECTED: UP >>>");
            std::cout << "[Arrow] Moving UP\n";
        }
        else if (inputSystem->IsKeyPressed(KEY_DOWN)) {
            stepY = -1; // Move down (decrease Y)
            animationName = "Idle_front";
            LOG_INFO("PlayerManager", "<<< KEY PRESS DETECTED: DOWN >>>");
            std::cout << "[Arrow] Moving DOWN\n";
        }
        else if (inputSystem->IsKeyPressed(KEY_LEFT)) {
            stepX = -1; // Move left (decrease X)
            animationName = "Idle_sideview";
            flipAnimation = true;  // Flip sprite to face left
            LOG_INFO("PlayerManager", "<<< KEY PRESS DETECTED: LEFT >>>");
            std::cout << "[Arrow] Moving LEFT\n";
        }
        else if (inputSystem->IsKeyPressed(KEY_RIGHT)) {
            stepX = 1;  // Move right (increase X)
            animationName = "Idle_sideview";
            flipAnimation = false;  // Normal orientation for right
            LOG_INFO("PlayerManager", "<<< KEY PRESS DETECTED: RIGHT >>>");
            std::cout << "[Arrow] Moving RIGHT\n";
        }
        else {
            return; // No movement input
        }

        // Switch animation based on movement direction
        if (!animationName.empty() && CORE && CORE->GetAnimationSystem()) {
            auto* animSys = CORE->GetAnimationSystem();
            auto* gfx = CORE->GetGraphicsSystem();

            if (entityManager->HasComponent<SpriteAnimation>(playerEntity)) {
                auto& anim = entityManager->GetComponent<SpriteAnimation>(playerEntity);
                animSys->LoadAnimation(playerEntity, anim, gfx, animationName);
                anim.flipX = flipAnimation;
                LOG_INFO("PlayerManager", "Switched to animation: %s (flip: %d)", animationName.c_str(), flipAnimation);
            }
        }

        // Calculate next tile
        Framework::GridCoord next{ cur.x + stepX, cur.y + stepY };

        // Check if next tile is valid and walkable
        if (!Framework::InBounds(next)) {
            std::cout << "[WASD] Out of bounds! Current(" << cur.x << "," << cur.y
                << ") -> Next(" << next.x << "," << next.y << ")\n";
            return;
        }

        if (!HandleTileInteraction(next)) {
            // Interaction blocked movement (e.g., goal without all chests)
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

        //EndPlayerTurn();

        // --- AND REPLACE IT WITH THIS ---
        int apBefore = stats.actionPoints;
        stats.actionPoints--; // Spend one AP
        int apAfter = stats.actionPoints;
        LOG_INFO("PlayerManager", "***** AP CONSUMED: %d -> %d (delta: -1) *****", apBefore, apAfter);
        LOG_INFO("PlayerManager", "Player moved. AP Remaining: %d", stats.actionPoints);

        // FIX: Set cooldown to prevent double AP consumption (200ms = 0.2 seconds)
        arrowMoveCooldown = 0.2f;
        LOG_INFO("PlayerManager", "Arrow move cooldown activated (0.2s)");
    }

    void PlayerControllerSystem::ResetGridState() {
        // Clear tile pulse state
        s_pulseEnt = Framework::Entity{};
        s_savedScaleX = 1.0f;
        s_savedScaleY = 1.0f;
        s_pulseEndMs = 0;

        // Clear border outline state
        if (s_outlineInit && entityManager) {
            auto destroyStrip = [&](Framework::Entity& e) {
                if (e.GetID() != Framework::INVALID_ENTITY) {
                    entityManager->DestroyEntity(e);
                    e = Framework::Entity{};
                }
                };

            destroyStrip(s_outlineTop);
            destroyStrip(s_outlineBot);
            destroyStrip(s_outlineLeft);
            destroyStrip(s_outlineRight);
        }

        s_outlineInit = false;
        s_outlineHideAtMs = 0;

        LOG_INFO("PlayerController", "Grid state reset");
    }

    void PlayerControllerSystem::SetGridMovementEnabled(bool enabled) {
        gridMovementEnabled = enabled;
        LOG_INFO("PlayerController", "Grid movement %s", enabled ? "ENABLED" : "DISABLED");
    }

    /**
 * @brief Checks and handles chest collection and goal interaction
 * @param nextTile The tile the player is moving to
 * @return true if movement should proceed, false if blocked
 */
    bool PlayerControllerSystem::HandleTileInteraction(const Framework::GridCoord& nextTile) {
        const Framework::Grid& grid = Framework::GetGrid();
        Framework::Entity tileEntity = grid.TileAt(nextTile.x, nextTile.y);

        if (tileEntity.GetID() == Framework::INVALID_ENTITY) return true;
        if (!entityManager->HasComponent<Framework::GridTiles>(tileEntity)) return true;

        auto& gridTile = entityManager->GetComponent<Framework::GridTiles>(tileEntity);
        Framework::Entity occupant = gridTile.occupant;

        if (occupant.GetID() == Framework::INVALID_ENTITY) return true;

        // ========================================================================
        // CHEST COLLECTION
        // ========================================================================
        if (entityManager->HasComponent<Framework::Chest>(occupant)) {
            auto& chest = entityManager->GetComponent<Framework::Chest>(occupant);

            if (!chest.collected) {
                // Add to player inventory
                if (entityManager->HasComponent<Framework::Inventory>(playerEntity)) {
                    auto& inventory = entityManager->GetComponent<Framework::Inventory>(playerEntity);
                    inventory.AddChest(chest.chestID);

                    LOG_INFO("PlayerManager", "Player collected Chest %d! Total: %d",
                        chest.chestID, inventory.GetChestCount());
                }

                // Mark chest as collected
                chest.collected = true;

                // UNBLOCK the tile so enemies can pass
                //gridTile.blocked = false;

                // Visual feedback: hide chest or change appearance
                if (entityManager->HasComponent<Framework::Renderable>(occupant)) {
                    auto& renderable = entityManager->GetComponent<Framework::Renderable>(occupant);
                    renderable.visible = false;  // Hide collected chest
                }

                // Clear occupant
                gridTile.occupant = Framework::Entity{ Framework::INVALID_ENTITY };

                LOG_INFO("PlayerManager", "Tile (%d, %d) UNBLOCKED after chest collection",
                    nextTile.x, nextTile.y);
            }

            return true;  // Allow movement onto chest tile
        }

        // ========================================================================
        // GOAL INTERACTION
        // ========================================================================
        if (entityManager->HasComponent<Goal>(occupant)) {
            auto& goal = entityManager->GetComponent<Goal>(occupant);

            // Check if player has all required chests
            if (entityManager->HasComponent<Inventory>(playerEntity)) {
                auto& inventory = entityManager->GetComponent<Inventory>(playerEntity);
                int chestsCollected = inventory.GetChestCount();
                int chestsRequired = goal.chestsRequired;

                LOG_INFO("PlayerManager", "Player at GOAL: %d/%d chests collected",
                    chestsCollected, chestsRequired);

                if (chestsCollected >= chestsRequired) {
                    // VICTORY! Player can exit
                    LOG_INFO("PlayerManager", "=== LEVEL COMPLETE! ===");
                    LOG_INFO("PlayerManager", "All chests collected! Returning to main menu...");

                    // Trigger level completion (you can customize this)
                    // Option 1: Return to main menu
                    next = mainMenu;

                    // Option 2: Load next level
                    // next = LEVEL_4;

                    // Option 3: Show victory screen
                    // next = victoryScreen;

                    return true;  // Allow movement onto goal
                }
                else {
                    // Not enough chests
                    LOG_WARN("PlayerManager", "Cannot exit! Need %d more chests",
                        chestsRequired - chestsCollected);

                    // Block movement onto goal
                    return false;
                }
            }
            else {
                LOG_WARN("PlayerManager", "Player has no inventory!");
                return false;
            }
        }

        return true;
    }

    // 
    Entity PlayerControllerSystem::FindFirstEnemyInRange(int minRange, int maxRange) {
        if (!entityManager || !entityManager->HasComponent<Transform>(playerEntity)) return {};
        auto& ptf = entityManager->GetComponent<Transform>(playerEntity);

        auto optPlayerTile = WorldToTile(ptf.position);
        if (!optPlayerTile) return {};

        GridCoord p = *optPlayerTile;

        // Iterate all entities: pick first with EnemyAI + Health in manhattan range
        for (Entity e : entityManager->GetAllEntities()) {
            if (!entityManager->HasComponent<EnemyAI>(e)) continue;
            if (!entityManager->HasComponent<Health>(e)) continue;

            // enemy world → tile
            if (!entityManager->HasComponent<Transform>(e)) continue;
            auto& etf = entityManager->GetComponent<Transform>(e);
            auto optETile = WorldToTile(etf.position);
            if (!optETile) continue;

            GridCoord q = *optETile;
            int manhattan = Abs(q.x - p.x) + Abs(q.y - p.y);
            if (manhattan >= minRange && manhattan <= maxRange) {
                return e; // first match
            }
        }
        return {};
    }

    // NEW
    void PlayerControllerSystem::HandleAttackAction() {
        if (!inputSystem || !entityManager) return;
        if (!IsPlayerTurn()) return;
        if (!inputSystem->IsKeyPressed(KEY_SPACE)) return; // edge trigger

        if (!entityManager->HasComponent<AttackAP>(playerEntity)) {
            // No attack AP component → fall back to existing shooting behavior
            return;
        }
        auto& aap = entityManager->GetComponent<AttackAP>(playerEntity);
        if (aap.points <= 0) {
            LOG_WARN("PlayerAttack", "No attack AP left");
            return;
        }

        int minR = 1, maxR = 1;
        /*if (entityManager->HasComponent<AttackRangeComponent>(playerEntity)) {
            auto& rng = entityManager->GetComponent<AttackRangeComponent>(playerEntity);
            minR = rng.minRange;
            maxR = rng.maxRange;
        }*/

        Entity target = FindFirstEnemyInRange(minR, maxR);
        if (target.GetID() == INVALID_ENTITY) {
            LOG_INFO("PlayerAttack", "No enemy in range [%d..%d]", minR, maxR);
            return;
        }

        // Deal damage (flat 1 for now)
        auto& hp = entityManager->GetComponent<Health>(target);
        hp.TakeDamage(1);
        aap.points--; // consume attack AP

        LOG_INFO("PlayerAttack", "Hit enemy %u for 1. Enemy HP now %d/%d. AttackAP=%d/%d",
            target.GetID(), hp.currentHealth, hp.maxHealth, aap.points, aap.maxPoints);

        // Optional SFX
        //if (audioSystem) audioSystem->PlaySound("hit", false);

        // Optional: if enemy died, hide/cleanup
        if (hp.isDead && entityManager->HasComponent<Renderable>(target)) {
            entityManager->GetComponent<Renderable>(target).visible = false;
        }
    }

} // namespace Framework