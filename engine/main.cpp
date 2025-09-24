#include "Precompiled.h"
#include "Core.h"
#include "WindowSystem.h"
#include "GraphicsSystem.h"
#include "Input.h"
#include "Collision/CollisionSystem.h"
#include <iostream>

#include "DebugComponents/Log.h"
#include "DebugComponents/Sinks.h"
#include "DebugComponents/CrashLogger.h"
#include "DebugComponents/PerfViewer.h"


int main()
{
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
    Framework::GraphicsSystem* graphicsSys = new Framework::GraphicsSystem();

    std::cout << "Systems created. Initializing WindowSystem...\n";

    // Initialize WindowSystem first to create the window
    windowSys->Initialize();

    if (!windowSys->GetWindow()) {
        std::cerr << "Failed to create window!\n";
        return -1;
    }

    std::cout << "Window created. Setting up GraphicsSystem...\n";

    // Then set the window for GraphicsSystem
    graphicsSys->SetWindow(windowSys->GetWindow());

    engine.AddSystem(graphicsSys);
    Framework::CollisionSystem* collisionSys = new Framework::CollisionSystem();
    Framework::MathTestSystem* mathSys = new Framework::MathTestSystem();
    engine.AddSystem(mathSys);
    engine.AddSystem(windowSys);
    engine.AddSystem(inputSys);

    std::cout << "Systems added. Initializing engine...\n";

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


    return 0;
}