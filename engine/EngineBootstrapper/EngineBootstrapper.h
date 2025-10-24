/**
===============================================================================
 File:           EngineBootstrapper.h
 Description:    Handles engine initialization and system setup
                 Keeps main.cpp clean by encapsulating all system wiring
===============================================================================
 */

#pragma once
#include "Precompiled.h"
#include "GraphicsSystemV2.h"
#include "EntitySpawner.h"
#include "PlayerManager.h"
#include "ProjectileSystem.h"
#include "ImguiSystem.h"
#include "MainMenuSystem.h"
#include "GSM/GameStateManager.h"  

namespace Framework {

    /**
     * @brief Configuration for engine initialization
     */
    struct EngineConfig {
        // Window settings
        int windowWidth = 1280;
        int windowHeight = 720;
        std::string windowTitle = "StructSquad";

        // GSM settings
        std::string gameConfigPath = "game_config.txt";

        // Debug settings
        bool enableImGui = true;
        bool enableDebugConsole = true;
    };

    /**
     * @brief Handles all engine system creation and wiring
     *
     * This class encapsulates the messy initialization code that would
     * otherwise clutter main.cpp. It creates all systems, wires their
     * dependencies, and registers them with the CoreEngine.
     */
    class EngineBootstrapper {
    public:
        /**
         * @brief Initialize the engine with all systems
         * @param engine Reference to the CoreEngine
         * @param config Engine configuration
         * @return true if initialization succeeded
         */
        static bool Initialize(CoreEngine& engine, const EngineConfig& config = EngineConfig());

        /**
         * @brief Get the entity manager (accessible after Initialize)
         */
        static EntityManager* GetEntityManager() { return s_entityManager; }

        /**
         * @brief Get the window system (accessible after Initialize)
         */
        static WindowSystem* GetWindowSystem() { return s_windowSystem; }

    private:
        // System creation
        static void CreateSystems();
        static void WireDependencies();
        static void InitializeCriticalSystems();
        static void RegisterSystems(CoreEngine& engine);

        // System storage (owned by CoreEngine after registration)
        static WindowSystem* s_windowSystem;
        static GraphicsSystemV2* s_graphicsSystem;
        static InputSystem* s_inputSystem;
        static CollisionSystem* s_collisionSystem;
        static MathTestSystem* s_mathSystem;
        static MovementSystem* s_movementSystem;
        static ProjectileMovementSystem* s_projectileSystem;
        static EntitySpawner* s_spawner;
        static PlayerControllerSystem* s_playerController;
        static ImGuiSystem* s_imguiSystem;
        static MainMenuSystem* s_mainMenu;
        static GameStateManager* s_gsm;

        // Entity manager (owned by bootstrapper, accessed via getter)
        static EntityManager* s_entityManager;

        // Config
        static EngineConfig s_config;
    };

} // namespace Framework