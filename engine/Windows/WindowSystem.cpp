/*
===============================================================================
 File:          WindowSystem.cpp
 Author:        TAN WEI LEONG
 Email:         weileong.tan@digipen.edu
 Date:          2025-10-02
 Contribution:  20% (TAN WEI LEONG)
 ------------------------------------------------------------------------------
 Implementation of the WindowSystem class, which handles the creation and
 management of a GLFW window. This class facilitates the interaction between
 the application and the windowing system (using GLFW) for rendering, input,
 and window lifecycle management.

 Description:
 -------------
 This file implements the methods of the `WindowSystem` class, responsible for 
 initializing, updating, and closing the window. The class also handles events 
 like window resizing and user input to keep the window responsive.

 Responsibilities:
 -----------------
 - `WindowSystem()`: Initializes the window with default values (width, height, title).
 - `~WindowSystem()`: Cleans up resources by destroying the window and terminating GLFW.
 - `Initialize()`: Initializes GLFW and creates a window with the specified width, height, and title.
 - `Update()`: Polls events to keep the window responsive and checks for updates each frame.
 - `SendEngineMessage()`: Handles messages, such as the quit message, and responds by closing the window.
 - `ShouldClose()`: Checks if the window should close (useful for the main loop to decide when to terminate).

 Platform-specific Notes:
 -------------------------
 - The class relies on GLFW for window management and OpenGL for rendering, assuming the 
   necessary OpenGL context setup is done elsewhere in the application.
 - Ensure that the OpenGL context is correctly set up before using this class.

 Safety:
 --------
 - Proper GLFW resource cleanup is ensured with the destructor to avoid memory leaks.
 - Event polling and window management functions are wrapped in checks to ensure proper execution.

===============================================================================
*/

#include "Precompiled.h"  // Includes essential precompiled headers for the graphics system.

namespace Framework {

    // Constructor for the WindowSystem class
    WindowSystem::WindowSystem()
        : window(nullptr),                      // Initialize the window pointer to nullptr
        WindowOpen(false),                      // Flag indicating whether the window is open
        windowWidth(1600),                      // Default width of the window
        windowHeight(800),                      // Default height of the window
        windowTitle("Struct Squad Game Engine") // Default title of the window
    {
    }

    // Destructor for the WindowSystem class
    WindowSystem::~WindowSystem() {
        // If the window exists, destroy it and terminate the GLFW library
        if (window) {
            glfwDestroyWindow(window);  // Destroys the GLFW window
            glfwTerminate();            // Terminates the GLFW library
        }
    }

    // Initializes the window system by setting up GLFW and creating the window
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

    // Updates the window system, called every frame to keep the window responsive
    void WindowSystem::Update(float dt) {
        // For now, just check for basic input to quit
        // Placeholder for future input handling (e.g., quitting)
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

        // Poll events (such as key presses, window resize, etc.)
        glfwPollEvents();  // Ensures the window responds to user interactions
    }

    // Handles engine messages sent to the window system, such as quit messages
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

    // Checks if the window should close (useful for controlling the main loop exit condition)
    bool WindowSystem::ShouldClose() const
    {
        // If the window is valid, check if it should close based on GLFW's internal state
        return window ? glfwWindowShouldClose(window) : true;  // If no window, return true (always closing)
    }

}  // End of Framework namespace