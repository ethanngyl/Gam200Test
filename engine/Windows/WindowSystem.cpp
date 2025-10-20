/*
===============================================================================
 File:          WindowSystem.cpp
 Author:        Sim Kah Yan, TAN WEI LEONG
 Email:         kahyan.sim@digipen.edu, weileong.tan@digipen.edu
 Date:          2025-10-02
 Contribution:  20%(Kah Yan), 80% (TAN WEI LEONG)
 ------------------------------------------------------------------------------
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
- Default window size: 1600x800.
- Default title: "Struct Squad Game Engine".
- GLFW must be initialized successfully before creating a window.

 Safety:
- All GLFW calls are wrapped with null checks.
- Properly destroys window and terminates GLFW to avoid resource leaks.
- Graceful fallback if initialization fails.

===============================================================================
*/

#include "Precompiled.h"  // Includes essential precompiled headers for the graphics system.

namespace Framework {

    /*
    ------------------------------------------------------------------------------
    Constructor: Sets default window configuration and initializes member variables.
    ------------------------------------------------------------------------------
    */
    WindowSystem::WindowSystem()
        : window(nullptr),                      // Initialize the window pointer to nullptr
        WindowOpen(false),                      // Flag indicating whether the window is open
        windowWidth(1600),                      // Default width of the window
        windowHeight(800),                      // Default height of the window
        windowTitle("Struct Squad Game Engine") // Default title of the window
    {
    }

    /*
    ------------------------------------------------------------------------------
    Destructor: Cleans up GLFW resources by destroying the window and terminating
                the GLFW library.
    ------------------------------------------------------------------------------
    */
    WindowSystem::~WindowSystem() {
        // If the window exists, destroy it and terminate the GLFW library
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
    void WindowSystem::Initialize() {
        std::cout << "WindowSystem: Initializing...\n";  // Output initialization message

        // Initialize GLFW
        if (!glfwInit()) {
            std::cerr << "GLFW init failed\n";  // If GLFW initialization fails, print error message
            return;
        }

        // Set GLFW window hints for OpenGL context version and profile
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);  // Set OpenGL major version (4.x)
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);  // Set OpenGL minor version (4.x)
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // Set OpenGL profile to core

        // Create the GLFW window with the given width, height, and title
        window = glfwCreateWindow(windowWidth, windowHeight, windowTitle.c_str(), nullptr, nullptr);
        if (!window) {
            std::cerr << "Window creation failed\n";  // If window creation fails, print error message
            glfwTerminate();  // Terminate GLFW
            return;
        }
        
        WindowOpen = true;  // Set window open flag to true

        // Output message confirming window creation
        std::cout << "WindowSystem: Window created! Press 'q' + Enter to quit.\n";
    }

    /*
    ------------------------------------------------------------------------------
    Update: Called once per frame to process OS window events.
            Keeps the window responsive to input and window manager actions.
    ------------------------------------------------------------------------------
    */
    void WindowSystem::Update(float dt) {

        // Poll events (such as key presses, window resize, etc.)
        (void)dt;
        glfwPollEvents();  // Ensures the window responds to user interactions
    }

    /*
    ------------------------------------------------------------------------------
    SendEngineMessage: Responds to engine-level messages such as Quit.
    Closes the window gracefully when a quit message is received.
    ------------------------------------------------------------------------------
    */
    void WindowSystem::SendEngineMessage(Message* message) {
        // If the quit message is received, close the window
        if (message->MessageId == Status::Quit) {
            std::cout << "WindowSystem: Received quit message, closing window.\n";
            WindowOpen = false;  // Set the flag to false to indicate the window should close
        }

        // If the window is valid, signal that the window should close
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