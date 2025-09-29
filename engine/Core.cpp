#include "Precompiled.h"
#include "Core.h"

#include "DebugComponents/PerfViewer.h"
#include "DebugComponents/Trace.h"
#include "DebugComponents/Perf.h"
#include "DebugComponents/Log.h"
#include "DebugComponents/CrashLogger.h"

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
        for (unsigned i = 0; i < Systems.size(); ++i)
            Systems[i]->Initialize();
    }

    void CoreEngine::GameLoop()
    {
        // Initialize timing for first frame
        LastTime = timeGetTime();  // original timing anchor

        // Debug tools
        eng::debug::FpsCounter fps;
        fps.set_enable_logging(true);

        while (GameActive)
        {
            // --- dt (seconds) ---
            unsigned currenttime = timeGetTime();
            float dt = (currenttime - LastTime) / 1000.0f;
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

                DBG_SCOPE_SYS("SystemUpdate", tag);
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