#pragma once
#include "Precompiled.h"
#include "Interface.h"
#include "Message.h"

/**
 * @file Core.h
 * @author ETHAN NG YONG LE (n.ethanyongle@digipen.edu)
 * @brief Core engine implementation providing game loop and system management
 * @date 2025-09-30
 *
 * @copyright Copyright (c) 2025
 *
 * Defines the CoreEngine class which serves as the central orchestrator
 * for the game engine, managing the game loop, system lifecycle, and
 * inter-system messaging.
 */

namespace Framework
{
    /**
     * @class CoreEngine
     * @brief Central orchestrator for the game engine
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
         */
        void Initialize();

        /**
         * @brief Main game loop that runs until termination
         *
         * Executes each frame:
         * - Calculates delta time
         * - Checks for window close
         * - Updates all systems
         * - Monitors performance metrics
         */
        void GameLoop();

        // System management

        /**
         * @brief Adds a system to the engine
         * @param system Pointer to the system to register
         * @note Systems are updated in the order they are added
         */
        void AddSystem(InterfaceSystem* system);
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
        std::vector<InterfaceSystem*> Systems;

        // Timing
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