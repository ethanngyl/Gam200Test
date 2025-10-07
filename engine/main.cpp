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

int WINAPI WinMain(_In_ HINSTANCE /*hInstance*/,
    _In_opt_ HINSTANCE /*hPrevInstance*/,
    _In_ LPSTR /*lpCmdLine*/,
    _In_ int /*nShowCmd*/
)
{
#ifdef _DEBUG
    // Allocate console for debug output in Debug builds
    AllocConsole();
    freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
    freopen_s((FILE**)stderr, "CONOUT$", "w", stderr);
    freopen_s((FILE**)stdin, "CONIN$", "r", stdin);

    // === Set up CRT debug heap leak checking ===
    // Send reports to debugger AND stdout
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG | _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG | _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDOUT);
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG | _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDOUT);

    // Enable leak check at process exit + allocation tracking
    int flags = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
    flags |= _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF;
    // Optional (slower): validate heap on every alloc/free
    // flags |= _CRTDBG_CHECK_ALWAYS_DF;
    _CrtSetDbgFlag(flags);
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

    // === CHANGED: Use GraphicsSystemV2 instead of GraphicsSystem ===
    Framework::GraphicsSystemV2* graphicsSys = new Framework::GraphicsSystemV2();

    Framework::InputSystem* inputSys = new Framework::InputSystem();
    Framework::CollisionSystem* collisionSys = new Framework::CollisionSystem();
    Framework::MathTestSystem* mathSys = new Framework::MathTestSystem();
    Framework::MovementSystem* movementSys = new Framework::MovementSystem();
    // Framework::TextSystem* textSys = new Framework::TextSystem();  // COMMENTED OUT - causing linker errors

    // Set entity manager for systems that need it
    movementSys->SetEntityManager(&entityManager);
    graphicsSys->SetEntityManager(&entityManager);
    collisionSys->SetEntityManager(&entityManager);

    movementSys->SetInputSystem(inputSys);
    collisionSys->SetInput(inputSys);

    // Add systems to engine
    engine.AddSystem(windowSys);
    engine.AddSystem(movementSys);
    engine.AddSystem(graphicsSys);
    engine.AddSystem(inputSys);
    engine.AddSystem(collisionSys);
    engine.AddSystem(mathSys);
    // engine.AddSystem(textSys);  // COMMENTED OUT - TextSystem causing linker errors

    LOG_INFO("CORE", "Systems added. Initializing engine...");

    // === WORKAROUND: Initialize WindowSystem first to create the window ===
    windowSys->Initialize();

    // === CRITICAL: Now set the window before engine.Initialize() ===
    graphicsSys->SetWindow(windowSys->GetWindow());

    // Initialize remaining systems (WindowSystem already initialized, won't run twice)
    engine.Initialize();

    LOG_INFO("CORE", "Engine initialized. Starting game loop...");

    // CREATE TEST ENTITIES AFTER INITIALIZATION
    LOG_INFO("CORE", "=== Creating ECS Test Entities ===");

    // ========================================================================
    // BACKWARD COMPATIBLE: Using OLD Sprite component (still works!)
    // ========================================================================

    // Test entity 1: Triangle
    Framework::Entity triangleEntity = entityManager.CreateEntity();
    entityManager.AddComponent<Framework::Transform>(triangleEntity, Framework::Vector2D(-0.5f, 0.0f));
    auto& transform = entityManager.GetComponent<Framework::Transform>(triangleEntity);
    transform.scale = Framework::Vector2D(0.5f, 0.5f);
    entityManager.AddComponent<Framework::Sprite>(triangleEntity);
    entityManager.GetComponent<Framework::Sprite>(triangleEntity).texturePath = "triangle";
    entityManager.AddComponent<Framework::TriangleCollider>(triangleEntity);

    auto& triCol = entityManager.GetComponent<Framework::TriangleCollider>(triangleEntity);
    float halfW = transform.scale.x * 0.1f;
    float halfH = transform.scale.y * 0.1f;
    triCol.v0 = Framework::Vector2D(0.0f, halfH);
    triCol.v1 = Framework::Vector2D(-halfW, -halfH);
    triCol.v2 = Framework::Vector2D(halfW, -halfH);
    LOG_INFO("CORE", "Created triangle entity");

    // Test entity 2: Quad
    Framework::Entity quadEntity = entityManager.CreateEntity();
    entityManager.AddComponent<Framework::Transform>(quadEntity, Framework::Vector2D(0.1f, 0.1f));
    auto& transform1 = entityManager.GetComponent<Framework::Transform>(quadEntity);
    entityManager.AddComponent<Framework::BoxCollider>(quadEntity);
    auto& box = entityManager.GetComponent<Framework::BoxCollider>(quadEntity);
    box.size = Framework::Vector2D(0.1f, 0.1f);
    transform1.scale = Framework::Vector2D(0.1f, 0.1f); // Match visual size to collider

    entityManager.AddComponent<Framework::Sprite>(quadEntity);
    entityManager.GetComponent<Framework::Sprite>(quadEntity).texturePath = "quad";
    entityManager.AddComponent<Framework::Movement>(quadEntity);
    entityManager.GetComponent<Framework::Movement>(quadEntity).moveSpeed = 0.1f;
    LOG_INFO("CORE", "Created quad entity with movement");

    // Test entity 3: Circle
    Framework::Entity circleEntity = entityManager.CreateEntity();
    entityManager.AddComponent<Framework::Transform>(circleEntity, Framework::Vector2D(0.5f, 0.5f));
    auto& transform2 = entityManager.GetComponent<Framework::Transform>(circleEntity);
    entityManager.AddComponent<Framework::CircleCollider>(circleEntity);
    auto& cc = entityManager.GetComponent<Framework::CircleCollider>(circleEntity);
    cc.radius = 0.05f;  // true collision radius
    transform2.scale = Framework::Vector2D(0.1f, 0.1f); //Visual size
    entityManager.AddComponent<Framework::Sprite>(circleEntity);
    entityManager.GetComponent<Framework::Sprite>(circleEntity).texturePath = "circle";
    LOG_INFO("CORE", "Created circle entity");

    // ========================================================================
    // OPTIONAL: Example using NEW Renderable component (uncomment to try)
    // ========================================================================
    /*
    // Example: Create an entity using the NEW Renderable component
    Framework::Entity newStyleEntity = entityManager.CreateEntity();
    entityManager.AddComponent<Framework::Transform>(newStyleEntity, Framework::Vector2D(-0.3f, 0.3f));

    // Get mesh and material handles from graphics system
    Framework::MeshHandle mesh = graphicsSys->GetMeshForSpriteName("triangle");
    Framework::MaterialHandle material = graphicsSys->GetMaterialForSpriteName("triangle");

    // Create Renderable component
    entityManager.AddComponent<Framework::Renderable>(newStyleEntity, mesh, material);
    auto& renderable = entityManager.GetComponent<Framework::Renderable>(newStyleEntity);
    renderable.tint = glm::vec4(1.0f, 0.5f, 0.5f, 1.0f); // Tinted red
    renderable.layer = 5; // Render on top

    LOG_INFO("CORE", "Created entity with new Renderable component");
    */

    // ========================================================================
    // OPTIONAL: Camera control example (uncomment to try)
    // ========================================================================
    
    // Control the camera
    graphicsSys->SetCameraZoom(1.5f);  // Zoom in
    graphicsSys->SetCameraPosition(glm::vec3(0.0f, 0.0f, 0.0f));  // Center camera
    

    // ========================================================================
    // OPTIONAL: Debug rendering example (uncomment to try)
    // ========================================================================
    /*
    // Enable debug rendering
    graphicsSys->SetDebugRenderingEnabled(true);

    // Add debug visuals (these would normally be added in Update loop)
    auto& debugQueue = graphicsSys->GetDebugQueue();
    debugQueue.AddCircle(glm::vec3(0.5f, 0.5f, 0.0f), 0.05f, glm::vec4(0, 1, 0, 1));
    debugQueue.AddLine(glm::vec3(-1, 0, 0), glm::vec3(1, 0, 0), glm::vec4(1, 0, 0, 1));
    */

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