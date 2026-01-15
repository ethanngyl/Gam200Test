#pragma once
#include "Precompiled.h"
/*
===============================================================================
File:        WindowSystem.h (With Config Support)
Author:      Sim Kah Yan
Modified by: GE YONGQI (Added config file support)
Email:       kahyan.sim@digipen.edu
Date:        2025-10-02
Contribution: 10%(Kah Yan)
-------------------------------------------------------------------------------
Brief:
Declares the WindowSystem class, which is responsible for creating and managing
the application window using GLFW. Now supports reading window configuration
from game_config.txt.

Details:
- Wraps GLFW window creation and management in a system-style interface.
- Reads window settings from config file (width, height, title, fullscreen).
- Exposes getters and setters for window configuration.
- Integrates with the engine messaging system through SendEngineMessage().
- Provides basic checks to determine if the window should close.

Config File Support:
The following keys are read from game_config.txt:
- window_width: Window width in pixels (default: 1600)
- window_height: Window height in pixels (default: 800)
- window_title: Window title string (default: "Struct Squad Game Engine")
- fullscreen: Enable fullscreen mode (default: false)

Notes:
- This class must be initialized before rendering systems that depend on the
  window context (e.g., GraphicsSystem).
- WindowSystem follows the InterfaceSystem pattern used by the engine.
- Config file is optional - default values are used if not found.

Safety:
- Uses forward declaration of GLFWwindow to avoid heavy includes.
- Properly cleans up window resources in destructor.
- Ensures window pointer is valid before returning or using it.

===============================================================================
*/

// Forward declaration to avoid including <GLFW/glfw3.h> here
struct GLFWwindow;

namespace Framework {
    /*
    ------------------------------------------------------------------------------
    WindowSystem:
    Manages the application window using GLFW, including configuration,
    initialization, updating, and cleanup. Now supports loading configuration
    from game_config.txt.

    @brief Manages the application window using GLFW with config file support.
    WindowSystem handles the creation, configuration, and lifetime
    of the main GLFW window. It reads window settings from game_config.txt
    and provides basic setters for size and title. Participates in the
    engine's system interface by implementing Initialize(), Update(),
    and SendEngineMessage().
    ------------------------------------------------------------------------------*/

    class WindowSystem : public EngineSystem
    {
    public:
        // Constructor: Initializes default window configuration and loads from config file.
        WindowSystem();
        // Destructor: Cleans up and closes the GLFW window.
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

        // GetWindowSize: Get current window dimensions
        int GetWidth() const { return windowWidth; }
        int GetHeight() const { return windowHeight; }
        std::string GetTitle() const { return windowTitle; }

        // jiahao
		void SetFullScreen(bool isFullscreen);

    private:
        GLFWwindow* window;         // Pointer to GLFW window instance
        bool WindowOpen;            // True if the window is currently open

        // Window configuration variables
        int windowWidth;            // Configured window width
        int windowHeight;           // Configured window height
        std::string windowTitle;    // Configured window title

        // LoadWindowConfig: Loads window settings from config file
        void LoadWindowConfig();
    };

}  // End of Framework namespace