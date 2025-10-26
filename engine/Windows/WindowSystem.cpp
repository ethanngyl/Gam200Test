#include "Precompiled.h"
#include "ConfigReader/ConfigReader.h"  // ÃÌº”≈‰÷√∂¡»°∆˜

/*
===============================================================================
File:        WindowSystem.cpp (With Config Support)
co-Author:   Sim Kah Yan
co-Author:   TAN WEI LEONG
Modified by: GE YONGQI (Added config file support)
Email:       kahyan.sim@digipen.edu
Date:        2025-10-02
Contribution: 10%(Kah Yan), 10% (TAN WEI LEONG)
-------------------------------------------------------------------------------
Brief:
Implementation of the WindowSystem class. Handles the creation and management
of the main application window using GLFW. Now supports reading window
configuration from game_config.txt.

Details:
- Initializes GLFW and creates a window with configurable width, height, and title.
- Reads window settings from config file if available.
- Destroys the window and terminates GLFW on shutdown.
- Responds to engine messages such as Quit.
- Checks for window close events and polls system events.

Config File Support:
- window_width: Window width in pixels (default: 1600)
- window_height: Window height in pixels (default: 800)
- window_title: Window title string (default: "Struct Squad Game Engine")
- fullscreen: Enable fullscreen mode (default: false)

Notes:
- Config file is optional. If not found, uses default values.
- GLFW must be initialized successfully before creating a window.

===============================================================================
*/


namespace Framework
{
    /*
    ------------------------------------------------------------------------------
    Constructor: Sets default window configuration and initializes member variables.
                 Now loads configuration from game_config.txt if available.
    ------------------------------------------------------------------------------
    */
    WindowSystem::WindowSystem() : window(nullptr),
        WindowOpen(false),
        windowWidth(1600),         // default width
        windowHeight(800),         // default height
        windowTitle("Struct Squad Game Engine") // default title
    {
        // Load window configuration from config file
        LoadWindowConfig();
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
    LoadWindowConfig: Loads window configuration from game_config.txt
    ------------------------------------------------------------------------------
    */
    void WindowSystem::LoadWindowConfig()
    {
        // Try to load config file
        ConfigReader::LoadConfig("assets/game_config.txt");
        // ConfigReader will handle file not found gracefully

        // Read window dimensions
        windowWidth = ConfigReader::GetInt("window_width", 1600);
        windowHeight = ConfigReader::GetInt("window_height", 800);

        // Read window title
        windowTitle = ConfigReader::GetString("window_title", "Struct Squad Game Engine");

        // Log the configuration
        LOG_INFO("WINDOW", "Window config loaded:");
        LOG_INFO("WINDOW", "  Size: %dx%d", windowWidth, windowHeight);
        LOG_INFO("WINDOW", "  Title: %s", windowTitle.c_str());
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
        static bool alreadyInitialized = false;
        if (alreadyInitialized) return;
        alreadyInitialized = true;

        // Initialize the GLFW library
        if (!glfwInit()) {
            std::cerr << "GLFW init failed\n";  // If GLFW initialization fails, print error message
            return;
        }

        // Specify desired OpenGL version (4.5 Core Profile)
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        // Check if fullscreen is requested
        bool fullscreen = ConfigReader::GetBool("fullscreen", false);
        GLFWmonitor* monitor = fullscreen ? glfwGetPrimaryMonitor() : nullptr;

        // Create the GLFW window using the configured width, height, and title
        window = glfwCreateWindow(windowWidth, windowHeight, windowTitle.c_str(), monitor, nullptr);
        if (!window) {
            std::cerr << "Window creation failed\n";  // If window creation fails, print error message
            glfwTerminate();  // Terminate GLFW
            return;
        }

        // Mark window as open for the engine loop
        WindowOpen = true;

        // Output message confirming window creation
        std::cout << "WindowSystem: Window created! Press 'q' + Enter to quit.\n";
        std::cout << "WindowSystem: Size: " << windowWidth << "x" << windowHeight << "\n";
        std::cout << "WindowSystem: Title: " << windowTitle << "\n";
        if (fullscreen) {
            std::cout << "WindowSystem: Fullscreen mode enabled\n";
        }
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