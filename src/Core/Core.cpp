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
#include "ScriptSystem.h"
#include "LevelLoader.h"

#include "Grid/Grid.h"
#include "Grid/GridECS.h"
#include "Grid/GridTile.h"
#include "Pathfinding/Pathfinding.h"
#include "AudioLoader.h"
#include "Pause/Pause.h"

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
		, pauseSystem(nullptr)
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
                LOG_INFO("CORE", "Loading audio from configuration...");
                bool success = AudioLoader::LoadAudioConfig("assets/scripts/JSON/AudioConfig.json", audioSystem);

                if (success) {
                    LOG_INFO("CORE", "All audio loaded successfully from config");
                }
                else {
                    LOG_WARN("CORE", "Failed to load audio configuration");
                }
            }


            LOG_INFO("CORE", "================================================");
            LOG_INFO("CORE", " CoreEngine: All Systems Ready!");
            LOG_INFO("CORE", "================================================");

            LevelLoader::GetInstance().Initialize(this);

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
        scriptSystem = new ScriptSystem();
		pauseSystem = new PauseSystem();
        scriptSystem->SetEntityManager(entityManager);
        scriptSystem->SetCoreEngine(this);
        scriptSystem->Initialize();
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
        graphicsSystem->SetInputSystem(inputSystem);
        collisionSystem->SetInput(inputSystem);
        playerController->SetEntitySpawner(spawner);

        // Wire AudioSystem to ImGuiSystem
        imguiSystem->SetAudioSystem(audioSystem);
        imguiSystem->SetGraphicsSystem(graphicsSystem);

        // Wire Event System
        projectileSystem->SetEventSystem(eventSystem);

        // Pause Event System
        pauseSystem->SetCoreEngine(this);
        LOG_INFO("CORE", "PauseSystem wired to CoreEngine");

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
        AddSystem(pauseSystem);


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
        char exePath[MAX_PATH];
        GetModuleFileNameA(NULL, exePath, MAX_PATH);

        LOG_INFO("Core", "================================================");
        LOG_INFO("Core", "EXECUTABLE LOCATION:");
        LOG_INFO("Core", "  %s", exePath);
        LOG_INFO("Core", "================================================");

        // Get just the directory
        std::filesystem::path p(exePath);
        auto exeDir = p.parent_path();
        LOG_INFO("Core", "EXE Directory: %s", exeDir.string().c_str());

        // Check if assets folder exists
        auto assetsPath = exeDir / "assets";
        if (std::filesystem::exists(assetsPath)) {
            LOG_INFO("Core", "[OK] assets/ folder found");

            // Check if JSON exists
            auto jsonPath = assetsPath / "scripts" / "JSON" / "levelselect_config.json";
            if (std::filesystem::exists(jsonPath)) {
                LOG_INFO("Core", "[OK] JSON file found at:");
                LOG_INFO("Core", "  %s", jsonPath.string().c_str());

                // CRITICAL: Show actual file contents
                std::ifstream file(jsonPath);
                std::stringstream buffer;
                buffer << file.rdbuf();
                LOG_INFO("Core", "");
                LOG_INFO("Core", "CURRENT FILE CONTENTS:");
                LOG_INFO("Core", "------------------------------------------------");
                LOG_INFO("Core", "%s", buffer.str().c_str());
                LOG_INFO("Core", "------------------------------------------------");
            }
            else {
                LOG_ERROR("Core", "[ERROR] JSON file NOT FOUND at:");
                LOG_ERROR("Core", "  %s", jsonPath.string().c_str());
            }
        }
        else {
            LOG_ERROR("Core", "[ERROR] assets/ folder NOT FOUND!");
        }

        LOG_INFO("Core", "================================================");

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

        LOG_INFO("CORE", "Shutting down LevelLoader...");
        LevelLoader::GetInstance().Shutdown();

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
        pauseSystem = nullptr;

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
        if (ShouldWindowClose()) {
            GameActive = false;
            return;
        }

        // Update all logic systems
        for (unsigned i = 0; i < Systems.size(); ++i) {
            if (dynamic_cast<GraphicsSystemV2*>(Systems[i]) ||
                dynamic_cast<ImGuiSystem*>(Systems[i])) {
                continue;
            }
            Systems[i]->Update(dt);
        }

        if (scriptSystem) scriptSystem->Update(dt);

        // === RENDER GAME ===
        bool useViewport = imguiSystem &&
            imguiSystem->IsEnabled() &&
            imguiSystem->IsRenderingToViewport() &&
            imguiSystem->GetViewportFBO() != 0 &&
            imguiSystem->GetViewportWidth() >= 100 &&
            imguiSystem->GetViewportHeight() >= 100;

        if (useViewport) {
            // Render game to viewport texture
            graphicsSystem->SetRenderTarget(
                imguiSystem->GetViewportFBO(),
                imguiSystem->GetViewportWidth(),
                imguiSystem->GetViewportHeight()
            );
            graphicsSystem->Update(dt);
            glViewport(0, 0, imguiSystem->GetViewportWidth(), imguiSystem->GetViewportHeight());

            // Setup GL state for text
            glDisable(GL_DEPTH_TEST);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            // ========================================
            // Draw level text INTO the viewport
            // ========================================
            graphicsSystem->GetTextRenderer().setScreenSize(
                imguiSystem->GetViewportWidth(),
                imguiSystem->GetViewportHeight()
            );
            std::cout << "[Core] Drawing text to viewport: "
                << imguiSystem->GetViewportWidth() << "x"
                << imguiSystem->GetViewportHeight() << "\n";
            LevelLoader::GetInstance().DrawCurrentLevel();
            graphicsSystem->DrawText4("Sans48", "TEST", 50.0f, 50.0f, 1.0f, glm::vec3(1.0f, 0.0f, 0.0f));
            // ========================================

            graphicsSystem->ClearRenderTarget();

            // Clear screen for ImGui
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            int w, h;
            glfwGetFramebufferSize(windowSystem->GetWindow(), &w, &h);
            glViewport(0, 0, w, h);
            glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        }
        else {
            // Normal rendering
            graphicsSystem->Update(dt);

            // ========================================
            // Draw level text for non-viewport mode
            // ========================================
            int fbW, fbH;
            glfwGetFramebufferSize(windowSystem->GetWindow(), &fbW, &fbH);
            graphicsSystem->GetTextRenderer().setScreenSize(fbW, fbH);
            LevelLoader::GetInstance().DrawCurrentLevel();
            // ========================================
        }

        // === IMGUI ===
        if (imguiSystem) {
            imguiSystem->Update(dt);
            if (imguiSystem->IsEnabled()) {
                imguiSystem->Render();
            }
            if (engine->GetGraphicsSystem()) {
                engine->GetGraphicsSystem()->RenderImGui();
            }
        }

        glfwSwapBuffers(windowSystem->GetWindow());
        glfwPollEvents();
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
        if (scriptSystem) {
            scriptSystem->Shutdown();
            delete scriptSystem;
            scriptSystem = nullptr;
        }
        Systems.clear();
    }
}