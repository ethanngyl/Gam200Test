/*
===============================================================================
 File:          WindowSystem.h
 Author:        
 Email:         
 Date:          2025-10-02
 Contribution:  100%
 ------------------------------------------------------------------------------
 Declaration of the `WindowSystem` class, responsible for managing the creation,
 management, and destruction of a GLFW-based window. This class encapsulates the
 logic for handling window-specific operations in a game engine or graphical
 application.

 Description:
 -------------
 This header file defines the `WindowSystem` class, which is part of a larger 
 framework that handles window creation, interaction, and destruction using the 
 GLFW library. The class manages a window, provides basic functions to update 
 window state, and handles shutdown messages.

 Responsibilities:
 -----------------
 - `WindowSystem()`: Constructor initializes the window with default values such 
   as width, height, and title.
 - `~WindowSystem()`: Destructor ensures that the GLFW window is properly destroyed 
   and GLFW is terminated.
 - `Initialize()`: Initializes GLFW, sets OpenGL context version, and creates 
   the window.
 - `Update()`: Polls for window events (e.g., user input, window state changes) 
   and keeps the window responsive.
 - `SendEngineMessage(Message* message)`: Processes engine messages (e.g., quit 
   messages), controlling window state (e.g., closing the window).
 - `ShouldClose()`: Checks if the window should be closed, useful for controlling 
   the game loop.
 - `GetWindow()`: Returns the `GLFWwindow` pointer, which is used to interact with 
   the underlying window.
 - `SetWindowSize(int w, int h)`: Sets the window's size to the specified width 
   and height.
 - `SetWindowTitle(const std::string& title)`: Sets the window's title to the 
   provided string.

 Platform-specific Notes:
 -------------------------
 - The class depends on the GLFW library for window management and OpenGL context 
   handling.
 - The class assumes that the OpenGL context and necessary setups (such as GLEW) 
   are done elsewhere in the application.

 Safety:
 --------
 - The class ensures proper cleanup of OpenGL resources by destroying the GLFW 
   window and terminating GLFW in the destructor.
 - It also provides safe handling for window size and title changes, ensuring 
   that the system remains stable throughout the application lifecycle.
===============================================================================
*/

#pragma once
#include "Precompiled.h"  // Includes essential precompiled headers for the graphics system.

// Forward declaration
struct GLFWwindow;

namespace Framework {

    // WindowSystem class manages a GLFW window and integrates it into the engine framework.
    class WindowSystem : public InterfaceSystem {
    public:
        // Constructor: Initializes default values for the window system
        WindowSystem();

        // Destructor: Ensures proper cleanup and termination of the GLFW window and context
        virtual ~WindowSystem();

        // ISystem interface method overrides for initialization and updates
        virtual void Initialize() override;      // Initializes GLFW, creates window
        virtual void Update(float dt) override;  // Polls window events, updates state
        virtual void SendEngineMessage(Message* message) override;  // Handles system messages (e.g., quit)

        // Getters
        GLFWwindow* GetWindow() const { return window; }  // Accessor for GLFW window handle
        bool ShouldClose() const;  // Checks if the window should close (i.e., if user initiated close)

        // Setters for window size and title
        void SetWindowSize(int w, int h) { windowWidth = w; windowHeight = h; }  // Set window width and height
        void SetWindowTitle(const std::string& title) { windowTitle = title; }  // Set window title

    private:
        GLFWwindow* window;  // Pointer to the GLFW window object
        bool WindowOpen;     // Flag indicating whether the window is open or closed

        // Configuration variables
        int windowWidth;  // Window width in pixels
        int windowHeight; // Window height in pixels
        std::string windowTitle;  // Window title string
    };

}  // End of Framework namespace