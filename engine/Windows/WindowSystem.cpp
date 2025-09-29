#include "Precompiled.h"
#include "WindowSystem.h"
#include "Message.h"
#include "Core.h"
#include <iostream>

namespace Framework
{
    WindowSystem::WindowSystem()
    {
        WindowOpen = false;
    }

    WindowSystem::~WindowSystem()
    {
        // Cleanup will go here later
    }

    void WindowSystem::Initialize()
    {
        std::cout << "WindowSystem: Initializing...\n";

        // For now, just simulate creating a window
        WindowOpen = true;

        std::cout << "WindowSystem: Window created! Press 'q' + Enter to quit.\n";
    }

    void WindowSystem::Update(float dt)
    {
        // For now, just check for basic input to quit
        // (We'll make this more sophisticated later)

        // Simple console input check
        //if (_kbhit()) // Windows-specific for now
        //{
        //    char key = _getch();
        //    if (key == 'q' || key == 'Q')
        //    {
        //        // Send quit message
        //        Message quitMsg(Mid::Quit);
        //        Framework::CORE->BroadcastMessage(&quitMsg);
        //    }
        //}
    }

    void WindowSystem::SendEngineMessage(Message* message)
    {
        // Handle any messages sent to the window system
        if (message->MessageId == Status::Quit)
        {
            std::cout << "WindowSystem: Received quit message, closing window.\n";
            WindowOpen = false;
        }
    }
}