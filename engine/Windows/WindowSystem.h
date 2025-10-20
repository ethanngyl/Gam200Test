/*
===============================================================================
 File:          WindowSystem.h
 Author:        Sim Kah Yan, TAN WEI LEONG
 Email:         kahyan.sim@digipen.edu, weileong.tan@digipen.edu
 Date:          2025-10-02
 Contribution:  20%(Kah Yan), 80% (TAN WEI LEONG)
 ------------------------------------------------------------------------------
 Brief:
 Declares the WindowSystem class, which is responsible for creating and managing
 the application window using GLFW. It provides basic configuration such as
 window size and title, handles initialization and cleanup, and communicates
 window-related messages to the engine.

 Details:
 - Wraps GLFW window creation and management in a system-style interface.
 - Exposes getters and setters for window configuration.
 - Integrates with the engine messaging system through SendEngineMessage().
 - Provides basic checks to determine if the window should close.

 Notes:
 - This class must be initialized before rendering systems that depend on the
   window context (e.g., GraphicsSystem).
 - WindowSystem follows the InterfaceSystem pattern used by the engine.

 Safety:
 - Uses forward declaration of GLFWwindow to avoid heavy includes.
 - Properly cleans up window resources in destructor.
 - Ensures window pointer is valid before returning or using it.
===============================================================================
*/

#pragma once
#include "Precompiled.h"  // Includes essential precompiled headers for the graphics system.

// Forward declaration
struct GLFWwindow;

namespace Framework {

    /*
    ------------------------------------------------------------------------------
    WindowSystem:
    Manages the application window using GLFW, including configuration,
    initialization, updating, and cleanup.
    @brief Manages the application window using GLFW.
    WindowSystem handles the creation, configuration, and lifetime
    of the main GLFW window. It provides basic setters for size
    and title, and participates in the engine's system interface
    by implementing Initialize(), Update(), and SendEngineMessage().
    ------------------------------------------------------------------------------*/

    class WindowSystem : public InterfaceSystem {
    public:
        // Constructor: Initializes default values for the window system
        WindowSystem();

        // Destructor: Ensures proper cleanup and termination of the GLFW window and context
        virtual ~WindowSystem();

        // Initialize: Creates the GLFW window with the specified configuration.
        virtual void Initialize() override;      
        // Update: Called once per frame to poll window events and update state.
        virtual void Update(float dt) override;  
        // SendEngineMessage: Receives engine-wide messages (e.g. Quit).
        virtual void SendEngineMessage(Message* message) override;  

        // GetWindow: Returns the raw GLFWwindow pointer for rendering systems.
        GLFWwindow* GetWindow() const { return window; }  
        // ShouldClose: Returns true if the window should close (user pressed X).
        bool ShouldClose() const;  

        // SetWindowSize / SetWindowTitle: Configure window before initialization.
        void SetWindowSize(int w, int h) { windowWidth = w; windowHeight = h; }  
        void SetWindowTitle(const std::string& title) { windowTitle = title; }  

    private:
        GLFWwindow* window;  // Pointer to GLFW window instance
        bool WindowOpen;     // True if the window is currently open

        // Configuration variables
        int windowWidth;  // Configured window width
        int windowHeight; // Configured window height
        std::string windowTitle;  // Configured window title
    };

}  // End of Framework namespace