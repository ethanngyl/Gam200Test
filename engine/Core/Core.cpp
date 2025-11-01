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
#include "ImguiSystem.h"

namespace Framework
{
    CoreEngine* CORE = nullptr;

    CoreEngine::CoreEngine()
    {
        LastTime = 0;
        GameActive = true;
        CORE = this;
    }

    CoreEngine::~CoreEngine()
    {
    }

    void CoreEngine::Initialize()
    {
        // 1. First initialize WindowSystem
        for (auto system : Systems)
        {
            if (auto windowSystem = dynamic_cast<WindowSystem*>(system))
            {
                windowSystem->Initialize();
            }
        }

        // 2. Get window handle
        GLFWwindow* glfwWin = nullptr;
        for (auto system : Systems)
        {
            if (auto windowSystem = dynamic_cast<WindowSystem*>(system))
            {
                glfwWin = windowSystem->GetWindow();
                break;
            }
        }

        // 3. Initialize all other systems
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

        // Find ImGui and Graphics systems
        ImGuiSystem* imguiSys = nullptr;
        GraphicsSystemV2* graphicsSys = nullptr;

        for (auto system : Systems)
        {
            if (auto imgui = dynamic_cast<ImGuiSystem*>(system)) {
                imguiSys = imgui;
            }
            if (auto graphics = dynamic_cast<GraphicsSystemV2*>(system)) {
                graphicsSys = graphics;
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
            if (dt < 0.001f) dt = 0.016f;
            LastTime = currenttime;

            // Begin perf frame
            eng::debug::PerfViewer::begin_frame();

            // ========================================================
            // Update all systems
            // ========================================================
            for (unsigned i = 0; i < Systems.size(); ++i)
            {
                eng::debug::Subsystem tag =
                    (i == 0) ? eng::debug::Subsystem::Graphics :
                    (i == 1) ? eng::debug::Subsystem::Gameplay :
                    eng::debug::Subsystem::Other;

                Systems[i]->Update(dt);
                (void)tag;
            }

            // ========================================================
            // FIXED: Render ImGui AFTER all systems (OUTSIDE loop)
            // ========================================================
            if (imguiSys) {
                imguiSys->Render();
            }

            // Swap buffers AFTER ImGui
            if (graphicsSys) {
                graphicsSys->RenderImGui();
            }

            // End perf frame
            eng::debug::PerfViewer::end_frame();

            // FPS counter
            fps.tick_with_dt(static_cast<double>(dt));

            // CSV export (F2)
            if (GetAsyncKeyState(VK_F2) & 0x0001)
            {
                eng::debug::PerfViewer::export_csv("perf_recent.csv");
            }

            // Crash test (F3)
            if (GetAsyncKeyState(VK_F3) & 0x0001) {
                eng::debug::CrashLogger::force_crash_for_test();
            }
        }
    }

    void CoreEngine::BroadcastMessage(Message* message)
    {
        if (message->MessageId == Status::Quit)
            GameActive = false;

        for (unsigned i = 0; i < Systems.size(); ++i)
            Systems[i]->SendEngineMessage(message);
    }

    void CoreEngine::AddSystem(EngineSystem* system)
    {
        Systems.push_back(system);
    }

    void CoreEngine::DestroySystems()
    {
        for (unsigned i = 0; i < Systems.size(); ++i)
        {
            delete Systems[Systems.size() - i - 1];
        }



        Systems.clear();
    }
}