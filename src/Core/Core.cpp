/*
===============================================================================
 File:          Core.cpp (FIXED - ALT+TAB Text Rendering Issue)
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

  BUGFIX (2025-11-28):
     - Fixed ALT+TAB text disappearing issue in UpdateSingleFrame()
     - Removed early return when paused to ensure LevelLoader::DrawCurrentLevel()
       is always called, which is needed for main menu text rendering
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

#include "Grid/Grid.h"
#include "Grid/GridECS.h"
#include "Grid/GridTile.h"
#include "Pathfinding/Pathfinding.h"
#include "AudioLoader.h"
#include "Pause/Pause.h"
#include "RangeIndicatorSystem.h"
#include "GlobalPauseManager.h"

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
        , rangeIndicatorSystem(nullptr)
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

            // Load audio configuration from JSON
            if (audioSystem) {
                LOG_INFO("CORE", "Loading audio configuration...");
                bool audioLoaded = AudioLoader::LoadAudioConfig("assets/JSON/AudioConfig.json", audioSystem);
                if (audioLoaded) {
                    LOG_INFO("CORE", "✓ Audio configuration loaded successfully");
                }
                else {
                    LOG_WARN("CORE", "✗ Failed to load audio configuration");
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
        scriptSystem = new ScriptSystem();
        rangeIndicatorSystem = new RangeIndicatorSystem();

        // Check for allocation failures
        if (!entityManager || !windowSystem || !graphicsSystem || !inputSystem ||
            !collisionSystem || !movementSystem || !projectileSystem || !spawner ||
            !playerController || !imguiSystem || !audioSystem || !animationSystem ||
            !uiSystem || !eventSystem || !damageIndicator || !pathfindingSystem ||
            !scriptSystem || !rangeIndicatorSystem) {

            LOG_ERROR("CORE", "Failed to allocate one or more systems!");

            // Clean up any successfully allocated systems
            delete entityManager;
            delete windowSystem;
            delete graphicsSystem;
            delete inputSystem;
            delete collisionSystem;
            delete movementSystem;
            delete projectileSystem;
            delete spawner;
            delete playerController;
            delete imguiSystem;
            delete audioSystem;
            delete animationSystem;
            delete uiSystem;
            delete eventSystem;
            delete damageIndicator;
            delete pathfindingSystem;
            delete scriptSystem;
            delete rangeIndicatorSystem;

            throw std::runtime_error("System allocation failure");
        }

        scriptSystem->SetEntityManager(entityManager);
        scriptSystem->SetCoreEngine(this);
        scriptSystem->Initialize();
        LevelLoader::GetInstance().Initialize(this);
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

        // Wire AudioSystem
        playerController->SetAudioSystem(audioSystem);  // Fix: PlayerController needs AudioSystem!
        pathfindingSystem->SetAudioSystem(audioSystem);  // Enemy walking sounds
        imguiSystem->SetAudioSystem(audioSystem);
        imguiSystem->SetGraphicsSystem(graphicsSystem);

        // Load master volume from audio config JSON and apply it
        float masterVolume = AudioLoader::GetSettings().masterVolume;
        audioSystem->SetMasterVolume(masterVolume);
        LOG_INFO("AUDIO", "Master volume loaded from audio_config.json: %.2f", masterVolume);

        // Wire Event System
        projectileSystem->SetEventSystem(eventSystem);

        // Wire Range Indicator
        rangeIndicatorSystem->SetEntityManager(entityManager);
        rangeIndicatorSystem->SetGraphicsSystem(graphicsSystem);

        // Pause Event System
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
        AddSystem(rangeIndicatorSystem);

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

// ========================================================================
    // Single frame update (GSM friendly) - MODIFIED FOR EDITOR MODE
    // ========================================================================

// ========================================================================
    // Single frame update (GSM friendly) - FIXED VERSION
    // ========================================================================
    //
    // MODIFICATIONS:
    // 1. Added IsPlaying() check for editor mode support
    // 2. Animations only update in play mode  
    // 3. Removed duplicate graphicsSystem->Update() calls in render section
    // 4. Audio and rendering always update
    //
// ========================================================================
    // Single frame update - VERSION WITH EDITOR MODE FREEZE
    // ========================================================================
    //
    // This version:
    // - Checks isEditorMode in game logic condition
    // - Game freezes when F1 is pressed (EditorMode = true)
    // - Buttons are disabled and grayed out in editor mode
    // - Displays "EDITOR MODE" message
    //
    void CoreEngine::UpdateSingleFrame(float dt)
    {
        if (ShouldWindowClose()) {
            GameActive = false;
            return;
        }

        // ====================================================================
        // CHECK GAME STATE
        // ====================================================================
        bool isPaused = GlobalPause::IsPaused();
        bool isPlaying = IsPlaying();
        bool isEditorMode = IsEditorMode();  // ✅ F1 editor mode check

        // ====================================================================
        // ALWAYS UPDATE
        // ====================================================================

        // Input - needed for ImGui and pause menu
        if (inputSystem) {
            inputSystem->Update(dt);
        }

        // Graphics - always update to render current state
        if (graphicsSystem) {
            graphicsSystem->Update(dt);
        }

        // UI System - needed for pause menu
        if (uiSystem) {
            uiSystem->Update(dt);
        }

        // ====================================================================
        // CONDITIONALLY UPDATE GAME LOGIC
        // ====================================================================
        // Game logic updates when ALL of these are true:
        // - isPlaying = true (PLAY button clicked)
        // - isPaused = false (not paused with P key)
        // - isEditorMode = false (F1 not pressed)
        //
        // ✅ EditorMode has HIGHEST priority - when F1 is pressed, game freezes
        // ====================================================================

        if (isPlaying && !isPaused && !isEditorMode) {  // ✅ Check all three!
            // ================================================================
            // GAME IS RUNNING
            // ================================================================

            // Movement & Physics
            if (movementSystem) {
                movementSystem->Update(dt);
            }

            if (projectileSystem) {
                projectileSystem->Update(dt);
            }

            if (collisionSystem) {
                collisionSystem->Update(dt);
            }

            // Game Logic
            if (scriptSystem) {
                scriptSystem->Update(dt);
            }

            // DISABLED: C++ PlayerController - using Lua PlayerScript.lua instead
            // if (playerController) {
            //     playerController->Update(dt);
            // }

            // DISABLED: C++ PathfindingSystem - using Lua EnemyScript.lua instead
            // if (pathfindingSystem) {
            //     pathfindingSystem->Update(dt);
            // }

            // Animation
            if (animationSystem) {
                animationSystem->Update(dt);
            }

            // Events & Indicators
            if (eventSystem) {
                eventSystem->Update(dt);
            }

            if (rangeIndicatorSystem) {
                rangeIndicatorSystem->Update(dt);
            }

            // Update all logic systems
            for (unsigned i = 0; i < Systems.size(); ++i) {
                if (dynamic_cast<GraphicsSystemV2*>(Systems[i]) ||
                    dynamic_cast<ImGuiSystem*>(Systems[i]) ||
                    dynamic_cast<InputSystem*>(Systems[i]) ||
                    dynamic_cast<AnimationSystem*>(Systems[i])) {
                    continue;
                }
                Systems[i]->Update(dt);
            }
        }
        else {
            // ================================================================
            // GAME IS FROZEN
            // ================================================================
            if (isEditorMode) {
                LOG_DEBUG("CORE", "Editor mode active - game frozen");
            }
            else if (!isPlaying) {
                LOG_DEBUG("CORE", "Not playing");
            }
            else if (isPaused) {
                LOG_DEBUG("CORE", "Game paused");
            }
        }

        if (audioSystem) {
            audioSystem->Update(dt);
        }


        // ====================================================================
        // RENDER GAME (Always render)
        // ====================================================================

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

            graphicsSystem->GetTextRenderer().setScreenSize(
                imguiSystem->GetViewportWidth(),
                imguiSystem->GetViewportHeight()
            );

            LevelLoader::GetInstance().DrawCurrentLevel();

            graphicsSystem->ClearRenderTarget();

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            int w, h;
            glfwGetFramebufferSize(windowSystem->GetWindow(), &w, &h);
            glViewport(0, 0, w, h);
            glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        }
        else {
            graphicsSystem->Update(dt);

            int winWidth, winHeight;
            glfwGetWindowSize(windowSystem->GetWindow(), &winWidth, &winHeight);
            graphicsSystem->GetTextRenderer().setScreenSize(winWidth, winHeight);

            LevelLoader::GetInstance().DrawCurrentLevel();
        }

        // === IMGUI ===
        if (imguiSystem) {
            imguiSystem->Update(dt);
            if (imguiSystem->IsEnabled()) {
                imguiSystem->Render();
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
        if (scriptSystem) {
            scriptSystem->Shutdown();
            delete scriptSystem;
            scriptSystem = nullptr;
        }
        Systems.clear();
    }
}