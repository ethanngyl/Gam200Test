#include "Precompiled.h"
#include "Core.h"

#include "DebugComponents/PerfViewer.h"
#include "DebugComponents/Trace.h"
#include "DebugComponents/Perf.h"
#include "DebugComponents/Log.h"
#include "DebugComponents/CrashLogger.h"

#include <GLFW/glfw3.h>


namespace Framework
{
    // Define the global pointer
    CoreEngine* CORE = nullptr;

    CoreEngine::CoreEngine()
    {
        LastTime = 0;
        GameActive = true;
        CORE = this; // Set the global pointer
    }

    CoreEngine::~CoreEngine()
    {
        // Destructor - cleanup handled in DestroySystems()
    }

    void CoreEngine::Initialize()
    {
        //for (size_t i = 0; i < Systems.size(); ++i)
        //    Systems[i]->Initialize();

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

    void CoreEngine::GameLoop()
    {
        LastTime = timeGetTime();

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
            // Check for quit requests from the window system
            for (auto system : Systems) {
                if (auto windowSystem = dynamic_cast<WindowSystem*>(system)) {
                    if (windowSystem->ShouldClose()) {
                        Message quitMsg(Status::Quit);
                        BroadcastMessage(&quitMsg);
                    }
                }
            }

            // Delta time calculation
            unsigned currenttime = timeGetTime();
            float dt = (currenttime - LastTime) / 1000.0f;
            LastTime = currenttime;

            // Begin profiling frame
            eng::debug::PerfViewer::begin_frame();

            // Update all systems with scoped timers
            for (unsigned i = 0; i < Systems.size(); ++i)
            {
                eng::debug::Subsystem tag =
                    (i == 0) ? eng::debug::Subsystem::Graphics :
                    (i == 1) ? eng::debug::Subsystem::Gameplay :
                    eng::debug::Subsystem::Other;

                DBG_SCOPE_SYS("SystemUpdate", tag);
                Systems[i]->Update(dt);
            }

            // End profiling frame
            eng::debug::PerfViewer::end_frame();

            // FPS counter (updates log + window title)
            fps.tick_with_dt(static_cast<double>(dt));

            // Export CSV when F2 is pressed
            if (GetAsyncKeyState(VK_F2) & 0x0001)
            {
                eng::debug::PerfViewer::export_csv("perf_recent.csv");
            }

            // Crash on demand when F3 is pressed
            if (GetAsyncKeyState(VK_F3) & 0x0001) {
                eng::debug::CrashLogger::force_crash_for_test();
            }
        }
    }

    void CoreEngine::BroadcastMessage(Message* message)
    {
        // Handle quit message
        if (message->MessageId == Status::Quit)
            GameActive = false;

        // Send to all systems
        for (unsigned i = 0; i < Systems.size(); ++i)
            Systems[i]->SendEngineMessage(message);
    }

    void CoreEngine::AddSystem(InterfaceSystem* system)
    {
        Systems.push_back(system);
    }

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