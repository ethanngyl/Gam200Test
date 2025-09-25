#include "Precompiled.h"
#include "WindowSystem.h"
#include "Message.h"
#include "Core.h"
#include <GLFW/glfw3.h>

namespace Framework
{
    WindowSystem::WindowSystem() : window(nullptr), WindowOpen(false)
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
        //std::cout << "WindowSystem: Initializing...\n";

        //// For now, just simulate creating a window
        //WindowOpen = true;

        //std::cout << "WindowSystem: Window created! Press 'q' + Enter to quit.\n";
        std::cout << "WindowSystem: Initializing...\n";

        if (!glfwInit()) {
            std::cerr << "GLFW init failed\n";
            return;
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        window = glfwCreateWindow(800, 600, "Struct Squad Game Engine", nullptr, nullptr);
        if (!window) {
            std::cerr << "Window creation failed\n";
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
        if (window) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
    }

    bool WindowSystem::ShouldClose() const
    {
        return window ? glfwWindowShouldClose(window) : true;
    }
}