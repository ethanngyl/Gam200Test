#include "Precompiled.h"
#include "Core.h"
#include "GraphicsSystem.h"
#include "ECSEntityManager.h"
#include "ECSEntity.h"
#include "Component.h"
#include "MovementSystem.h"
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

    LOG_INFO("CORE", "Starting Game Engine...");

    // Create the core engine
    Framework::CoreEngine engine;
    Framework::EntityManager entityManager;
    // Create systems
    Framework::WindowSystem* windowSys = new Framework::WindowSystem();
    Framework::GraphicsSystem* graphicsSys = new Framework::GraphicsSystem();
    Framework::InputSystem* inputSys = new Framework::InputSystem();
    Framework::CollisionSystem* collisionSys = new Framework::CollisionSystem();
    Framework::MathTestSystem* mathSys = new Framework::MathTestSystem();
    Framework::MovementSystem* movementSys = new Framework::MovementSystem();
    movementSys->SetEntityManager(&entityManager);
    graphicsSys->SetEntityManager(&entityManager);
    movementSys->SetInputSystem(inputSys);
	collisionSys->SetInput(inputSys);

    engine.AddSystem(windowSys);
    engine.AddSystem(movementSys);
    engine.AddSystem(graphicsSys);
    engine.AddSystem(inputSys);
    engine.AddSystem(collisionSys);
    engine.AddSystem(mathSys);

    LOG_INFO("CORE", "Systems added.Initializing engine...");


    // Pass a pointer of InputSystem to CollisionSystem.
// This lets CollisionSystem call IsKeyDown() to move the circle for collider testing.
    collisionSys->SetInput(inputSys);

    // Initialize all systems
    engine.Initialize();

    LOG_INFO("CORE", "Engine initialized. Starting game loop...");

    // CREATE TEST ENTITIES AFTER INITIALIZATION
    LOG_INFO("CORE", "=== Creating ECS Test Entities ===");


    // Test entity 1: Triangle
    Framework::Entity triangleEntity = entityManager.CreateEntity();
    entityManager.AddComponent<Framework::Transform>(triangleEntity, Framework::Vector2D(-0.5f, 0.0f));
    entityManager.AddComponent<Framework::Sprite>(triangleEntity);
    entityManager.GetComponent<Framework::Sprite>(triangleEntity).texturePath = "triangle";
    entityManager.AddComponent<Framework::Movement>(triangleEntity);  // Add this
    entityManager.GetComponent<Framework::Movement>(triangleEntity).moveSpeed = 1.0f;  // Set speed
    //entityManager.GetComponent<Framework::Movement>(triangleEntity).direction = Framework::Vector2D(1.0f, 0.0f);  // Move right
    LOG_INFO("CORE", "Created triangle entity with movement");


    //// Test entity 2: Quad
    //Framework::Entity quadEntity = entityManager.CreateEntity();
    //entityManager.AddComponent<Framework::Transform>(quadEntity, Framework::Vector2D(0.0f, 0.0f));
    //entityManager.AddComponent<Framework::Sprite>(quadEntity);
    //entityManager.GetComponent<Framework::Sprite>(quadEntity).texturePath = "quad";
    //std::cout << "Created quad entity\n";

    //// Test entity 3: Circle
    //Framework::Entity circleEntity = entityManager.CreateEntity();
    //entityManager.AddComponent<Framework::Transform>(circleEntity, Framework::Vector2D(-1.0f, -1.5f));
    //entityManager.AddComponent<Framework::Sprite>(circleEntity);
    //entityManager.GetComponent<Framework::Sprite>(circleEntity).texturePath = "circle";
    //std::cout << "Created circle entity\n";

    std::cout << "Total entities: " << entityManager.GetAllEntities().size() << "\n\n";

    // Run the main game loop
    engine.GameLoop();
    LOG_INFO("CORE", "Game loop ended. Cleaning up...");

    // Cleanup systems
    engine.DestroySystems();

    LOG_INFO("CORE", "Engine shutdown complete.");


    // Shutdown debug tools
    eng::debug::Log::shutdown();
#ifdef _DEBUG
    std::cout << "Press Enter to close console...\n";

    std::cin.get(); // Wait for actual Enter key
    FreeConsole();
#endif

    return 0;
}