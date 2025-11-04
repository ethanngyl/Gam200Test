/*
===============================================================================
 File:           Core.h
 Author:         ETHAN NG YONG LE
 Email:          n.ethanyongle@digipen.edu
 Date:           2025-09-30
 Contribution:   100%
 ------------------------------------------------------------------------------
 
  Design notes:
    Defines the CoreEngine class which serves as the central orchestrator
 * for the game engine, managing the game loop, system lifecycle, and
 * inter-system messaging.
===============================================================================
 */

#pragma once
#include "Precompiled.h"
#include "Interface.h"
#include "Message.h"

namespace Framework
{
    // Forward declaration
    class EntityManager;
    class WindowSystem;
    class GraphicsSystemV2;
    class InputSystem;
    class CollisionSystem;
    class MovementSystem;
    class ProjectileMovementSystem;
    class EntitySpawner;
    class PlayerControllerSystem;
    class ImGuiSystem;
    class AudioSystem;
    class AnimationSystem;
    class UISystem;

    /**
     * @class CoreEngine
     * @brief Enhanced Core Engine - Integrates initialization, update, and cleanup functions
     *
     * The CoreEngine manages:
     * - Main game loop execution and frame timing
     * - System initialization, updates, and cleanup
     * - Message broadcasting between systems
     * - Global game state
     */
    class CoreEngine
    {
    public:
        /**
         * @brief Constructs the CoreEngine instance
         *
         * Initializes timing, sets game state to active, and assigns
         * the global CORE pointer.
         */
        CoreEngine();

        /**
         * @brief Destructor
         * @note Cleanup is handled by DestroySystems()
         */
        ~CoreEngine();

        // Core functionality

        /**
         * @brief Initializes all registered systems in dependency order
         *
         * Performs multi-phase initialization:
         * 1. WindowSystem initialization
         * 2. GraphicsSystem window assignment
         * 3. Remaining systems initialization
         * @note This replaces ShouldWindowClose() in main.cpp
        */
        bool ShouldWindowClose() const;

        // ====================================================================
        // Single frame update (GSM friendly)
        // ====================================================================

        /**
         * @brief Update all systems for one frame
         * @param dt Delta time in seconds
         *
         * @note This is for the GSM loop and does not include its own infinite loop.
        */
        void UpdateSingleFrame(float dt);

        // ====================================================================
        // State Access
        // ====================================================================

        /**
         * @brief Check if the engine is active
         * @return true if the engine should continue running
        */
        bool IsActive() const { return GameActive; }

        /**
         * @brief Set the engine active state
         * @param active The new active state
         */
        void SetActive(bool active) { GameActive = active; }

        // ====================================================================
        // System accessor (for external access)
        // ====================================================================

        EntityManager* GetEntityManager() const { return entityManager; }
        WindowSystem* GetWindowSystem() const { return windowSystem; }
        GraphicsSystemV2* GetGraphicsSystem() const { return graphicsSystem; }
        InputSystem* GetInputSystem() const { return inputSystem; }
        CollisionSystem* GetCollisionSystem() const { return collisionSystem; }
        MovementSystem* GetMovementSystem() const { return movementSystem; }
        ProjectileMovementSystem* GetProjectileSystem() const { return projectileSystem; }
        EntitySpawner* GetSpawner() const { return spawner; }
        PlayerControllerSystem* GetPlayerController() const { return playerController; }
        ImGuiSystem* GetImGuiSystem() const { return imguiSystem; }
        AudioSystem* GetAudioSystem() const { return audioSystem; }
        AnimationSystem* GetAnimationSystem() const { return animationSystem; }
        UISystem* GetUISystem() const { return uiSystem; }


        /**
         * @brief Initializes the added system
         * @note Usually does not need to be called manually, InitializeAllSystems() will call it
         */
        void Initialize();

        /**
         * @brief Add a system to the engine
         * @param system System pointer
         */
        void GameLoop();

        // System management

        /**
         * @brief Adds a system to the engine
         * @param system Pointer to the system to register
         * @note Systems are updated in the order they are added
         */
        void AddSystem(EngineSystem* system);
        /**
         * @brief Destroys all registered systems
         *
         * Deletes systems in reverse order of addition to minimize
         * dependency issues during cleanup.
         */
        void DestroySystems();

        // Message system

        /**
         * @brief Broadcasts a message to all systems
         * @param message Pointer to the message to send
         * @note Quit messages terminate the game loop
         */
        void BroadcastMessage(Message* message);

    private:
        // Systems collection
        std::vector<EngineSystem*> Systems;

        // Timing
        // Pointers to various systems (for easy management)
        EntityManager* entityManager;
        WindowSystem* windowSystem;
        GraphicsSystemV2* graphicsSystem;
        InputSystem* inputSystem;
        CollisionSystem* collisionSystem;
        MovementSystem* movementSystem;
        ProjectileMovementSystem* projectileSystem;
        EntitySpawner* spawner;
        PlayerControllerSystem* playerController;
        ImGuiSystem* imguiSystem;
        AudioSystem* audioSystem;
        AnimationSystem* animationSystem;
        UISystem* uiSystem;

        // state
        unsigned LastTime;

        // Game state
        bool GameActive;
    };

    /**
     * @brief Global pointer to the core engine instance
     * @note Provides system-wide access to the engine
     */
    extern CoreEngine* CORE;
}