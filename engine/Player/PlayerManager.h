/**
===============================================================================
 File:           PlayerControllerSystem.h
 Description:    Player input controller - Header

 Handles player shooting and entity spawning based on keyboard/mouse input.
===============================================================================
 */

#pragma once
#include "Precompiled.h"

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

        // ========================================================================
        // CONFIGURATION
        // ========================================================================

        void SetShootCooldown(float cooldown);
        void SetProjectileSpeed(float speed);

    private:
        // ========================================================================
        // SHOOTING HANDLERS
        // ========================================================================

        void HandleShootUp(const Vector2D& playerPos);
        void HandleShootDown(const Vector2D& playerPos);
        void HandleShootAtMouse(const Vector2D& playerPos);

        // ========================================================================
        // ENTITY SPAWNING HANDLERS (Debug/Testing)
        // ========================================================================

        void HandleSpawnEnemy();
        void HandleSpawnObstacle();
        void HandleSpawnPickup();

        // ========================================================================
        // MEMBER VARIABLES
        // ========================================================================

        EntitySpawner* spawner;
        EntityManager* entityManager;
        InputSystem* inputSystem;
        GLFWwindow* window;
        Entity playerEntity;

        // Shooting configuration
        float shootCooldown;
        float shootCooldownTime;
        float projectileSpeed;
    };

} // namespace Framework