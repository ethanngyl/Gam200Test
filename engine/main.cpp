/**
===============================================================================
File: main.cpp (Super Simple Version)
Author: GE YONGQI
Description: Using the integrated CoreEngine, main.cpp becomes very simple.
===============================================================================
 */

#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "Precompiled.h"
#include "GSM/GameStateManager.h"
#include "ImguiSystem.h"
#include "ConfigReader/ConfigReader.h"



 // ============================================================================
 // GLOBAL VARIABLES (School Template Style)
 // ============================================================================
extern int current, previous, next;
extern FP fpLoad, fpInitialize, fpUpdate, fpDraw, fpFree, fpUnload;

// ============================================================================
// GLOBAL ENGINE
// ============================================================================
Framework::CoreEngine* engine = nullptr;

// ============================================================================
// MAIN ENTRY POINT
// ============================================================================

int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int)
{

#ifdef _DEBUG
    // Debug console setup
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
    LOG_INFO("CORE", "     Starting Game (Clean GSM Template)");
    LOG_INFO("CORE", "=================================================");


    // ========================================================================
    // INITIALIZE ENGINE
    // ========================================================================
    engine = new Framework::CoreEngine();
    if (!engine->InitializeAllSystems()) {
        LOG_ERROR("CORE", "Failed to initialize engine!");
        return -1;
    }

    // ========================================================================
    // GAME STATE MANAGER
    // ========================================================================
    int initialState = ConfigReader::GetInitialGameState(mainMenu);
    GSM_Initialize(initialState);

    LOG_INFO("CORE", "Entering GSM main loop...");

    unsigned lastTime = timeGetTime();

    // ========================================================================
    // GAME STATE MANAGER LOOP
    // ========================================================================
    while (current != GS_QUIT)
    {
        // Check for window close
        if (engine->ShouldWindowClose()) {
            LOG_INFO("CORE", "Window close requested");
            break;
        }

        // --------------------------------------------------------------------
        // STATE TRANSITION
        // --------------------------------------------------------------------
        if (current != GS_RESTART)
        {
            LOG_INFO("CORE", "[GSM] Transitioning to state: %d", current);
            GSM_Update();

            if (fpLoad) {
                LOG_INFO("CORE", "[GSM] Load phase...");
                fpLoad();
            }
        }
        else
        {
            LOG_INFO("CORE", "[GSM] Restarting state: %d", previous);
            next = previous;
            current = previous;
        }

        if (fpInitialize) {
            LOG_INFO("CORE", "[GSM] Initialize phase...");
            fpInitialize();
        }

        // --------------------------------------------------------------------
        // STATE LOOP
        // --------------------------------------------------------------------
        LOG_INFO("CORE", "[GSM] Entering state loop...");

        while (next == current)
        {
            // Check engine and window
            if (!engine->IsActive() || engine->ShouldWindowClose()) {
                next = GS_QUIT;
                break;
            }

            // Calculate delta time
            unsigned currentTime = timeGetTime();
            float dt = (currentTime - lastTime) / 1000.0f;
            if (dt < 0.001f) dt = 0.016f;
            lastTime = currentTime;

            // Poll events
            glfwPollEvents();

            // Update all systems
            engine->UpdateSingleFrame(dt);

            // State update
            if (fpUpdate) {
                fpUpdate();
            }

            // State draw
            if (fpDraw) {
                fpDraw();
            }

            if (engine->GetImGuiSystem() &&
                engine->GetWindowSystem() &&
                engine->GetWindowSystem()->GetWindow()) {
                engine->GetImGuiSystem()->Render();

                if (engine->GetGraphicsSystem()) {
                    engine->GetGraphicsSystem()->RenderImGui();
                }
            }
        }

        LOG_INFO("CORE", "[GSM] Exiting state loop");

        // --------------------------------------------------------------------
        // STATE CLEANUP
        // --------------------------------------------------------------------
        if (fpFree) {
            LOG_INFO("CORE", "[GSM] Free phase...");
            fpFree();
        }

        if (next != GS_RESTART) {
            if (fpUnload) {
                LOG_INFO("CORE", "[GSM] Unload phase...");
                fpUnload();
            }
        }

        previous = current;
        current = next;
    }

    // ========================================================================
    // CLEANUP
    // ========================================================================
    LOG_INFO("CORE", "GSM loop ended. Cleaning up...");

    engine->Cleanup();
    delete engine;

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