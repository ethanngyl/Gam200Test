/**
===============================================================================
 File:          DebugConfig.h
 Author:        GE YONGQI
 Email:         yongqi.ge@digipen.edu
 Date:          2025-10-31
 Contribution:  100%
 ------------------------------------------------------------------------------
 Description:    Debug system configuration manager
                 Reads debug settings from game_config.txt and initializes
                 all debug systems in one call
===============================================================================
 */

#pragma once
#include "Precompiled.h"


namespace Framework {

    /**
     * @class DebugConfig
     * @brief Manages debug system initialization from config file
     *
     * Usage:
     *   DebugConfig::Initialize();  // That's it!
     */
    class DebugConfig
    {
    public:
        /**
         * @brief Initialize all debug systems from config file
         *
         * Reads settings from game_config.txt and initializes:
         * - Logging system
         * - Performance monitoring
         * - Crash logger
         * - FPS counter (if window provided)
         *
         * @param window Optional: GLFW window for FPS display
         * @return true if initialization successful
         */
        static bool Initialize(GLFWwindow* window = nullptr);

        /**
         * @brief Shutdown all debug systems
         */
        static void Shutdown();

        /**
         * @brief Get the FPS counter instance
         * @return Reference to FPS counter
         */
        static eng::debug::FpsCounter& GetFpsCounter();

        /**
         * @brief Reload debug configuration from file
         * @return true if reload successful
         */
        static bool Reload();

        /**
         * @brief Check if debug systems are initialized
         * @return true if initialized
         */
        static bool IsInitialized() { return initialized; }

    private:
        static bool initialized;
        static eng::debug::FpsCounter fpsCounter;

        /**
         * @brief Parse log level string to enum
         */
        static eng::debug::LogLevel ParseLogLevel(const std::string& levelStr);

        /**
         * @brief Initialize logging system
         */
        static void InitializeLogging();

        /**
         * @brief Initialize performance monitoring
         */
        static void InitializePerformance();

        /**
         * @brief Initialize crash logger
         */
        static void InitializeCrashLogger();

        /**
         * @brief Initialize FPS counter
         */
        static void InitializeFpsCounter(GLFWwindow* window);
    };

} // namespace Framework