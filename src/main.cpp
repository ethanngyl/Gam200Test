/*
===============================================================================
 File:          main.cpp
 Author:        GE YONGQI
 Email:         yongqi.ge@digipen.edu
 Date:          2025-10-31
 Contribution:  100%
 ------------------------------------------------------------------------------
  Main entry point of the StructSquad Engine

  Modified: 2025-11-22
  - Simplified pause handling (moved to Core.cpp)
  - Removed duplicate pause checks
  - Engine systems now handle pause state internally

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
#include "Pause/Pause.h"
#include "GlobalPauseManager.h"
#include "WindowEventHandler.h"
#include "EditorModeManager.h"
#include "GameRuntime/GameBootstrap.h"



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
    FILE* fp_stdout = nullptr;
    FILE* fp_stderr = nullptr;
    FILE* fp_stdin = nullptr;
    freopen_s(&fp_stdout, "CONOUT$", "w", stdout);
    freopen_s(&fp_stderr, "CONOUT$", "w", stderr);
    freopen_s(&fp_stdin, "CONIN$", "r", stdin);

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

    Framework::GameBootstrap::Initialize();

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

#ifdef STRUCTSQUAD_START_IN_EDITOR
    engine->SetEditorMode(true);
    Framework::EditorMode::SetEditorMode(true);
    LOG_INFO("CORE", "Startup override: editor mode enabled by build config");
#endif

    // ===============================================================================
    // GAME STATE MANAGER
    // ===============================================================================
    int initialState = ConfigReader::GetInitialGameState(mainMenu);
#ifdef STRUCTSQUAD_INITIAL_STATE_ALIAS
    initialState = ConfigReader::ResolveStateName(STRUCTSQUAD_INITIAL_STATE_ALIAS, initialState);
    LOG_INFO("CORE", "Startup override: initial state alias '%s'", STRUCTSQUAD_INITIAL_STATE_ALIAS);
#endif
    GSM_Initialize(initialState);

    LOG_INFO("CORE", "Entering GSM main loop...");
    LOG_INFO("CORE", "Debug Controls: F2 = Export Performance CSV, P = Pause/Resume");

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
        if (current == GS_RESTART)
        {
            next = previous;
            current = previous;
        }

        GSM_Update();

        if (fpLoad) {
            fpLoad();
        }

        if (fpInitialize) {
            fpInitialize();
        }

        // ===============================================================================
        // STATE LOOP
        // ===============================================================================

        while (next == current)
        {
            if (!engine->IsActive() || engine->ShouldWindowClose()) {
                next = GS_QUIT;
                break;
            }

            // ===============================================================================
            // DELTA TIME
            // ===============================================================================
            unsigned currentTime = timeGetTime();
            double deltaTime = (currentTime - lastTime) / 1000.0;
            lastTime = currentTime;

            if (deltaTime < FIXED_DT) {
                double sleepTime = (FIXED_DT - deltaTime);
                Sleep((DWORD)(sleepTime * 1000.0));
            }

            if (deltaTime > 0.25) {
                deltaTime = 0.25;
            }

            accumulatedTime += deltaTime;

            // ===============================================================================
            // BEGIN FRAME
            // ===============================================================================
            eng::debug::PerfViewer::begin_frame();
            glfwPollEvents();

            // ===============================================================================
            // WINDOW EVENT HANDLING (TECH 1701 & 1702)
            // ===============================================================================
            // Must be called BEFORE level update to handle:
            // - CTRL-ALT-DEL (auto-pause on focus loss)
            // - Window minimize/restore (auto-pause/resume)
            // - ALT-TAB (auto-pause)
            // - ALT-ENTER (fullscreen toggle)
            if (engine->GetWindowSystem() && engine->GetWindowSystem()->GetWindow()) {
                Framework::WindowEventHandler::Update(engine->GetWindowSystem()->GetWindow());
            }

            // ===============================================================================
            // LEVEL UPDATE (Handles pause toggle and level-specific input)
            // ===============================================================================
            if (fpUpdate) {
                fpUpdate();  // This is level1_Update() - handles pause detection
            }

            // ===============================================================================
            // FIXED TIME STEP UPDATES
            // ===============================================================================
            currentNumberOfSteps = 0;
            while (accumulatedTime >= FIXED_DT) {
                accumulatedTime -= FIXED_DT;
                currentNumberOfSteps++;

                if (currentNumberOfSteps >= 5) {
                    accumulatedTime = 0.0;
                    break;
                }
            }

            if (currentNumberOfSteps == 0) {
                currentNumberOfSteps = 1;
            }

            // ===============================================================================
            // ENGINE UPDATE (Handles pause internally)
            // ===============================================================================
            // CoreEngine::UpdateSingleFrame will:
            // - Always update Input and Graphics (prevents ghosting)
            // - Skip game logic systems when GlobalPause::IsPaused() is true
            for (int step = 0; step < currentNumberOfSteps; ++step) {
                engine->UpdateSingleFrame(static_cast<float>(FIXED_DT));
            }

            // ===============================================================================
            // LEVEL DRAW (Game-specific UI and pause menu overlay)
            // ===============================================================================
            if (fpDraw) {
                fpDraw();  // level1_Draw() - draws UI + pause menu if paused
            }

            Framework::PopUp::Render();

            // ===============================================================================
            // END FRAME
            // ===============================================================================

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
            Framework::DebugConfig::GetFpsCounter().tick_with_dt(deltaTime);

            // F2 to export performance
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

    Framework::LevelLoader::GetInstance().Shutdown();

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