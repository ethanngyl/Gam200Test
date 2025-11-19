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
#include "TimeConstants.h"

// ===============================================================================
// GLOBAL VARIABLES
// ===============================================================================
extern int current, previous, next;
extern FP fpLoad, fpInitialize, fpUpdate, fpDraw, fpFree, fpUnload;

Framework::CoreEngine* engine = nullptr;

// Fixed DT Variables
using namespace Framework::Time;
double accumulatedTime = 0.0;              // Time debt accumulator
int currentNumberOfSteps = 0;              // Steps to execute this frame

// ===============================================================================
// MAIN ENTRY POINT
// ===============================================================================

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

    // ===============================================================================
    // INITIALIZE DEBUG SYSTEMS
    // ===============================================================================
    Framework::DebugConfig::Initialize();

    LOG_INFO("CORE", "=================================================");
    LOG_INFO("CORE", "     StructSquad Engine Starting");
    LOG_INFO("CORE", "     Fixed DT: %.4f ms (%.0f FPS target)",
        FIXED_DT * 1000.0, 1.0 / FIXED_DT);
    LOG_INFO("CORE", "=================================================");

    // ===============================================================================
    // INITIALIZE ENGINE
    // ===============================================================================
    engine = new Framework::CoreEngine();
    if (!engine->InitializeAllSystems()) {
        LOG_ERROR("CORE", "Failed to initialize engine!");
        Framework::DebugConfig::Shutdown();
        return -1;
    }

    // ===============================================================================
    // SETUP FPS COUNTER WITH WINDOW
    // ===============================================================================
    if (engine->GetWindowSystem() && engine->GetWindowSystem()->GetWindow()) {
        auto window = engine->GetWindowSystem()->GetWindow();
        Framework::DebugConfig::Initialize(window);
    }

    // ===============================================================================
    // GAME STATE MANAGER
    // ===============================================================================
    int initialState = ConfigReader::GetInitialGameState(mainMenu);
    GSM_Initialize(initialState);

    LOG_INFO("CORE", "Entering GSM main loop...");
    LOG_INFO("CORE", "Debug Controls: F2 = Export Performance CSV");

    // Time tracking variables
    unsigned lastTime = timeGetTime();

    // ===============================================================================
    // GAME STATE MANAGER LOOP
    // ===============================================================================
    while (current != GS_QUIT)
    {
        // Check for window close
        if (engine->ShouldWindowClose()) {
            LOG_INFO("CORE", "Window close requested");
            break;
        }

        // ===============================================================================
        // STATE TRANSITION
        // ===============================================================================
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

        // ===============================================================================
        // STATE LOOP
        // ===============================================================================
        LOG_INFO("CORE", "[GSM] Entering state loop...");

        while (next == current)
        {
            // Check engine and window
            if (!engine->IsActive() || engine->ShouldWindowClose()) {
                next = GS_QUIT;
                break;
            }

            // ===============================================================================
            // FIXED DELTA TIME IMPLEMENTATION (PDF Method)
            // ===============================================================================
            unsigned currentTime = timeGetTime();
            double deltaTime = (currentTime - lastTime) / 1000.0;
            lastTime = currentTime;

            // Safety clamp for extreme cases (e.g., debugging breakpoints)
            if (deltaTime > 0.25) {
                deltaTime = 0.25;  // Max 250ms to prevent spiral of death
            }

            // Accumulate actual time
            accumulatedTime += deltaTime;

            // Calculate how many fixed steps we need to execute
            currentNumberOfSteps = 0;
            while (accumulatedTime >= FIXED_DT) {
                accumulatedTime -= FIXED_DT;
                currentNumberOfSteps++;

                // Safety cap: prevent infinite loop if system is too slow
                if (currentNumberOfSteps >= 5) {
                    LOG_WARN("CORE", "Frame took too long! Capping at 5 physics steps");
                    accumulatedTime = 0.0;
                    break;
                }
            }

            // ===============================================================================
            // BEGIN FRAME
            // ===============================================================================
            eng::debug::PerfViewer::begin_frame();
            glfwPollEvents();

            // ===============================================================================
            // FIXED TIME STEP UPDATES (Physics/Logic ONLY)
            // ===============================================================================

            // Ensure at least one physics step
            if (currentNumberOfSteps == 0) {
                currentNumberOfSteps = 1;
            }

            // Execute physics/gameplay updates with FIXED_DT, N times
            for (int step = 0; step < currentNumberOfSteps; ++step) {
                // Update systems that require fixed timestep
                // This calls ImGuiSystem::Update() multiple times!
                // We'll handle this by making ImGui Update() skip if already drawn
                engine->UpdateSingleFrame(static_cast<float>(FIXED_DT));

                // State update (Lua scripts, game logic)
                if (fpUpdate) {
                    fpUpdate();
                }
            }

            // ===============================================================================
            // RENDERING (Always happens once per frame)
            // ===============================================================================
            if (fpDraw) {
                fpDraw();
            }

            // ===============================================================================
            // ImGui Frame (ONCE per visual frame, AFTER game rendering)
            // ===============================================================================
            if (engine->GetImGuiSystem()) {
                engine->GetImGuiSystem()->NewFrame();
                engine->GetImGuiSystem()->UpdateUI();
                engine->GetImGuiSystem()->Render();
            }

            // ===============================================================================
            // SWAP BUFFERS - Display everything on screen
            // ===============================================================================
            if (engine->GetWindowSystem() && engine->GetWindowSystem()->GetWindow()) {
                glfwSwapBuffers(engine->GetWindowSystem()->GetWindow());
            }

            // ===============================================================================
            // END FRAME
            // ===============================================================================
            eng::debug::PerfViewer::end_frame();

            // Update FPS counter with actual deltaTime
            Framework::DebugConfig::GetFpsCounter().tick_with_dt(deltaTime);

            // Debug hotkey: F2 to export performance data
            if (GetAsyncKeyState(VK_F2) & 0x0001) {
                if (eng::debug::PerfViewer::export_csv("performance.csv")) {
                    LOG_INFO("DEBUG", "Performance data exported");
                }
            }
        }

        LOG_INFO("CORE", "[GSM] Exiting state loop");

        // ===============================================================================
        // STATE CLEANUP
        // ===============================================================================
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

    // ===============================================================================
    // CLEANUP
    // ===============================================================================
    LOG_INFO("CORE", "GSM loop ended. Cleaning up...");

    engine->Cleanup();
    delete engine;
    engine = nullptr;

    LOG_INFO("CORE", "=================================================");
    LOG_INFO("CORE", "     Engine shutdown complete");
    LOG_INFO("CORE", "=================================================");

    Framework::DebugConfig::Shutdown();

#ifdef _DEBUG
    std::cout << "\nPress Enter to close console...\n";
    std::cin.get();
    FreeConsole();
#endif

    return 0;
}