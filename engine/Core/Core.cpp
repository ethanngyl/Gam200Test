#include "Precompiled.h"
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
            for (unsigned i = 0; i < Systems.size(); ++i) {
                using eng::debug::Subsystem;
                Subsystem tag = Subsystem::Other;

                if (dynamic_cast<GraphicsSystem*>(Systems[i]))        tag = Subsystem::Graphics;
                else if (dynamic_cast<MovementSystem*>(Systems[i]))   tag = Subsystem::IO;
                else if (dynamic_cast<CollisionSystem*>(Systems[i]))  tag = Subsystem::Physics;

                // else if (dynamic_cast<AudioSystem*>(Systems[i]))   tag = Subsystem::Audio;
                // else if (dynamic_cast<IOSystem*>(Systems[i]))      tag = Subsystem::IO;
                    

                { // ensure destructor runs before end_frame()
                    DBG_SCOPE_SYS("SystemUpdate", tag);
                    Systems[i]->Update(dt);
                }
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