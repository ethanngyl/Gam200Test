#include "Precompiled.h"
#include "Core.h"
#include "WindowSystem.h"
#include "GraphicsSystem.h"
#include "Input.h"
#include <iostream>

int main()
{
    std::cout << "Starting Game Engine...\n";

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
    engine.AddSystem(inputSys);

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

    return 0;
}