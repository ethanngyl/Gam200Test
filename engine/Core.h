#pragma once
#include "Interface.h"
#include "Message.h"
#include <vector>

namespace Framework
{
    class CoreEngine
    {
    public:
        CoreEngine();
        ~CoreEngine();

        // Core functionality
        void Initialize();
        void GameLoop();

        // System management
        void AddSystem(InterfaceSystem* system);
        void DestroySystems();

        // Message system
        void BroadcastMessage(Message* message);

    private:
        // Systems collection
        std::vector<InterfaceSystem*> Systems;

        // Timing
        unsigned LastTime;

        // Game state
        bool GameActive;
    };

    // Global pointer to the core engine
    extern CoreEngine* CORE;
}