/*
===============================================================================
 File:          main.cpp
 Author:        GE YONGQI
 Email:         yongqi.ge@digipen.edu
 Date:          2025-10-31
 Contribution:  100%
 ------------------------------------------------------------------------------
  Main entry point of the StructSquad Engine

  Responsibilities:
     - Initializes debug console, logging, and memory leak detection
     - Sets up the CoreEngine and initializes all subsystems
     - Manages the Game State Manager (GSM) lifecycle
     - Runs the main game loop until the quit condition is met

  Highlights:
     - Uses ConfigReader to determine the initial game state
     - Integrates DebugConfig for performance profiling and FPS tracking
     - Supports hotkey-based debug features (e.g., F2 exports performance data)
===============================================================================
*/


#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "Precompiled.h"
#include "ImguiSystem.h"

 // ============================================================================
 // GLOBAL VARIABLES
 // ============================================================================
extern int current, previous, next;
extern FP fpLoad, fpInitialize, fpUpdate, fpDraw, fpFree, fpUnload;

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
    // INITIALIZE DEBUG SYSTEMS
    // ========================================================================
    Framework::DebugConfig::Initialize();

    LOG_INFO("CORE", "=================================================");
    LOG_INFO("CORE", "     StructSquad Engine Starting");
    LOG_INFO("CORE", "=================================================");

    // ========================================================================
    // INITIALIZE ENGINE
    // ========================================================================
    engine = new Framework::CoreEngine();
    if (!engine->InitializeAllSystems()) {
        LOG_ERROR("CORE", "Failed to initialize engine!");
        Framework::DebugConfig::Shutdown();
        return -1;
    }

    // ========================================================================
    // ⭐ SETUP FPS COUNTER WITH WINDOW ⭐
    // ========================================================================
    if (engine->GetWindowSystem() && engine->GetWindowSystem()->GetWindow()) {
        auto window = engine->GetWindowSystem()->GetWindow();
        // Reinitialize FPS counter with window (for title updates)
        Framework::DebugConfig::Initialize(window);
    }

    // ========================================================================
    // GAME STATE MANAGER
    // ========================================================================
    int initialState = ConfigReader::GetInitialGameState(mainMenu);
    GSM_Initialize(initialState);

    LOG_INFO("CORE", "Entering GSM main loop...");
    LOG_INFO("CORE", "Debug Controls: F2 = Export Performance CSV");

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

            // Begin performance frame
            eng::debug::PerfViewer::begin_frame();

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

            // ImGui rendering
            if (engine->GetImGuiSystem() &&
                engine->GetWindowSystem() &&
                engine->GetWindowSystem()->GetWindow()) {
                engine->GetImGuiSystem()->Render();

                if (engine->GetGraphicsSystem()) {
                    engine->GetGraphicsSystem()->RenderImGui();
                }
            }

            // End performance frame
            eng::debug::PerfViewer::end_frame();

            // Update FPS counter
            Framework::DebugConfig::GetFpsCounter().tick_with_dt(
                static_cast<double>(dt)
            );

            // ⭐ Debug hotkey: F2 to export performance data
            if (GetAsyncKeyState(VK_F2) & 0x0001) {
                if (eng::debug::PerfViewer::export_csv("performance.csv")) {
                    LOG_INFO("DEBUG", "Performance data exported");
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
    engine = nullptr;

    LOG_INFO("CORE", "=================================================");
    LOG_INFO("CORE", "     Engine shutdown complete");
    LOG_INFO("CORE", "=================================================");

    // SHUTDOWN DEBUG SYSTEMS
    Framework::DebugConfig::Shutdown();

#ifdef _DEBUG
    std::cout << "\nPress Enter to close console...\n";
    std::cin.get();
    FreeConsole();
#endif

    return 0;
}