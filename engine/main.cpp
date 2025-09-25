#include "Precompiled.h"
#include "Core.h"
#include "Collision/CollisionSystem.h"
#include <iostream>
#include "DebugComponents/Log.h"
#include "DebugComponents/Sinks.h"
#include "DebugComponents/CrashLogger.h"
#include "DebugComponents/PerfViewer.h"

int WINAPI WinMain(    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine,
    _In_ int nShowCmd)
{
    // Allocate console for debug output in debug builds
#ifdef _DEBUG
    AllocConsole();
    freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
    freopen_s((FILE**)stderr, "CONOUT$", "w", stderr);
    freopen_s((FILE**)stdin, "CONIN$", "r", stdin);
#endif

    std::cout << "Starting Game Engine...\n";

    // ------------ Debug tools bootstrap ------------//
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
    // --------- End Of Debug tools bootstrap ---------//

    // Create the core engine
    Framework::CoreEngine engine;

    // Create and add systems
    Framework::WindowSystem* windowSys = new Framework::WindowSystem();
    Framework::InputSystem* inputSys = new Framework::InputSystem();
    Framework::CollisionSystem* collisionSys = new Framework::CollisionSystem();
    Framework::MathTestSystem* mathSys = new Framework::MathTestSystem();

    engine.AddSystem(mathSys);
    engine.AddSystem(windowSys);
    engine.AddSystem(inputSys);
    collisionSys->SetInput(inputSys);
    engine.AddSystem(collisionSys);

    // Initialize all systems
    engine.Initialize();

    std::cout << "Engine initialized. Starting game loop...\n";

    // Run the main game loop
    engine.GameLoop();

    std::cout << "Game loop ended. Cleaning up...\n";

    // Cleanup systems
    engine.DestroySystems();

    std::cout << "Engine shutdown complete.\n";

    // Shutdown debug tools
    eng::debug::Log::shutdown();

#ifdef _DEBUG
    FreeConsole();
#endif

    return 0;
}