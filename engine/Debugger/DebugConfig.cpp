/**
===============================================================================
 File:          DebugConfig.h
 Author:        GE YONGQI
 Email:         yongqi.ge@digipen.edu
 Date:          2025-10-31
 Contribution:  100%
 ------------------------------------------------------------------------------
 Description:    Debug system configuration manager implementation
===============================================================================
 */

#include "Precompiled.h"

namespace Framework {

    // ============================================================================
    // STATIC MEMBER DEFINITIONS
    // ============================================================================

    bool DebugConfig::initialized = false;
    eng::debug::FpsCounter DebugConfig::fpsCounter;

    // ============================================================================
    // PUBLIC METHODS
    // ============================================================================

    bool DebugConfig::Initialize(GLFWwindow* window)
    {
        if (initialized) {
            LOG_WARN("DEBUG", "Debug systems already initialized");
            return true;
        }

        LOG_INFO("DEBUG", "=================================================");
        LOG_INFO("DEBUG", "  Initializing Debug Systems from Config");
        LOG_INFO("DEBUG", "=================================================");

        // Ensure config is loaded
        if (!ConfigReader::IsConfigLoaded()) {
            ConfigReader::LoadConfig();
        }

        // Initialize subsystems
        InitializeLogging();
        InitializePerformance();
        InitializeCrashLogger();
        InitializeFpsCounter(window);

        initialized = true;

        LOG_INFO("DEBUG", "=================================================");
        LOG_INFO("DEBUG", "  Debug Systems Ready!");
        LOG_INFO("DEBUG", "=================================================");

        return true;
    }

    void DebugConfig::Shutdown()
    {
        if (!initialized) return;

        LOG_INFO("DEBUG", "Shutting down debug systems...");

        eng::debug::Log::shutdown();

        initialized = false;
    }

    eng::debug::FpsCounter& DebugConfig::GetFpsCounter()
    {
        return fpsCounter;
    }

    bool DebugConfig::Reload()
    {
        LOG_INFO("DEBUG", "Reloading debug configuration...");

        // Reload config file
        ConfigReader::ReloadConfig();

        // Reinitialize (currently only affects new log level)
        eng::debug::LogLevel newLevel = ParseLogLevel(
            ConfigReader::GetString("debug_log_level", "Info")
        );
        eng::debug::Log::set_level(newLevel);

        LOG_INFO("DEBUG", "Debug configuration reloaded");
        return true;
    }

    // ============================================================================
    // PRIVATE HELPER METHODS
    // ============================================================================

    eng::debug::LogLevel DebugConfig::ParseLogLevel(const std::string& levelStr)
    {
        std::string lower = levelStr;
        std::transform(lower.begin(), lower.end(), lower.begin(),
            [](unsigned char c) { return (char)std::tolower(c); });

        if (lower == "debug") return eng::debug::LogLevel::Debug;
        if (lower == "info")  return eng::debug::LogLevel::Info;
        if (lower == "warn")  return eng::debug::LogLevel::Warn;
        if (lower == "error") return eng::debug::LogLevel::Error;

        LOG_WARN("DEBUG", "Unknown log level '%s', using Info", levelStr.c_str());
        return eng::debug::LogLevel::Info;
    }

    void DebugConfig::InitializeLogging()
    {
        // Read logging configuration
        eng::debug::LogConfig logCfg;

        logCfg.level = ParseLogLevel(
            ConfigReader::GetString("debug_log_level", "Info")
        );

        logCfg.filePath = ConfigReader::GetString(
            "debug_log_file", "StructSquad.log"
        );

        logCfg.useConsole = ConfigReader::GetBool(
            "debug_use_console", true
        );

        logCfg.useFile = ConfigReader::GetBool(
            "debug_use_file", true
        );

        logCfg.usePlatformOutput = ConfigReader::GetBool(
            "debug_use_platform_output", true
        );

        logCfg.showSourceInfo = ConfigReader::GetBool(
            "debug_show_source_info", false
        );

        // Initialize logging system
        eng::debug::Log::init(logCfg);

        LOG_INFO("DEBUG", "Logging system initialized");
        LOG_INFO("DEBUG", "  Level: %s",
            ConfigReader::GetString("debug_log_level", "Info").c_str());
        LOG_INFO("DEBUG", "  File: %s", logCfg.filePath.c_str());
        LOG_INFO("DEBUG", "  Console: %s", logCfg.useConsole ? "ON" : "OFF");
        LOG_INFO("DEBUG", "  File Output: %s", logCfg.useFile ? "ON" : "OFF");
        LOG_INFO("DEBUG", "  Platform Output: %s", logCfg.usePlatformOutput ? "ON" : "OFF");
    }

    void DebugConfig::InitializePerformance()
    {
        // Read performance monitoring settings
        double printInterval = (double)ConfigReader::GetFloat(
            "debug_perf_print_interval", 1.0f
        );

        // Set performance viewer print interval
        eng::debug::PerfViewer::set_print_interval(printInterval);

        LOG_INFO("DEBUG", "Performance monitoring initialized");
        LOG_INFO("DEBUG", "  Print interval: %.1f seconds", printInterval);
    }

    void DebugConfig::InitializeCrashLogger()
    {
        // Check if crash logger is enabled
        bool enabled = ConfigReader::GetBool(
            "debug_crash_logger_enabled", true
        );

        if (enabled) {
            // Install crash handlers (will automatically load configuration)
            eng::debug::CrashLogger::install_handlers();

            // Get and display configuration
            const auto& config = eng::debug::CrashLogger::get_config();

            LOG_INFO("DEBUG", "Crash logger installed");
            LOG_INFO("DEBUG", "  Report prefix: %s", config.reportPrefix.c_str());
            LOG_INFO("DEBUG", "  Report extension: %s", config.reportExtension.c_str());
            LOG_INFO("DEBUG", "  Output directory: %s", config.reportDirectory.c_str());
            LOG_INFO("DEBUG", "  Max stack frames: %d", config.maxStackFrames);
        }
        else {
            LOG_INFO("DEBUG", "Crash logger disabled");
        }
    }

    void DebugConfig::InitializeFpsCounter(GLFWwindow* window)
    {
        // Check if FPS counter is enabled
        bool enabled = ConfigReader::GetBool(
            "debug_fps_enabled", true
        );

        if (!enabled) {
            LOG_INFO("DEBUG", "FPS counter disabled");
            return;
        }

        // Read FPS settings
        bool logging = ConfigReader::GetBool(
            "debug_fps_logging", true
        );

        float smoothing = ConfigReader::GetFloat(
            "debug_fps_smoothing", 0.2f
        );

        // Configure FPS counter
        fpsCounter.set_enable_logging(logging);
        fpsCounter.set_smoothing_alpha((double)smoothing);

        // Set window title updater if window provided
        if (window) {
            fpsCounter.set_title_updater([window](const char* title) {
                glfwSetWindowTitle(window, title);
                });
            LOG_INFO("DEBUG", "FPS counter initialized with window title updates");
        }
        else {
            LOG_INFO("DEBUG", "FPS counter initialized (no window title updates)");
        }

        LOG_INFO("DEBUG", "  FPS logging: %s", logging ? "ON" : "OFF");
        LOG_INFO("DEBUG", "  Smoothing: %.2f", smoothing);
    }

} // namespace Framework