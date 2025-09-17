#include "Precompiled.h"
#include "Core.h"

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
        LastTime = timeGetTime();

        while (GameActive)
        {
            // Calculate delta time
            unsigned currenttime = timeGetTime();
            float dt = (currenttime - LastTime) / 1000.0f;
            LastTime = currenttime;

            // Update all systems
            for (unsigned i = 0; i < Systems.size(); ++i)
                Systems[i]->Update(dt);
        }
    }

    void CoreEngine::BroadcastMessage(Message* message)
    {
        // Handle quit message
        if (message->MessageId == Status::Quit)
            GameActive = false;

        // Send to all systems
        for (unsigned i = 0; i < Systems.size(); ++i)
            Systems[i]->SendMessage(message);
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