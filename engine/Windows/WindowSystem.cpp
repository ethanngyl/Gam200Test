#include "Precompiled.h"

/*
===============================================================================
File:        WindowSystem.cpp
Author:      Sim Kah Yan
Email:       kahyan.sim@digipen.edu
Date:        2025-10-02
Contribution: 20%(Kah Yan)
-------------------------------------------------------------------------------
Brief:
Implementation of the WindowSystem class. Handles the creation and management
of the main application window using GLFW. Provides initialization, cleanup,
and basic engine message handling to support quitting.

Details:
- Initializes GLFW and creates a window with configurable width, height, and title.
- Destroys the window and terminates GLFW on shutdown.
- Responds to engine messages such as Quit.
- Checks for window close events and polls system events.

Notes:
- Default window size: 1600×800.
- Default title: "Struct Squad Game Engine".
- GLFW must be initialized successfully before creating a window.

Safety:
- All GLFW calls are wrapped with null checks.
- Properly destroys window and terminates GLFW to avoid resource leaks.
- Graceful fallback if initialization fails.

===============================================================================
*/


namespace Framework
{
    /*
    ------------------------------------------------------------------------------
    Constructor: Sets default window configuration and initializes member variables.
    ------------------------------------------------------------------------------
    */
    WindowSystem::WindowSystem() : window(nullptr),
        WindowOpen(false),
        windowWidth(1600),         // default width
        windowHeight(800),         // default height
        windowTitle("Struct Squad Game Engine") // default title
    {
    }

    /*
    ------------------------------------------------------------------------------
    Destructor: Cleans up GLFW resources by destroying the window and terminating
                the GLFW library.
    ------------------------------------------------------------------------------
    */
    WindowSystem::~WindowSystem()
    {
        // If a window exists, destroy it and terminate GLFW
        if (window) {
            glfwDestroyWindow(window);
            glfwTerminate();
        }
    }

    /*
    ------------------------------------------------------------------------------
    Initialize: Initializes the GLFW library and creates the main application
                window using the configured size and title.
    ------------------------------------------------------------------------------
    */
    void WindowSystem::Initialize()
    {
        std::cout << "WindowSystem: Initializing...\n";

        // Initialize the GLFW library
        if (!glfwInit()) {
            std::cerr << "GLFW init failed\n";
            return;
        }
        // Specify desired OpenGL version (4.5 Core Profile)
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        // Create the GLFW window using the configured width, height, and title
        window = glfwCreateWindow(windowWidth, windowHeight, windowTitle.c_str(), nullptr, nullptr);
        if (!window) {
            std::cerr << "Window creation failed\n";
            glfwTerminate();
            return;
        }

        // Mark window as open for the engine loop
        WindowOpen = true;

        std::cout << "WindowSystem: Window created! Press 'q' + Enter to quit.\n";
    }

    /*
    ------------------------------------------------------------------------------
    Update: Called once per frame to process OS window events.
            Keeps the window responsive to input and window manager actions.
    ------------------------------------------------------------------------------
    */
    void WindowSystem::Update(float dt)
    {
        // Poll events here to keep window responsive
        glfwPollEvents();
    }

    /*
    ------------------------------------------------------------------------------
    SendEngineMessage: Responds to engine-level messages such as Quit.
    Closes the window gracefully when a quit message is received.
    ------------------------------------------------------------------------------
    */
    void WindowSystem::SendEngineMessage(Message* message)
    {
        // Check for quit messages from the engine
        if (message->MessageId == Status::Quit)
        {
            std::cout << "WindowSystem: Received quit message, closing window.\n";
            WindowOpen = false;
        }
        // If a window exists, mark it for closure
        if (window) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
    }

    /*
   ------------------------------------------------------------------------------
   ShouldClose: Returns true if the GLFW window should close.
                This is checked each frame by the engine's main loop.
   ------------------------------------------------------------------------------
   */
    bool WindowSystem::ShouldClose() const
    {
        return window ? glfwWindowShouldClose(window) : true;
    }
}