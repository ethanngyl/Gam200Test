/**
===============================================================================
File: Core.h (Enhanced - Integration Version)
Author: GE YONGQI / Enhanced by integration
-------------------------------------------------------------------------

Design Improvements:
- Integrate InitializeEngineSystems() from main.cpp into Core
- Integrate CleanupEngineSystems() into Core
- Add the ShouldWindowClose() method
- Add the UpdateSingleFrame() method

Goal: Make main.cpp more concise, requiring only calls to Core methods
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

    /**
     * @class CoreEngine
     * @brief Enhanced Core Engine - Integrates initialization, update, and cleanup functions
     *
     * New functions:
     * - InitializeAllSystems(): Creates and initializes all systems
     * - Cleanup(): Cleans up all systems and resources
     * - ShouldWindowClose(): Checks if the window is closed
     * - UpdateSingleFrame(): Single-frame update (GSM-friendly)
    */
    class CoreEngine
    {
    public:
        CoreEngine();
        ~CoreEngine();

        // ====================================================================
        // Initialize all systems with one click
        // ====================================================================

        /**
        * @brief Creates, connects, and initializes all engine systems
        *
         * This method will:
         * 1. Create all system objects
         * 2. Connect dependencies between systems
         * 3. Initialize all systems in the correct order
         * 4. Add the systems to the CoreEngine
        *
        * @return true if initialization was successful
        */
        bool InitializeAllSystems();

        /**
        * @brief Cleans up all systems and resources
         * This method will:
         * 1. Delete all systems
         * 2. Clean up the EntityManager
         * 3. Terminate GLFW
        */
        void Cleanup();

        // ====================================================================
        // Window status check
        // ====================================================================

        /**
         * @brief Checks if the window should be closed
         * @return true if the user requested to close the window
         *
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


        /**
         * @brief Initializes the added system
         * @note Usually does not need to be called manually, InitializeAllSystems() will call it
         */
        void Initialize();

        /**
         * @brief Add a system to the engine
         * @param system System pointer
         */
        void AddSystem(EngineSystem* system);

        /**
         * @brief Delete all systems
         * @note Usually does not need to be called manually, Cleanup() will call it
         */
        void DestroySystems();

        /**
         * @brief Broadcast message to all systems
         * @param message Message pointer
         */
        void BroadcastMessage(Message* message);

        /**
         * @brief Standalone game loop (do not use with GSM)
         * @warning Contains an infinite loop that will block GSM
         */

        bool IsPlaying() const { return isPlaying; }
        void SetPlaying(bool value) { isPlaying = value; }

         //void GameLoop();

    private:
        // System Collection
        std::vector<EngineSystem*> Systems;

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

        // state
        unsigned LastTime;
        bool GameActive;
        bool isPlaying = false;


        // Private helper methods
        void CreateAllSystems();
        void WireSystemDependencies();
        void InitializeCriticalSystems();
        void AddSystemsToEngine();
    };

    /**
     * @brief Global engine pointer
     */
    extern CoreEngine* CORE;
}