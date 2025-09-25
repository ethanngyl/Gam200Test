#include "Precompiled.h"
#include "WindowSystem.h"
#include "Message.h"
#include "Core.h"
#include <iostream>


namespace Framework
{
    WindowSystem::WindowSystem() : WindowOpen(false), window(nullptr)
    {
        //WindowOpen = false;
    }

    WindowSystem::~WindowSystem()
    {
        // Cleanup will go here later
    }

    void WindowSystem::Initialize()
    {
        //std::cout << "WindowSystem: Initializing...\n";

        //// For now, just simulate creating a window
        //WindowOpen = true;

        //std::cout << "WindowSystem: Window created! Press 'q' + Enter to quit.\n";
        std::cout << "WindowSystem: Initializing...\n";

        if (!glfwInit()) {
            std::cerr << "Failed to initialize GLFW!\n";
            return;
        }
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        // Create the window
        window = glfwCreateWindow(800, 600, "My Engine Window", nullptr, nullptr);
        if (!window) {
            std::cerr << "Failed to create GLFW window!\n";
            glfwTerminate();
            return;
        }

        glfwMakeContextCurrent(window);

        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);
        std::cout << "[WindowSystem] Viewport set to " << width << "x" << height << std::endl;

        // Initialize GLEW (for OpenGL function loading)
        glewExperimental = GL_TRUE;
        if (glewInit() != GLEW_OK) {
            std::cerr << "Failed to initialize GLEW!\n";
            return;
        }

        WindowOpen = true;
        std::cout << "WindowSystem: Window created!\n";
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
        if (window) {
            glfwSwapBuffers(window);
            glfwPollEvents();
            if (glfwWindowShouldClose(window)) {
                Message quitMsg(Status::Quit);
                Framework::CORE->BroadcastMessage(&quitMsg);
                WindowOpen = false;
            }
        }
    }

    void WindowSystem::SendEngineMessage(Message* message)
    {
        if (message->MessageId == Status::Quit) {
            std::cout << "WindowSystem: Received quit message, closing window.\n";
            WindowOpen = false;

            if (window) {
                glfwDestroyWindow(window);
                window = nullptr;
            }
            glfwTerminate();
        }
    }
}