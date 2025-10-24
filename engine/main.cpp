/**
===============================================================================
 File:           main.cpp (Clean Version with EngineBootstrapper)
 Author:         ETHAN NG YONG LE
 Email:          n.ethanyongle@digipen.edu
 Date:           2025-09-30
 Modified:       2025-10-24 (Refactored with EngineBootstrapper)
 ------------------------------------------------------------------------------

  Design notes:
  Ultra-clean entry point. All system initialization is delegated to
  EngineBootstrapper, keeping main.cpp focused on high-level flow.

===============================================================================
 */

#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "Precompiled.h"
#include "EngineBootstrapper/EngineBootstrapper.h"

 /**
  * @brief Windows application entry point
  */
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int)
{
#ifdef _DEBUG
    // Setup debug console
    AllocConsole();
    freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
    freopen_s((FILE**)stderr, "CONOUT$", "w", stderr);
    freopen_s((FILE**)stdin, "CONIN$", "r", stdin);

    // Enable memory leak detection
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
    LOG_INFO("CORE", "     Starting StructSquad Game Engine");
    LOG_INFO("CORE", "=================================================");

    // ========================================================================
    // CREATE AND INITIALIZE ENGINE
    // ========================================================================
    Framework::CoreEngine engine;

    // Configure engine
    Framework::EngineConfig config;
    config.windowWidth = 1280;
    config.windowHeight = 720;
    config.windowTitle = "StructSquad";
    config.gameConfigPath = "game_config.txt";
    config.enableImGui = true;
    config.enableDebugConsole = true;

    // Initialize all systems via bootstrapper
    if (!Framework::EngineBootstrapper::Initialize(engine, config)) {
        LOG_ERROR("ERROR", "Failed to initialize engine!");
        return -1;
    }

    // Initialize engine (calls Initialize() on all registered systems)
    engine.Initialize();

    // ========================================================================
    // DISPLAY CONTROLS
    // ========================================================================
    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════════════╗\n";
    std::cout << "║              GAME CONTROLS                     ║\n";
    std::cout << "╠════════════════════════════════════════════════╣\n";
    std::cout << "║  WASD/Arrows  : Move player                    ║\n";
    std::cout << "║  SPACE        : Shoot up                       ║\n";
    std::cout << "║  LEFT SHIFT   : Shoot down                     ║\n";
    std::cout << "║  LEFT MOUSE   : Shoot toward mouse             ║\n";
    std::cout << "║                                                ║\n";
    std::cout << "║  [DEBUG CONTROLS]                              ║\n";
    std::cout << "║  E            : Spawn enemy                    ║\n";
    std::cout << "║  Q            : Spawn obstacle                 ║\n";
    std::cout << "║  R            : Spawn pickup                   ║\n";
    std::cout << "║  F2           : Export performance CSV         ║\n";
    std::cout << "║  ESC          : Quit                           ║\n";
    std::cout << "╚════════════════════════════════════════════════╝\n";
    std::cout << "\n";

    LOG_INFO("CORE", "Entering game loop...");

    // ========================================================================
    // GAME LOOP
    // ========================================================================
    engine.GameLoop();

    // ========================================================================
    // CLEANUP
    // ========================================================================
    LOG_INFO("CORE", "Game loop ended. Cleaning up...");

    engine.DestroySystems();

    // Cleanup entity manager
    delete Framework::EngineBootstrapper::GetEntityManager();

    glfwTerminate();

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