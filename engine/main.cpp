/**
===============================================================================
 File:           main.cpp (Updated for GraphicsSystemV2)
 Author:         ETHAN NG YONG LE
 Email:          n.ethanyongle@digipen.edu
 Date:           2025-09-30
 Modified:       2025-10-07 (Graphics System V2 Integration)
 ------------------------------------------------------------------------------

  Design notes:
  Sets up the game engine, creates systems and entities, runs the main loop,
 * and handles cleanup. Includes debug features like memory leak detection
 * and crash logging in debug builds.

  CHANGES FOR GRAPHICS SYSTEM V2:
  - Replaced GraphicsSystem with GraphicsSystemV2
  - Added window pointer passing to graphics system
  - Kept backward compatibility with Sprite components
  - Added example of using new Renderable component (commented out)
===============================================================================
 */

#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#include "Precompiled.h"
#include "GraphicsSystemV2.h"
#include "EntitySpawner.h"
#include "PlayerManager.h"
#include "ProjectileSystem.h"
 /**
  * @brief Windows application entry point
  * @param hInstance Handle to current application instance
  * @param hPrevInstance Always NULL in modern Windows
  * @param lpCmdLine Command line arguments
  * @param nShowCmd Window display mode
  * @return Exit code (0 for success)
  *
  * Initializes:
  * - Debug console and memory leak detection (debug builds only)
  * - Logging and crash reporting systems
  * - Core engine and all subsystems
  * - ECS entities for demonstration
  *
  * Execution flow:
  * 1. Debug setup (console, heap tracking)
  * 2. Initialize logging and crash handlers
  * 3. Create and wire up engine systems
  * 4. Initialize all systems
  * 5. Create test entities (triangle, quad)
  * 6. Run game loop until quit
  * 7. Cleanup and shutdown
  */

void SetupGame(Framework::EntitySpawner* spawner)
{
    LOG_INFO("CORE", "=== Setting up game ===");

    // Note: Player is spawned separately so we can get its Entity ID

    // Spawn some initial enemies
    spawner->SpawnEnemyWave(5, 0.6f);

    // Spawn walls
    spawner->SpawnObstacle(Framework::Vector2D(-1.8f, 0.0f), Framework::Vector2D(0.1f, 2.0f));
    spawner->SpawnObstacle(Framework::Vector2D(1.8f, 0.0f), Framework::Vector2D(0.1f, 2.0f));

    LOG_INFO("CORE", "Game setup complete!");
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

    // Initialize debug tools
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

    LOG_INFO("CORE", "Starting Game Engine...");

    // Create core
    Framework::CoreEngine engine;
    Framework::EntityManager entityManager;

    // Create systems
    auto* windowSys = new Framework::WindowSystem();
    auto* graphicsSys = new Framework::GraphicsSystemV2();
    auto* inputSys = new Framework::InputSystem();
    auto* collisionSys = new Framework::CollisionSystem();
    auto* mathSys = new Framework::MathTestSystem();
    auto* movementSys = new Framework::MovementSystem();
    auto* projectileMovement = new Framework::ProjectileMovementSystem();
    auto* spawner = new Framework::EntitySpawner();
    auto* playerController = new Framework::PlayerControllerSystem();  // NEW!

    // Configure systems
    movementSys->SetEntityManager(&entityManager);
    projectileMovement->SetEntityManager(&entityManager);
    graphicsSys->SetEntityManager(&entityManager);
    collisionSys->SetEntityManager(&entityManager);
    spawner->SetEntityManager(&entityManager);

    // NEW: Configure player controller
    playerController->SetEntitySpawner(spawner);
    playerController->SetEntityManager(&entityManager);
    playerController->SetInputSystem(inputSys);

    movementSys->SetInputSystem(inputSys);
    collisionSys->SetInput(inputSys);

    // Add systems to engine
    engine.AddSystem(windowSys);
    engine.AddSystem(spawner);
    engine.AddSystem(playerController);  // NEW! Add before movement
    engine.AddSystem(movementSys);
    engine.AddSystem(graphicsSys);
    engine.AddSystem(inputSys);
    engine.AddSystem(collisionSys);
    engine.AddSystem(mathSys);
    engine.AddSystem(projectileMovement);

    LOG_INFO("CORE", "Systems added. Initializing engine...");

    // Initialize
    windowSys->Initialize();
    graphicsSys->SetWindow(windowSys->GetWindow());

    // NEW: Give player controller access to window
    playerController->SetWindow(windowSys->GetWindow());

    engine.Initialize();

    LOG_INFO("CORE", "Engine initialized. Setting up game...");

    // Setup game
    SetupGame(spawner);

    // Spawn player and give controller access to it
    Framework::Entity player = spawner->SpawnPlayer(Framework::Vector2D(0.0f, -0.5f));
    playerController->SetPlayerEntity(player);  // NEW!

    std::cout << "\n=== CONTROLS ===\n";
    std::cout << "WASD/Arrows: Move player\n";
    std::cout << "SPACE: Shoot up\n";
    std::cout << "LEFT SHIFT: Shoot down\n";
    std::cout << "LEFT MOUSE: Shoot toward mouse\n";
    std::cout << "E: Spawn enemy (debug)\n";
    std::cout << "Q: Spawn obstacle (debug)\n";
    std::cout << "R: Spawn pickup (debug)\n";
    std::cout << "================\n\n";

    std::cout << "Total entities: " << entityManager.GetAllEntities().size() << "\n\n";

    // Run game
    engine.GameLoop();

    LOG_INFO("CORE", "Game loop ended. Cleaning up...");

    // Cleanup
    engine.DestroySystems();
    LOG_INFO("CORE", "Engine shutdown complete.");
    eng::debug::Log::shutdown();

#ifdef _DEBUG
    std::cout << "Press Enter to close console...\n";
    std::cin.get();
    FreeConsole();
#endif

    return 0;
}


