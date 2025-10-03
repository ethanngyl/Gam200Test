#include "Precompiled.h"

/*
===============================================================================
File:        WindowSystem.cpp
co-Author:   Sim Kah Yan
co-Author:   TAN WEI LEONG
Email:       kahyan.sim@digipen.edu
Date:        2025-10-02
Contribution: 10%(Kah Yan), 10% (TAN WEI LEONG)
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
            glfwDestroyWindow(window);  // Destroys the GLFW window
            glfwTerminate();            // Terminates the GLFW library
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
            std::cerr << "GLFW init failed\n";  // If GLFW initialization fails, print error message
            return;
        }
        // Specify desired OpenGL version (4.5 Core Profile)
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        // Create the GLFW window using the configured width, height, and title
        window = glfwCreateWindow(windowWidth, windowHeight, windowTitle.c_str(), nullptr, nullptr);
        if (!window) {
            std::cerr << "Window creation failed\n";  // If window creation fails, print error message
            glfwTerminate();  // Terminate GLFW
            return;
        }

        // Mark window as open for the engine loop
        WindowOpen = true;

        // Output message confirming window creation
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
        (void)dt;
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
            WindowOpen = false;  // Set the flag to false to indicate the window should close
        }
        // If a window exists, mark it for closure
        if (window) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);  // Set the window to close in the next loop
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
        // If the window is valid, check if it should close based on GLFW's internal state
        return window ? glfwWindowShouldClose(window) : true;  // If no window, return true (always closing)
    }

}  // End of Framework namespace