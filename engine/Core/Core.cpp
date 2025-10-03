/**

===============================================================================
 File:           Core.cpp
 Author:         ETHAN NG YONG LE
 Email:          n.ethanyongle@digipen.edu
 Date:           2025-09-30
 Contribution:   100%
 ------------------------------------------------------------------------------
 
  Design notes:
  This file implements the CoreEngine class which serves as the central
 * orchestrator for the game engine. It manages the main game loop, system
 * initialization and updates, message broadcasting, and frame timing.
===============================================================================
 */
 
#include "Precompiled.h"
#include "Core.h"
#include "MovementSystem.h"
#include "PerfViewer.h"
#include "Trace.h"
#include "Perf.h"
#include "Log.h"
#include "CrashLogger.h"

namespace Framework
{
    // Global pointer to the core engine instance
    CoreEngine* CORE = nullptr;

    /**
     * @brief Constructs the CoreEngine and initializes the global pointer
     *
     * Initializes timing to zero, sets the game to active state, and
     * assigns the global CORE pointer for system-wide access.
     */
    CoreEngine::CoreEngine()
    {
        LastTime = 0;
        GameActive = true;
        CORE = this; // Set the global pointer
    }

    /**
     * @brief Destructor for CoreEngine
     *
     * @note Actual cleanup is handled by DestroySystems()
     */
    CoreEngine::~CoreEngine()
    {
        // Destructor - cleanup handled in DestroySystems()
    }

    /**
     * @brief Initializes all registered systems in dependency order
     *
     * This function performs a three-phase initialization:
     * 1. WindowSystem is initialized first to create the window
     * 2. GraphicsSystem receives the window handle
     * 3. All remaining systems are initialized
     *
     * @note Systems are initialized using dynamic_cast to identify types
     */
    void CoreEngine::Initialize()
    {
        // 1. First initialize WindowSystem (ensures window exists)
        for (auto system : Systems)
        {
            if (auto windowSystem = dynamic_cast<WindowSystem*>(system))
            {
                windowSystem->Initialize();
            }
        }

        // 2. Now set window for GraphicsSystem (after window is created)
        GLFWwindow* glfwWin = nullptr;
        for (auto system : Systems)
        {
            if (auto windowSystem = dynamic_cast<WindowSystem*>(system))
            {
                glfwWin = windowSystem->GetWindow();
                break;
            }
        }

        for (auto system : Systems)
        {
            if (auto graphicsSystem = dynamic_cast<GraphicsSystem*>(system))
            {
                graphicsSystem->SetWindow(glfwWin);
            }
        }

        // 3. Initialize all systems (skip WindowSystem if already initialized)
        for (auto system : Systems)
        {
            if (dynamic_cast<WindowSystem*>(system) == nullptr)
            {
                system->Initialize();
            }
        }
    }

    /**
     * @brief Main game loop that runs until GameActive is false
     *
     * Executes the following operations each frame:
     * - Calculates delta time between frames
     * - Checks for window close requests
     * - Updates all registered systems with delta time
     * - Monitors performance metrics and FPS
     * - Handles debug hotkeys (F2 for CSV export, F3 for crash test)
     *
     * @note Uses timeGetTime() for frame timing with 60 FPS fallback
     * @note Integrates with eng::debug::PerfViewer for performance monitoring
     */
    void CoreEngine::GameLoop()
    {
        // Initialize timing for first frame
        LastTime = timeGetTime();  // original timing anchor

        // Debug tools
        eng::debug::FpsCounter fps;
        fps.set_enable_logging(true);

        // Attach window title updater
        for (auto system : Systems)
        {
            if (auto windowSystem = dynamic_cast<WindowSystem*>(system))
            {
                GLFWwindow* win = windowSystem->GetWindow();
                fps.set_title_updater([win](const char* title) {
                    glfwSetWindowTitle(win, title);
                    });
            }
        }

        while (GameActive)
        {
            // Check if window should close
            for (auto system : Systems) {
                if (auto windowSystem = dynamic_cast<WindowSystem*>(system)) {
                    if (windowSystem->ShouldClose()) {
                        Message quitMsg(Status::Quit);
                        BroadcastMessage(&quitMsg);
                    }
                }
            }

            // Calculate delta time
            unsigned currenttime = timeGetTime();
            float dt = (currenttime - LastTime) / 1000.0f;
            // Ensure minimum dt to prevent zero
            if (dt < 0.001f) dt = 0.016f;  // Default to ~60 FPS if too fast

            // Debug timing
            static int frameCount = 0;
            //if (frameCount++ % 60 == 0) {  // Print every 60 frames
                //std::cout << "LastTime: " << LastTime
                    //<< ", CurrentTime: " << currenttime
                    //<< ", dt: " << dt << "\n";
            //}
            LastTime = currenttime;


            // --- begin perf frame ---
            eng::debug::PerfViewer::begin_frame();

            // --- per-system updates with scoped timers ---
            for (unsigned i = 0; i < Systems.size(); ++i)
            {
                // Tag systems (adjust to your actual system types/order)
                eng::debug::Subsystem tag =
                    (i == 0) ? eng::debug::Subsystem::Graphics :
                    (i == 1) ? eng::debug::Subsystem::Gameplay :
                    eng::debug::Subsystem::Other;

                //DBG_SCOPE_SYS("SystemUpdate", tag);
                Systems[i]->Update(dt);
            }

            // --- end perf frame ---
            eng::debug::PerfViewer::end_frame();

            // --- FPS (uses your dt directly; logs once/sec) ---
            fps.tick_with_dt(static_cast<double>(dt));

            // --- CSV export when F2 is pressed (edge-triggered) ---
            // 0x0001 bit = key transitioned from up to down since last call.
            if (GetAsyncKeyState(VK_F2) & 0x0001)
            {
                eng::debug::PerfViewer::export_csv("perf_recent.csv");
            }
            // Crash-on-demand (F3). Fires once per key press.
            if (GetAsyncKeyState(VK_F3) & 0x0001) {
                eng::debug::CrashLogger::force_crash_for_test();
            }

        }
    }

    /**
     * @brief Broadcasts a message to all registered systems
     *
     * Special handling for Quit messages:
     * - Sets GameActive to false to terminate the game loop
     *
     * @param message Pointer to the message to broadcast
     * @note All systems receive the message via SendEngineMessage()
     */
    void CoreEngine::BroadcastMessage(Message* message)
    {
        // Handle quit message
        if (message->MessageId == Status::Quit)
            GameActive = false;

        // Send to all systems
        for (unsigned i = 0; i < Systems.size(); ++i)
            Systems[i]->SendEngineMessage(message);
    }

    /**
     * @brief Adds a system to the engine's system list
     *
     * @param system Pointer to the system to add
     * @note Systems are updated in the order they are added
     */
    void CoreEngine::AddSystem(InterfaceSystem* system)
    {
        Systems.push_back(system);
    }

    /**
     * @brief Destroys all registered systems in reverse order
     *
     * Deletes systems in reverse order of addition to minimize
     * dependency issues during cleanup.
     *
     * @note Clears the Systems vector after deletion
     */
    void CoreEngine::DestroySystems()
    {
        // Delete in reverse order
        for (unsigned i = 0; i < Systems.size(); ++i)
        {
            delete Systems[Systems.size() - i - 1];
        }
        Systems.clear();
    }
}