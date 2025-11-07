/*
===============================================================================
 File:          Core.cpp
 Author:        GE YONGQI
 Email:         yongqi.ge@digipen.edu
 Date:          2025-10-31
 Contribution:  100%
 ------------------------------------------------------------------------------
  Core engine manager (implementation)

  Design notes:
     - Integrates initialization, update, and cleanup of all subsystems
     - Provides a one-click startup routine via InitializeAllSystems()
     - Uses dependency wiring to ensure correct order and relationships
     - Automatically logs progress and errors through the Log system

  Thread-safety:
     - Not thread-safe (single-threaded engine model)
     - All systems created and destroyed on the same thread
===============================================================================
*/


#include "Precompiled.h"

#include "ProjectileSystem.h"        
#include "EntitySpawner.h"
#include "PlayerManager.h"           
#include "ImguiSystem.h"  
#include "Event/Event.h"
#include "Event/DamageIndicatorSystem.h"

#include "Grid/Grid.h"
#include "Grid/GridECS.h"
#include "Grid/GridTile.h"
#include "Pathfinding/Pathfinding.h"


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
        , animationSystem(nullptr)
        , uiSystem(nullptr)
        , eventSystem(nullptr)
        , LastTime(0)
        , GameActive(true)
        , damageIndicator(nullptr)
        , pathfindingSystem(nullptr)
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
            // Create all systems
            CreateAllSystems();

            // Connect system dependencies
            WireSystemDependencies();

            // Initialize key systems
            InitializeCriticalSystems();

            // Add the system to the engine
            AddSystemsToEngine();

            // Initialize the remaining systems
            Initialize();

            //Initializes the subscribers to receive events
            SetupEventListeners();

            //Use a container to store(future)
            if (audioSystem) {
                LOG_INFO("CORE", "Loading test audio...");
                bool imguitestaudio = audioSystem->LoadSound("assets/leaves.wav", "leaves");
                bool mainmenubgm = audioSystem->LoadSound("assets/Moron3MenuMusic.wav", "mmbgm");
                bool bgm2 = audioSystem->LoadSound("assets/Moron3BGM.wav", "bgm");
                bool shooting = audioSystem->LoadSound("assets/shooting.wav", "shooting");
                if (imguitestaudio && mainmenubgm && bgm2) {
                    LOG_INFO("CORE", "All audio loaded successfully");
                }
                else {
                    LOG_WARN("CORE", "Failed to load audio");
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
        animationSystem = new AnimationSystem();
        uiSystem = new UISystem(this);
        eventSystem = new EventSystem();
        damageIndicator = new DamageIndicatorSystem();
        pathfindingSystem = new PathfindingSystem();

        LOG_INFO("CORE", " All systems created");
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
        animationSystem->SetEntityManager(entityManager);
        pathfindingSystem->SetEntityManager(entityManager);

       

        // Wire InputSystem
        playerController->SetInputSystem(inputSystem);
        movementSystem->SetInputSystem(inputSystem);
        collisionSystem->SetInput(inputSystem);
        playerController->SetEntitySpawner(spawner);

        // Wire AudioSystem to ImGuiSystem
        imguiSystem->SetAudioSystem(audioSystem);
        imguiSystem->SetGraphicsSystem(graphicsSystem);

        // Wire Event System
        projectileSystem->SetEventSystem(eventSystem);

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
        inputSystem->SetWindow(windowSystem->GetWindow());  

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
        AddSystem(animationSystem);
        AddSystem(uiSystem);
        AddSystem(eventSystem);
        AddSystem(pathfindingSystem);


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
            // Skip already initialized systems
            if (dynamic_cast<WindowSystem*>(system) != nullptr ||
                dynamic_cast<GraphicsSystemV2*>(system) != nullptr)
            {
                continue;
            }

            system->Initialize();
        }

        LOG_INFO("CORE", "All systems initialized");
    }

    // ========================================================================
    // Event Subscribers
   // ========================================================================

    void CoreEngine::SetupEventListeners() {
        LOG_INFO("CORE", "Setting up event listeners...");

        if (!eventSystem || !damageIndicator) {
            LOG_WARN("CORE", "EventSystem or DamageIndicator is null, skipping listener setup");
            return;
        }

        // *** THIS IS THE CRITICAL SUBSCRIPTION STEP ***

        // 1. Register for damage events (ENEMY_DAMAGED)
        eventSystem->RegisterObserver(Framework::MessageIds::enemyDamaged, damageIndicator);
        LOG_INFO("CORE", "Registered DamageIndicator for ENEMY_DAMAGED events");

        // 2. Register for death events (ENEMY_DEATH)
        eventSystem->RegisterObserver(Framework::MessageIds::enemyDeath, damageIndicator);
        LOG_INFO("CORE", "Registered DamageIndicator for ENEMY_DEATH events");
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
        animationSystem = nullptr;
        uiSystem = nullptr;
        eventSystem = nullptr;

        // Delete EntityManager (not added to engine)
        if (entityManager) {
            delete entityManager;
            entityManager = nullptr;
        }

        if (damageIndicator) {
            delete damageIndicator;
            damageIndicator = nullptr;
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
        DBG_SCOPE_SYS("CoreEngine Frame", eng::debug::Subsystem::Engine);

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