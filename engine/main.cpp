#include "Platform.h"
// [IMPORTANT] This main entry file is only for Desktop Based applications support Window, Ubuntu, Mac etc
#ifdef PLATFORM_WINDOWS

#include "GLRenderer.h"
#include <iostream>

// === Debug tools ==
#include "DebugComponents/Log.h"
#include "DebugComponents/Perf.h"

#include "DebugComponents/Trace.h"      
#include "DebugComponents/PerfViewer.h"

#include "DebugComponents/CrashLogger.h"


int main() {
    using namespace eng::debug;

    // Initializing Logs
    LogConfig cfg;
    cfg.level = LogLevel::Info;
    cfg.filePath = "engine.log";      // Output Files
    cfg.showSourceInfo = false;       // Off (file:line)
    cfg.useConsole = true;            // Console(stderr)
    cfg.useFile = true;
    cfg.usePlatformOutput = true;     // VS Output
    Log::init(cfg);

    LOG_INFO0("CORE", "Starting OpenGL ES 3.0 Triangle Demo on Windows");

	// Install crash handlers
    eng::debug::CrashLogger::install_handlers();

    // Create renderer
    GLRenderer renderer;

    // Initialize
    if (!renderer.initialize()) {
        std::cerr << "Failed to initialize renderer" << std::endl;
        return -1;
    }

    LOG_INFO0("CORE", "Renderer initialized successfully");
    LOG_INFO0("CORE", "Press ESC to exit");

    FpsCounter fps;

    //Title Updater: Write FPS to window title
    GLFWwindow* wnd = renderer.getWindow();
    if (wnd) {
        fps.set_title_updater([wnd](const char* title) {
            glfwSetWindowTitle(wnd, title);
            });
    }

    // Make smooth FPS more responsive (adjustable from 0 to 1, the larger the value, the responsiveer it is)
    fps.set_smoothing_alpha(0.25);


    // Main render loop
    while (!glfwWindowShouldClose(renderer.getWindow())) {

        // Statistics start
        fps.begin_frame();

        // Frame Start
        eng::debug::PerfViewer::begin_frame();
        
        {
            DBG_SCOPE_SYS("Graphics", eng::debug::Subsystem::Graphics);

            // Render frame
            renderer.render();
        }

        // End of frame (will print "Perf %: Graphics 30% | ..." once per second)
        eng::debug::PerfViewer::end_frame();

        // End of statistics
        fps.end_frame();

        // Press F2 to export CSV (recent frames)
        if (glfwGetKey(renderer.getWindow(), GLFW_KEY_F2) == GLFW_PRESS) {
            eng::debug::PerfViewer::export_csv("perf_recent.csv");
        }

        // Press Ctrl+Shift+C to force a crash (test CrashLogger)
        if ((glfwGetKey(renderer.getWindow(), GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
            glfwGetKey(renderer.getWindow(), GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS) &&
            (glfwGetKey(renderer.getWindow(), GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
                glfwGetKey(renderer.getWindow(), GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS) &&
            glfwGetKey(renderer.getWindow(), GLFW_KEY_C) == GLFW_PRESS) {
            eng::debug::Log::write(eng::debug::LogLevel::Warn, "CRASH", "", 0,
                "Crash test requested (Ctrl+Shift+C).");
            eng::debug::CrashLogger::force_crash_for_test();
        }

        // Check for ESC key to exit
        if (glfwGetKey(renderer.getWindow(), GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(renderer.getWindow(), GLFW_TRUE);
        }
    }

    // Cleanup
    renderer.cleanup();

    LOG_INFO0("CORE", "Application closed successfully");
    Log::shutdown();


    return 0;
}
#endif