/******************************************************************************
===============================================================================
 File:           PlayerManager.h
 Author:         <MAIN AUTHOR NAME>
 Co-authors:     PADILLA CARL JAMESON Z.
 Email:          <main.author@digipen.edu>
 Date:           2025/11/07
 Contribution:   PADILLA CARL JAMESON Z.: 10%
 ------------------------------------------------------------------------------
  Description:
  Declares the player control interface for a grid-based, turn-driven game.
  Exposes movement handlers (click-to-move and arrow-key step) and lightweight
  visual feedback helpers (tile pulse + border outline) used to confirm input.

  Design notes:
  - Single-tile movement semantics with bounds/walkability checks
  - Turn integration: movements can end the player’s turn
  - Non-blocking, timer-based UI feedback (start/stop pulse; show/hide outline)
  - Minimal surface area: effects are initiated here and updated per frame
===============================================================================
******************************************************************************/

#pragma once
#include "Precompiled.h"
#include "Grid/GridECS.h"

namespace Framework {

    class EntitySpawner;

    /**
    * @brief Player Controller System - Handles player shooting/spawning input
    */
    class PlayerControllerSystem : public EngineSystem {
    public:
        PlayerControllerSystem();
        ~PlayerControllerSystem();

        void Initialize() override;
        void Update(float dt) override;
        void SendEngineMessage(Message* msg) override;

        // ========================================================================
        // SETTERS
        // ========================================================================

        void SetEntitySpawner(EntitySpawner* s);
        void SetEntityManager(EntityManager* em);
        void SetInputSystem(InputSystem* is);
        void SetPlayerEntity(Entity player);
        void SetWindow(GLFWwindow* win);
        void SetAudioSystem(AudioSystem* audio);

        // ========================================================================
        // CONFIGURATION
        // ========================================================================

        void SetShootCooldown(float cooldown);
        void SetProjectileSpeed(float speed);

        void ResetGridState();
        void SetGridMovementEnabled(bool enabled);
		bool HandleTileInteraction(const GridCoord& tileCoord);

    private:
        // ========================================================================
        // SHOOTING HANDLERS
        // ========================================================================

        void HandleShootUp(const Vector2D& playerPos);
        void HandleShootDown(const Vector2D& playerPos);
        void HandleShootAtMouse(const Vector2D& playerPos);
        void HandleAttackAction();
		Entity FindFirstEnemyInRange(int minRange, int maxRange);

        // ========================================================================
        // PLAYER MOVEMENT HANDLERS
        // ========================================================================
        void UpdateTilePulseAnimation();
        void UpdateBorderOutlineAnimation();
        void HandleArrowKeyMovement();
        void StartTilePulse(Framework::Entity tileEntity, float pulseScale, DWORD pulseDurationMs);
        void ShowBorderOutline(const GridCoord& tile, float thicknessFraction, DWORD durationMs);

        // ========================================================================
        // MEMBER VARIABLES
        // ========================================================================

        EntitySpawner* spawner;
        EntityManager* entityManager;
        InputSystem* inputSystem;
        GLFWwindow* window;
        Entity playerEntity;
        AudioSystem* audioSystem;
        
        // Shooting configuration
        float shootCooldown;
        float shootCooldownTime;
        float projectileSpeed;
        bool gridMovementEnabled = false;

        uint64_t lastTurnIndex = 0;
    };

} // namespace Framework