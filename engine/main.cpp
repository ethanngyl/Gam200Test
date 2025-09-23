#include "Precompiled.h"
#include "Core.h"
#include "WindowSystem.h"
#include "CollisionSystem.h"
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
    Framework::CollisionSystem* collisionSys = new Framework::CollisionSystem();
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

    return 0;
}