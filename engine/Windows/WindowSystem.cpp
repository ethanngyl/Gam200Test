#include "Precompiled.h"
#include "ConfigReader/ConfigReader.h"  // 使用优化版 ConfigReader

/*
===============================================================================
File:        WindowSystem.cpp (Optimized Config Version)
co-Author:   Sim Kah Yan
co-Author:   TAN WEI LEONG
Modified by: GE YONGQI (Added optimized config file support)
Email:       kahyan.sim@digipen.edu
Date:        2025-10-02
Contribution: 10%(Kah Yan), 10% (TAN WEI LEONG)
-------------------------------------------------------------------------------
Brief:
Implementation of the WindowSystem class with optimized config loading.
Uses ConfigReader's unified path and duplicate loading prevention.

Config File Support:
- window_width: Window width in pixels (default: 1600)
- window_height: Window height in pixels (default: 800)
- window_title: Window title string (default: "Struct Squad Game Engine")
- fullscreen: Enable fullscreen mode (default: false)

===============================================================================
*/

namespace Framework
{
    /*
    ------------------------------------------------------------------------------
    Constructor: Sets default window configuration and loads from config file.
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
    Destructor: Cleans up GLFW resources
    ------------------------------------------------------------------------------
    */
    WindowSystem::~WindowSystem()
    {
        if (window) {
            glfwDestroyWindow(window);
            glfwTerminate();
        }
    }

    /*
    ------------------------------------------------------------------------------
    LoadWindowConfig: Loads window configuration from game_config.txt
    Uses ConfigReader's unified path and prevents duplicate loading
    ------------------------------------------------------------------------------
    */
    void WindowSystem::LoadWindowConfig()
    {
        // Load config file using unified path
        // This is safe to call multiple times - won't reload if already loaded
        ConfigReader::LoadConfig(ConfigReader::CONFIG_FILE_PATH);

        // Read window configuration
        windowWidth = ConfigReader::GetInt("window_width", 1600);
        windowHeight = ConfigReader::GetInt("window_height", 800);
        windowTitle = ConfigReader::GetString("window_title", "Struct Squad Game Engine");

        // Log the configuration
        LOG_INFO("WINDOW", "Window config loaded:");
        LOG_INFO("WINDOW", "  Size: %dx%d", windowWidth, windowHeight);
        LOG_INFO("WINDOW", "  Title: %s", windowTitle.c_str());
    }

    /*
    ------------------------------------------------------------------------------
    Initialize: Initializes GLFW and creates the window
    ------------------------------------------------------------------------------
    */
    void WindowSystem::Initialize()
    {
        std::cout << "WindowSystem: Initializing...\n";
        static bool alreadyInitialized = false;
        if (alreadyInitialized) return;
        alreadyInitialized = true;

        // Initialize GLFW
        if (!glfwInit()) {
            std::cerr << "GLFW init failed\n";
            return;
        }

        // Specify OpenGL version
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        // Check if fullscreen is requested
        bool fullscreen = ConfigReader::GetBool("fullscreen", false);
        GLFWmonitor* monitor = fullscreen ? glfwGetPrimaryMonitor() : nullptr;

        // Create the window
        window = glfwCreateWindow(windowWidth, windowHeight, windowTitle.c_str(), monitor, nullptr);
        if (!window) {
            std::cerr << "Window creation failed\n";
            glfwTerminate();
            return;
        }

        WindowOpen = true;

        // Output confirmation
        std::cout << "WindowSystem: Window created! Press 'q' + Enter to quit.\n";
        std::cout << "WindowSystem: Size: " << windowWidth << "x" << windowHeight << "\n";
        std::cout << "WindowSystem: Title: " << windowTitle << "\n";
        if (fullscreen) {
            std::cout << "WindowSystem: Fullscreen mode enabled\n";
        }
    }

    /*
    ------------------------------------------------------------------------------
    Update: Called once per frame to process OS window events
    ------------------------------------------------------------------------------
    */
    void WindowSystem::Update(float dt)
    {
        (void)dt;
        glfwPollEvents();
    }

    /*
    ------------------------------------------------------------------------------
    SendEngineMessage: Responds to engine-level messages such as Quit
    ------------------------------------------------------------------------------
    */
    void WindowSystem::SendEngineMessage(Message* message)
    {
        if (message->MessageId == Status::Quit)
        {
            std::cout << "WindowSystem: Received quit message, closing window.\n";
            WindowOpen = false;
        }
        if (window) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
    }

    /*
    ------------------------------------------------------------------------------
    ShouldClose: Returns true if the GLFW window should close
    ------------------------------------------------------------------------------
    */
    bool WindowSystem::ShouldClose() const
    {
        return window ? glfwWindowShouldClose(window) : true;
    }

}  // End of Framework namespace