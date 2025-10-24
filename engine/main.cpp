/**
===============================================================================
 File:           main.cpp
 Author:         GE YONGQI
 Description:    School template style(GSM) with ECS integration
===============================================================================
 */

#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "Precompiled.h"
#include "GSM/GameStateManager.h"
#include <ProjectileSystem.h>
#include <ImguiSystem.h>
#include <PlayerManager.h>
#include <EntitySpawner.h>

 // ============================================================================
 // GLOBAL VARIABLES (School Template Style)
 // ============================================================================
extern int current, previous, next;
extern FP fpLoad, fpInitialize, fpUpdate, fpDraw, fpFree, fpUnload;

// ============================================================================
// ENGINE SYSTEMS (Global for easy access in state functions)
// ============================================================================
namespace Global {
    auto* entityManager = new Framework::EntityManager();
    auto * windowSystem = new Framework::WindowSystem();
    auto * graphicsSystem = new Framework::GraphicsSystemV2();
    auto * inputSystem = new Framework::InputSystem();
    auto * collisionSystem = new Framework::CollisionSystem();
    auto * movementSystem = new Framework::MovementSystem();
    auto * projectileSystem = new Framework::ProjectileMovementSystem();
    auto * spawner = new Framework::EntitySpawner();
    auto * playerController = new Framework::PlayerControllerSystem();
    auto * imguiSystem = new Framework::ImGuiSystem();
    auto * engine = new Framework::CoreEngine();
}

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

void InitializeEngineSystems()
{
    LOG_INFO("CORE", "[Init] Creating engine systems...");

    // Create entity manager
    Global::entityManager = new Framework::EntityManager();

    // Create all systems
    Global::windowSystem = new Framework::WindowSystem();
    Global::graphicsSystem = new Framework::GraphicsSystemV2();
    Global::inputSystem = new Framework::InputSystem();
    Global::collisionSystem = new Framework::CollisionSystem();
    Global::movementSystem = new Framework::MovementSystem();


    LOG_INFO("CORE", "[Init] Wiring system dependencies...");

    // Wire EntityManager to systems
    Global::movementSystem->SetEntityManager(Global::entityManager);
    Global::projectileSystem->SetEntityManager(Global::entityManager);
    Global::graphicsSystem->SetEntityManager(Global::entityManager);
    Global::collisionSystem->SetEntityManager(Global::entityManager);
    Global::spawner->SetEntityManager(Global::entityManager);
    Global::playerController->SetEntityManager(Global::entityManager);
    Global::imguiSystem->SetEntityManager(Global::entityManager);

    // Wire InputSystem
    Global::playerController->SetInputSystem(Global::inputSystem);
    Global::movementSystem->SetInputSystem(Global::inputSystem);
    Global::collisionSystem->SetInput(Global::inputSystem);

    LOG_INFO("CORE", "[Init] Initializing critical systems...");

    // Initialize window
    Global::windowSystem->Initialize();

    // Initialize graphics
    Global::graphicsSystem->SetWindow(Global::windowSystem->GetWindow());
    Global::graphicsSystem->Initialize();

    // Initialize ImGui
    Global::imguiSystem->SetWindow(Global::windowSystem->GetWindow());
    Global::imguiSystem->SetEntitySpawner(Global::spawner);

    // Initialize player controller
    Global::playerController->SetWindow(Global::windowSystem->GetWindow());

    LOG_INFO("CORE", "[Init] Adding systems to engine...");

    // Add systems to engine (order matters!)
    Global::engine->AddSystem(Global::windowSystem);
    Global::engine->AddSystem(Global::inputSystem);
    Global::engine->AddSystem(Global::spawner);
    Global::engine->AddSystem(Global::playerController);
    Global::engine->AddSystem(Global::movementSystem);
    Global::engine->AddSystem(Global::collisionSystem);
    Global::engine->AddSystem(Global::projectileSystem);
    Global::engine->AddSystem(Global::graphicsSystem);
    Global::engine->AddSystem(Global::imguiSystem);

    // Initialize all systems
    Global::engine->Initialize();

    LOG_INFO("CORE", "[Init] Engine systems ready!");
}

void CleanupEngineSystems()
{
    LOG_INFO("CORE", "[Cleanup] Destroying engine systems...");

    if (Global::engine) {
        Global::engine->DestroySystems();
    }

    delete Global::entityManager;
    Global::entityManager = nullptr;

    glfwTerminate();

    LOG_INFO("CORE", "[Cleanup] Engine systems destroyed");
}

bool ShouldWindowClose()
{
    return Global::windowSystem && Global::windowSystem->ShouldClose();
}

void UpdateEngineSystems()
{
    // Engine systems are updated through the state's fpUpdate
    // But we need to handle input first
    Global::inputSystem->Update(0.0f);
}

// ============================================================================
// MAIN ENTRY POINT
// ============================================================================

int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int)
{
#ifdef _DEBUG
    AllocConsole();
    freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
    freopen_s((FILE**)stderr, "CONOUT$", "w", stderr);
    freopen_s((FILE**)stdin, "CONIN$", "r", stdin);

    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG | _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG | _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDOUT);
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG | _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDOUT);
    int flags = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
    flags |= _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF;
    _CrtSetDbgFlag(flags);
#endif

    // ========================================================================
    // INITIALIZE DEBUG TOOLS
    // ========================================================================
    eng::debug::LogConfig logCfg;
    logCfg.level = eng::debug::LogLevel::Info;
    logCfg.filePath = "engine.log";
    logCfg.useConsole = true;
    logCfg.useFile = true;
    logCfg.usePlatformOutput = true;
    logCfg.showSourceInfo = false;
    eng::debug::Log::init(logCfg);
    eng::debug::PerfViewer::set_print_interval(1.0);
    eng::debug::CrashLogger::install_handlers();

    LOG_INFO("CORE", "=================================================");
    LOG_INFO("CORE", "     Starting Game (Pure GSM Template)");
    LOG_INFO("CORE", "=================================================");

    // ========================================================================
    // INITIALIZE ENGINE
    // ========================================================================
    Global::engine = new Framework::CoreEngine();
    InitializeEngineSystems();

    // ========================================================================
    // DISPLAY CONTROLS
    // ========================================================================


    // ========================================================================
    // GAME STATE MANAGER INITIALIZATION
    // ========================================================================
    GSM_Initialize(mainMenu);  // Start from main menu

    LOG_INFO("CORE", "Entering GSM main loop...");
    LOG_INFO("CORE", "Initial state: %s",
        current == mainMenu ? "MainMenu" : "Unknown");

    // ========================================================================
    // GAME STATE MANAGER LOOP (Pure School Template Style)
    // ========================================================================
    while (current != GS_QUIT)
    {
        // Check for window close
        if (ShouldWindowClose()) {
            LOG_INFO("CORE", "Window close requested");
            break;
        }

        // --------------------------------------------------------------------
        // STATE TRANSITION LOGIC
        // --------------------------------------------------------------------
        if (current != GS_RESTART)
        {
            LOG_INFO("CORE", "[GSM] Transitioning to state: %d", current);

            // Update function pointers based on current state
            GSM_Update();

            // LOAD: Load assets for this state
            if (fpLoad) {
                LOG_INFO("CORE", "[GSM] Load phase...");
                fpLoad();
            }
        }
        else
        {
            // RESTART: Go back to previous state without reloading
            LOG_INFO("CORE", "[GSM] Restarting state: %d", previous);
            next = previous;
            current = previous;
        }

        // INITIALIZE: Set up the state
        if (fpInitialize) {
            LOG_INFO("CORE", "[GSM] Initialize phase...");
            fpInitialize();
        }

        // --------------------------------------------------------------------
        // STATE INNER LOOP (Game Loop)
        // --------------------------------------------------------------------
        LOG_INFO("CORE", "[GSM] Entering state loop...");

        while (next == current)
        {
            // Check window close in inner loop
            if (ShouldWindowClose()) {
                next = GS_QUIT;
                break;
            }

            // Start frame
            glfwPollEvents();

            // Update input
            UpdateEngineSystems();

            // UPDATE: State-specific logic
            if (fpUpdate) {
                fpUpdate();
            }

            // Update core engine systems (movement, collision, etc.)
            Global::engine->GameLoop();

            // DRAW: State-specific rendering
            if (fpDraw) {
                fpDraw();
            }

            // Swap buffers and present
            if (Global::windowSystem) {
                Global::windowSystem->GetWindow();
            }

        }

        LOG_INFO("CORE", "[GSM] Exiting state loop");

        // --------------------------------------------------------------------
        // STATE CLEANUP LOGIC
        // --------------------------------------------------------------------

        // FREE: Release temporary data
        if (fpFree) {
            LOG_INFO("CORE", "[GSM] Free phase...");
            fpFree();
        }

        // UNLOAD: Unload assets (unless restarting)
        if (next != GS_RESTART) {
            if (fpUnload) {
                LOG_INFO("CORE", "[GSM] Unload phase...");
                fpUnload();
            }
        }

        // Transition to next state
        previous = current;
        current = next;

        LOG_INFO("CORE", "[GSM] State transition complete: %d -> %d", previous, current);
    }

    // ========================================================================
    // CLEANUP
    // ========================================================================
    LOG_INFO("CORE", "GSM loop ended. Cleaning up...");

    CleanupEngineSystems();
    delete Global::engine;

    LOG_INFO("CORE", "=================================================");
    LOG_INFO("CORE", "     Engine shutdown complete");
    LOG_INFO("CORE", "=================================================");

    eng::debug::Log::shutdown();

#ifdef _DEBUG
    std::cout << "\nPress Enter to close console...\n";
    std::cin.get();
    FreeConsole();
#endif

    return 0;
}