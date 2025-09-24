#include "Precompiled.h"
#include "Core.h"
#include "Windows/WindowSystem.h"
#include "Graphics/GraphicsSystem.h"
#include "Input/Input.h"
#include "Collision/CollisionSystem.h"

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

    // Create window system first
    Framework::WindowSystem* windowSys = new Framework::WindowSystem();

    // Initialize WindowSystem first to create the window
    windowSys->Initialize();

    if (!windowSys->GetWindow()) {
        std::cerr << "Failed to create window!\n";
        return -1;
    }

    std::cout << "Systems created. Initializing WindowSystem...\n";

    // Create other systems
    Framework::GraphicsSystem* graphicsSys = new Framework::GraphicsSystem();
    Framework::InputSystem* inputSys = new Framework::InputSystem();
    Framework::CollisionSystem* collisionSys = new Framework::CollisionSystem();
    Framework::MathTestSystem* mathSys = new Framework::MathTestSystem();

    engine.AddSystem(graphicsSys);
    engine.AddSystem(inputSys);
    engine.AddSystem(collisionSys);
    engine.AddSystem(mathSys);

    collisionSys->SetInput(inputSys);

    // Then set the window for GraphicsSystem
    graphicsSys->SetWindow(windowSys->GetWindow());

    std::cout << "Window created. Setting up GraphicsSystem...\n";

    std::cout << "Systems added. Initializing engine...\n";

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