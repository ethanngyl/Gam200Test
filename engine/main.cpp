/**
===============================================================================
 File:           main.cpp (Updated with GameStateManager)
 Description:    Main entry point with GSM integration
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
#include "ImguiSystem.h"
#include "MainMenuSystem.h"
#include "GSM/GameStateManager.h"  

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
    auto* playerController = new Framework::PlayerControllerSystem();
    auto* imguiSys = new Framework::ImGuiSystem();
    auto* mainMenu = new Framework::MainMenuSystem();
    auto* gsm = new Framework::GameStateManager();  

    // --- Wire dependencies ---
    movementSys->SetEntityManager(&entityManager);
    projectileMovement->SetEntityManager(&entityManager);
    graphicsSys->SetEntityManager(&entityManager);
    collisionSys->SetEntityManager(&entityManager);
    spawner->SetEntityManager(&entityManager); 

    playerController->SetEntityManager(&entityManager);
    playerController->SetInputSystem(inputSys);

    movementSys->SetInputSystem(inputSys);
    collisionSys->SetInput(inputSys);

    // Window & Graphics
    windowSys->Initialize();
    graphicsSys->SetWindow(windowSys->GetWindow());
    graphicsSys->Initialize();

    // ImGui
    imguiSys->SetWindow(windowSys->GetWindow());
    imguiSys->SetEntityManager(&entityManager);
    imguiSys->SetEntitySpawner(spawner); 

    // MainMenu
    mainMenu->SetEntityManager(&entityManager);
    mainMenu->SetGraphics(graphicsSys);
    mainMenu->SetWindow(windowSys->GetWindow());

    // NEW: GameStateManager setup
    gsm->SetEntityManager(&entityManager);
    gsm->SetMainMenuSystem(mainMenu);
    gsm->SetEntitySpawner(spawner);
    gsm->SetConfigPath("assets/game_config.txt");  

 

    // --- Add systems to engine ---
    engine.AddSystem(windowSys);
    engine.AddSystem(gsm);                
    engine.AddSystem(inputSys);
    engine.AddSystem(spawner);        
    engine.AddSystem(playerController);
    engine.AddSystem(movementSys);
    engine.AddSystem(collisionSys);
    engine.AddSystem(mathSys);
    engine.AddSystem(projectileMovement);
    engine.AddSystem(mainMenu);
    engine.AddSystem(graphicsSys);
    engine.AddSystem(imguiSys);

    // NEW: Connect MainMenu Play button to GSM
    mainMenu->SetOnPlayCallback([gsm]() {
        gsm->ChangeState(Framework::GameState::Level1);
        });

    // --- Initialize all systems ---
    engine.Initialize();

    playerController->SetWindow(windowSys->GetWindow());

    LOG_INFO("CORE", "Engine initialized. Starting game loop...");

    // Run game
    engine.GameLoop();

    LOG_INFO("CORE", "Game loop ended. Cleaning up...");

    // Cleanup
    engine.DestroySystems();
    glfwTerminate();
    LOG_INFO("CORE", "Engine shutdown complete.");
    eng::debug::Log::shutdown();

#ifdef _DEBUG
    std::cout << "Press Enter to close console...\n";
    std::cin.get();
    FreeConsole();
#endif

    return 0;
}
