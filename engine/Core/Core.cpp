/*
===============================================================================
 File:           Core.cpp
 Author:         ETHAN NG YONG LE
 Email:          n.ethanyongle@digipen.edu
 Date:           2025-09-30
 Contribution:   100%
 ------------------------------------------------------------------------------
 
  Design notes:
  This file implements the CoreEngine class which serves as the central
 * orchestrator for the game engine. It manages the main game loop, system
 * initialization and updates, message broadcasting, and frame timing.
===============================================================================
 */
 
#include "Precompiled.h"
#include "Core.h"
#include "MovementSystem.h"
#include "PerfViewer.h"
#include "Trace.h"
#include "Perf.h"
#include "Log.h"
#include "CrashLogger.h"
#include "ImguiSystem.h"

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
        , LastTime(0)
        , GameActive(true)
    {
        LastTime = 0;
        GameActive = true;
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

            if (audioSystem) {
                LOG_INFO("CORE", "Loading test audio...");
                bool loaded = audioSystem->LoadSound("assets/leaves.wav", "leaves");
                if (loaded) {
                    LOG_INFO("CORE", "Test audio 'leaves' loaded successfully");
                }
                else {
                    LOG_WARN("CORE", "Failed to load test audio");
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
        AddSystem(animationSystem);
        AddSystem(uiSystem);

        LOG_INFO("CORE", "%zu systems added", Systems.size());
    }

    // ========================================================================
    // Original Initialize (initialize the remaining systems)
    // ========================================================================

    void CoreEngine::Initialize()
    {
        // 1. First initialize WindowSystem
        for (auto system : Systems)
        {
            if (auto windowSystem = dynamic_cast<WindowSystem*>(system))
            {
                windowSystem->Initialize();
            }
        }

        // 2. Get window handle
        GLFWwindow* glfwWin = nullptr;
        for (auto system : Systems)
        {
            if (auto windowSystem = dynamic_cast<WindowSystem*>(system))
            {
                glfwWin = windowSystem->GetWindow();
                break;
            }
        }

        // 3. Initialize all other systems
        for (auto system : Systems)
        {
            if (dynamic_cast<WindowSystem*>(system) == nullptr)
            {
                system->Initialize();
            }
        }
    }

    void CoreEngine::GameLoop()
    {
        LastTime = timeGetTime();

        eng::debug::FpsCounter fps;
        fps.set_enable_logging(true);

        // Attach window title updater
        for (auto system : Systems)
        {
            if (auto windowSystem = dynamic_cast<WindowSystem*>(system))
            {
                GLFWwindow* win = windowSystem->GetWindow();
                fps.set_title_updater([win](const char* title) {
                    glfwSetWindowTitle(win, title);
                    });
            }
        }

        // Find ImGui and Graphics systems
        ImGuiSystem* imguiSys = nullptr;
        GraphicsSystemV2* graphicsSys = nullptr;

        for (auto system : Systems)
        {
            if (auto imgui = dynamic_cast<ImGuiSystem*>(system)) {
                imguiSys = imgui;
            }
            if (auto graphics = dynamic_cast<GraphicsSystemV2*>(system)) {
                graphicsSys = graphics;
            }
        }

        while (GameActive)
        {
            // Check if window should close
            for (auto system : Systems) {
                if (auto windowSystem = dynamic_cast<WindowSystem*>(system)) {
                    if (windowSystem->ShouldClose()) {
                        Message quitMsg(Status::Quit);
                        BroadcastMessage(&quitMsg);
                    }
                }
            }

            // Calculate delta time
            unsigned currenttime = timeGetTime();
            float dt = (currenttime - LastTime) / 1000.0f;
            if (dt < 0.001f) dt = 0.016f;
            LastTime = currenttime;

            // Begin perf frame
            eng::debug::PerfViewer::begin_frame();

            // ========================================================
            // Update all systems
            // ========================================================
            for (unsigned i = 0; i < Systems.size(); ++i)
            {
                eng::debug::Subsystem tag =
                    (i == 0) ? eng::debug::Subsystem::Graphics :
                    (i == 1) ? eng::debug::Subsystem::Gameplay :
                    eng::debug::Subsystem::Other;

                Systems[i]->Update(dt);
                (void)tag;
            }

            // ========================================================
            // FIXED: Render ImGui AFTER all systems (OUTSIDE loop)
            // ========================================================
            if (imguiSys) {
                imguiSys->Render();
            }

            // Swap buffers AFTER ImGui
            if (graphicsSys) {
                graphicsSys->RenderImGui();
            }

            // End perf frame
            eng::debug::PerfViewer::end_frame();
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

            // FPS counter
            fps.tick_with_dt(static_cast<double>(dt));

            // CSV export (F2)
            if (GetAsyncKeyState(VK_F2) & 0x0001)
            {
                eng::debug::PerfViewer::export_csv("perf_recent.csv");
            }

            // Crash test (F3)
            if (GetAsyncKeyState(VK_F3) & 0x0001) {
                eng::debug::CrashLogger::force_crash_for_test();
            }
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