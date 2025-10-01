<<<<<<< HEAD
#include "Precompiled.h"
=======
﻿#include "Precompiled.h"
#include "WindowSystem.h"
#include "Message.h"
#include "Core.h"
#include <GLFW/glfw3.h>
>>>>>>> parent of 88546ef (Rearranged files)

namespace Framework
{
    WindowSystem::WindowSystem() : window(nullptr),
        WindowOpen(false),
        windowWidth(1800),         // default width
        windowHeight(900),         // default height
        windowTitle("Struct Squad Game Engine") // default title
    {
    }

    WindowSystem::~WindowSystem()
    {
        if (window) {
            glfwDestroyWindow(window);
            glfwTerminate();
        }
    }

    void WindowSystem::Initialize()
    {
        std::cout << "WindowSystem: Initializing...\n";

        if (!glfwInit()) {
            std::cerr << "GLFW init failed\n";
            return;
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        // use configurable values here
        window = glfwCreateWindow(windowWidth, windowHeight, windowTitle.c_str(), nullptr, nullptr);
        if (!window) {
            std::cerr << "Window creation failed\n";
            glfwTerminate();
            return;
        }
        
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

        // Poll events here to keep window responsive
        glfwPollEvents();
    }

    void WindowSystem::SendEngineMessage(Message* message)
    {
        // Handle any messages sent to the window system
        if (message->MessageId == Status::Quit)
        {
            std::cout << "WindowSystem: Received quit message, closing window.\n";
            WindowOpen = false;
        }
        if (window) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
    }

    bool WindowSystem::ShouldClose() const
    {
        return window ? glfwWindowShouldClose(window) : true;
    }
}