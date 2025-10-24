/**
===============================================================================
 File:           EngineBootstrapper.cpp
 Description:    Implementation of engine initialization and system setup
===============================================================================
 */

#include "Precompiled.h"
#include "EngineBootstrapper.h"
#include "EntitySpawner.h"
#include "PlayerManager.h"
#include "ProjectileSystem.h"
#include "ImguiSystem.h"
#include "MainMenuSystem.h"
#include "GSM/GameStateManager.h"

namespace Framework {

    // Static member definitions
    WindowSystem* EngineBootstrapper::s_windowSystem = nullptr;
    GraphicsSystemV2* EngineBootstrapper::s_graphicsSystem = nullptr;
    InputSystem* EngineBootstrapper::s_inputSystem = nullptr;
    CollisionSystem* EngineBootstrapper::s_collisionSystem = nullptr;
    MathTestSystem* EngineBootstrapper::s_mathSystem = nullptr;
    MovementSystem* EngineBootstrapper::s_movementSystem = nullptr;
    ProjectileMovementSystem* EngineBootstrapper::s_projectileSystem = nullptr;
    EntitySpawner* EngineBootstrapper::s_spawner = nullptr;
    PlayerControllerSystem* EngineBootstrapper::s_playerController = nullptr;
    ImGuiSystem* EngineBootstrapper::s_imguiSystem = nullptr;
    MainMenuSystem* EngineBootstrapper::s_mainMenu = nullptr;
    GameStateManager* EngineBootstrapper::s_gsm = nullptr;
    EntityManager* EngineBootstrapper::s_entityManager = nullptr;
    EngineConfig EngineBootstrapper::s_config;

    bool EngineBootstrapper::Initialize(CoreEngine& engine, const EngineConfig& config)
    {
        LOG_INFO("CORE", "=== EngineBootstrapper: Starting initialization ===");

        s_config = config;

        // Create entity manager
        s_entityManager = new EntityManager();
        if (!s_entityManager) {
            LOG_ERROR("ERROR", "[Bootstrapper] Failed to create EntityManager!");
            return false;
        }

        // Step 1: Create all systems
        CreateSystems();

        // Step 2: Wire dependencies between systems
        WireDependencies();

        // Step 3: Initialize critical systems (Window, Graphics)
        InitializeCriticalSystems();

        // Step 4: Register all systems with the engine
        RegisterSystems(engine);

        LOG_INFO("CORE", "=== EngineBootstrapper: Initialization complete ===");
        return true;
    }

    void EngineBootstrapper::CreateSystems()
    {
        LOG_INFO("CORE", "[Bootstrapper] Creating systems...");

        s_windowSystem = new WindowSystem();
        s_graphicsSystem = new GraphicsSystemV2();
        s_inputSystem = new InputSystem();
        s_collisionSystem = new CollisionSystem();
        s_mathSystem = new MathTestSystem();
        s_movementSystem = new MovementSystem();
        s_projectileSystem = new ProjectileMovementSystem();
        s_spawner = new EntitySpawner();
        s_playerController = new PlayerControllerSystem();
        s_imguiSystem = new ImGuiSystem();
        s_mainMenu = new MainMenuSystem();
        s_gsm = new GameStateManager();

        LOG_INFO("CORE", "[Bootstrapper] All systems created");
    }

    void EngineBootstrapper::WireDependencies()
    {
        LOG_INFO("CORE", "[Bootstrapper] Wiring system dependencies...");

        // Wire EntityManager to systems that need it
        s_movementSystem->SetEntityManager(s_entityManager);
        s_projectileSystem->SetEntityManager(s_entityManager);
        s_graphicsSystem->SetEntityManager(s_entityManager);
        s_collisionSystem->SetEntityManager(s_entityManager);
        s_spawner->SetEntityManager(s_entityManager);
        s_playerController->SetEntityManager(s_entityManager);
        s_imguiSystem->SetEntityManager(s_entityManager);
        s_mainMenu->SetEntityManager(s_entityManager);
        s_gsm->SetEntityManager(s_entityManager);

        // Wire InputSystem to systems that need it
        s_playerController->SetInputSystem(s_inputSystem);
        s_movementSystem->SetInputSystem(s_inputSystem);
        s_collisionSystem->SetInput(s_inputSystem);

        // Wire GraphicsSystem
        s_mainMenu->SetGraphics(s_graphicsSystem);

        // Wire GameStateManager
        s_gsm->SetMainMenuSystem(s_mainMenu);
        s_gsm->SetEntitySpawner(s_spawner);
        s_gsm->SetConfigPath(s_config.gameConfigPath);

        // Setup MainMenu callback for state transitions
        s_mainMenu->SetOnPlayCallback([]() {
            s_gsm->ChangeState(GameState::Level1);
            });

        LOG_INFO("CORE", "[Bootstrapper] Dependencies wired");
    }

    void EngineBootstrapper::InitializeCriticalSystems()
    {
        LOG_INFO("CORE", "[Bootstrapper] Initializing critical systems...");

        // Window must be initialized first
        s_windowSystem->Initialize();

        // Graphics needs window handle
        s_graphicsSystem->SetWindow(s_windowSystem->GetWindow());
        s_graphicsSystem->Initialize();

        // ImGui needs window handle
        if (s_config.enableImGui) {
            s_imguiSystem->SetWindow(s_windowSystem->GetWindow());
            s_imguiSystem->SetEntitySpawner(s_spawner);
        }

        // MainMenu needs window handle
        s_mainMenu->SetWindow(s_windowSystem->GetWindow());

        // PlayerController needs window handle
        s_playerController->SetWindow(s_windowSystem->GetWindow());

        LOG_INFO("CORE", "[Bootstrapper] Critical systems initialized");
    }

    void EngineBootstrapper::RegisterSystems(CoreEngine& engine)
    {
        LOG_INFO("CORE", "[Bootstrapper] Registering systems with engine...");

        // Order matters! Add systems in the order they should be updated
        engine.AddSystem(s_windowSystem);
        engine.AddSystem(s_gsm);                // GSM early for state management
        engine.AddSystem(s_inputSystem);
        engine.AddSystem(s_spawner);
        engine.AddSystem(s_playerController);
        engine.AddSystem(s_movementSystem);
        engine.AddSystem(s_collisionSystem);
        engine.AddSystem(s_mathSystem);
        engine.AddSystem(s_projectileSystem);
        engine.AddSystem(s_mainMenu);
        engine.AddSystem(s_graphicsSystem);

        if (s_config.enableImGui) {
            engine.AddSystem(s_imguiSystem);
        }

        LOG_INFO("CORE", "[Bootstrapper] All systems registered");
    }

} // namespace Framework