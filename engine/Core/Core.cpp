/**
===============================================================================
File: Core.cpp (Enhanced - Integration Version)
Author: GE YONGQI
-------------------------------------------------------------------------

Improvements:
- Integrate InitializeEngineSystems() into InitializeAllSystems()
- Integrate CleanupEngineSystems() into Cleanup()
- Implement ShouldWindowClose()
- Implement UpdateSingleFrame()
===============================================================================
 */

#include "Precompiled.h"
#include "Core.h"
#include "MovementSystem.h"
#include "CollisionSystem.h"
#include "ProjectileSystem.h"        
#include "ECSEntityManager.h"
#include "EntitySpawner.h"
#include "PlayerManager.h"           
#include "ImguiSystem.h"            
#include "WindowSystem.h"
#include "GraphicsSystemV2.h"
#include "Input.h"
#include "AudioSystem.h"

 /*
 #include "PerfViewer.h"
 #include "Trace.h"
 #include "Perf.h"
 #include "Log.h"
 #include "CrashLogger.h"
 */

namespace Framework
{
    CoreEngine* CORE = nullptr;

    CoreEngine::CoreEngine()
        : entityManager(nullptr)
        , windowSystem(nullptr)
        , graphicsSystem(nullptr)
        , inputSystem(nullptr)
        , collisionSystem(nullptr)
        , movementSystem(nullptr)
        , projectileSystem(nullptr)
        , spawner(nullptr)
        , playerController(nullptr)
        , imguiSystem(nullptr)
        , audioSystem(nullptr)
        , LastTime(0)
        , GameActive(true)
    {
        CORE = this;
    }

    CoreEngine::~CoreEngine()
    {
    }

    // ========================================================================
    // Initialize all systems with one click
    // ========================================================================

    bool CoreEngine::InitializeAllSystems()
    {
        LOG_INFO("CORE", "================================================");
        LOG_INFO("CORE", " CoreEngine: Initializing All Systems");
        LOG_INFO("CORE", "================================================");

        try {
            // Step 1: Create all systems
            CreateAllSystems();

            // Step 2: Connect system dependencies
            WireSystemDependencies();

            // Step 3: Initialize key systems
            InitializeCriticalSystems();

            // Step 4: Add the system to the engine
            AddSystemsToEngine();

            // Step 5: Initialize the remaining systems
            Initialize();

            if (audioSystem) {
                LOG_INFO("CORE", "Loading test audio...");
                bool loaded = audioSystem->LoadSound("assets/leaves.wav", "leaves");
                if (loaded) {
                    LOG_INFO("CORE", "✓ Test audio 'leaves' loaded successfully");
                }
                else {
                    LOG_WARN("CORE", "✗ Failed to load test audio");
                }
            }


            LOG_INFO("CORE", "================================================");
            LOG_INFO("CORE", " CoreEngine: All Systems Ready!");
            LOG_INFO("CORE", "================================================");

            return true;
        }
        catch (const std::exception& e) {
            LOG_ERROR("CORE", "Failed to initialize systems: %s", e.what());
            return false;
        }
    }

    void CoreEngine::CreateAllSystems()
    {
        LOG_INFO("CORE", "[1/5] Creating engine systems...");

        entityManager = new EntityManager();
        windowSystem = new WindowSystem();
        graphicsSystem = new GraphicsSystemV2();
        inputSystem = new InputSystem();
        collisionSystem = new CollisionSystem();
        movementSystem = new MovementSystem();
        projectileSystem = new ProjectileMovementSystem();
        spawner = new EntitySpawner();
        playerController = new PlayerControllerSystem();
        imguiSystem = new ImGuiSystem();
        audioSystem = new AudioSystem();

        LOG_INFO("CORE", "  ✓ All systems created");
    }

    void CoreEngine::WireSystemDependencies()
    {
        LOG_INFO("CORE", "[2/5] Wiring system dependencies...");

        // Wire EntityManager
        movementSystem->SetEntityManager(entityManager);
        projectileSystem->SetEntityManager(entityManager);
        graphicsSystem->SetEntityManager(entityManager);
        collisionSystem->SetEntityManager(entityManager);
        spawner->SetEntityManager(entityManager);
        playerController->SetEntityManager(entityManager);
        imguiSystem->SetEntityManager(entityManager);
        audioSystem->SetEntityManager(entityManager);

        // Wire InputSystem
        playerController->SetInputSystem(inputSystem);
        movementSystem->SetInputSystem(inputSystem);
        collisionSystem->SetInput(inputSystem);
        playerController->SetEntitySpawner(spawner);

        // Wire AudioSystem to ImGuiSystem
        imguiSystem->SetAudioSystem(audioSystem);

        LOG_INFO("CORE", "Dependencies wired");
    }

    void CoreEngine::InitializeCriticalSystems()
    {
        LOG_INFO("CORE", "[3/5] Initializing critical systems...");

        // Initialize WindowSystem first
        windowSystem->Initialize();
        LOG_INFO("CORE", "WindowSystem initialized");

        // Set window dependencies
        graphicsSystem->SetWindow(windowSystem->GetWindow());
        imguiSystem->SetWindow(windowSystem->GetWindow());
        imguiSystem->SetEntitySpawner(spawner);
        playerController->SetWindow(windowSystem->GetWindow());
        LOG_INFO("CORE", "Window dependencies set");

        // Initialize GraphicsSystem
        graphicsSystem->Initialize();
        LOG_INFO("CORE", "GraphicsSystem initialized");
    }

    void CoreEngine::AddSystemsToEngine()
    {
        LOG_INFO("CORE", "[4/5] Adding systems to engine...");

        // Add in specific order
        AddSystem(windowSystem);
        AddSystem(inputSystem);
        AddSystem(spawner);
        AddSystem(playerController);
        AddSystem(movementSystem);
        AddSystem(collisionSystem);
        AddSystem(projectileSystem);
        AddSystem(graphicsSystem);
        AddSystem(imguiSystem);
        AddSystem(audioSystem);

        LOG_INFO("CORE", "%zu systems added", Systems.size());
    }

    // ========================================================================
    // Original Initialize (initialize the remaining systems)
    // ========================================================================

    void CoreEngine::Initialize()
    {
        LOG_INFO("CORE", "[5/5] Initializing remaining systems...");

        // WindowSystem and GraphicsSystem already initialized
        // Just initialize the others
        for (auto system : Systems)
        {
            system->Initialize();
        }

        LOG_INFO("CORE", "All systems initialized");
    }

    // ========================================================================
    // Clean all systems
   // ========================================================================

    void CoreEngine::Cleanup()
    {
        LOG_INFO("CORE", "================================================");
        LOG_INFO("CORE", " CoreEngine: Cleaning Up");
        LOG_INFO("CORE", "================================================");

        // Stop all audio before destroying systems
        if (audioSystem) {
            LOG_INFO("CORE", "Stopping all audio...");
            audioSystem->StopAllSounds();
        }

        // Destroy all systems added to engine
        DestroySystems();

        // Clear system pointers
        windowSystem = nullptr;
        inputSystem = nullptr;
        spawner = nullptr;
        playerController = nullptr;
        movementSystem = nullptr;
        collisionSystem = nullptr;
        projectileSystem = nullptr;
        graphicsSystem = nullptr;
        imguiSystem = nullptr;
        audioSystem = nullptr;

        // Delete EntityManager (not added to engine)
        if (entityManager) {
            delete entityManager;
            entityManager = nullptr;
        }

        // Terminate GLFW
        glfwTerminate();

        LOG_INFO("CORE", "Cleanup complete");
    }

    // ========================================================================
    // Check if the window is closed
    // ========================================================================

    bool CoreEngine::ShouldWindowClose() const
    {
        if (!windowSystem) return false;
        return windowSystem->ShouldClose();
    }

    // ========================================================================
    // Single frame update (GSM friendly)
    // ========================================================================

    void CoreEngine::UpdateSingleFrame(float dt)
    {
        // Check if window should close
        if (ShouldWindowClose()) {
            GameActive = false;
            Message quitMsg(Status::Quit);
            BroadcastMessage(&quitMsg);
            return;
        }

        // Update all systems
        for (unsigned i = 0; i < Systems.size(); ++i)
        {
            Systems[i]->Update(dt);
        }
    }

    void CoreEngine::BroadcastMessage(Message* message)
    {
        if (message->MessageId == Status::Quit)
            GameActive = false;

        for (unsigned i = 0; i < Systems.size(); ++i)
            Systems[i]->SendEngineMessage(message);
    }

    void CoreEngine::AddSystem(EngineSystem* system)
    {
        Systems.push_back(system);
    }

    void CoreEngine::DestroySystems()
    {
        for (unsigned i = 0; i < Systems.size(); ++i)
        {
            delete Systems[Systems.size() - i - 1];
        }
        Systems.clear();
    }
}