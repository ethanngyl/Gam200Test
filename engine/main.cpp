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

int main() {
    using namespace eng::debug;

    // Initializing Logs
    LogConfig logCfg;
    logCfg.level = LogLevel::Info;       
    logCfg.filePath = "engine.log";      // Output Files
    logCfg.showSourceInfo = false;       // Off (file:line)
    logCfg.useConsole = true;            // Console(stderr)
    logCfg.useFile = true;               
    logCfg.usePlatformOutput = true;     // VS Output
    Log::Init(logCfg);

    LOG_INFO0("CORE", "Starting OpenGL ES 3.0 Triangle Demo on Windows");

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
        fps.SetTitleUpdater([wnd](const char* title) {
            glfwSetWindowTitle(wnd, title);
            });
    }

    // Make smooth FPS more responsive (adjustable from 0 to 1, the larger the value, the responsiveer it is)
    fps.SetSmoothingAlpha(0.25);


    // Main render loop
    while (!glfwWindowShouldClose(renderer.getWindow())) {

        // Statistics start
        fps.BeginFrame();

        // Frame Start
        eng::debug::PerfViewer::BeginFrame();
        
        {
            DBG_SCOPE_SYS("Graphics", eng::debug::Subsystem::Graphics);

            // Render frame
            renderer.render();
        }

        // End of frame (will print "Perf %: Graphics 30% | ..." once per second)
        eng::debug::PerfViewer::EndFrame();

        // End of statistics
        fps.EndFrame();       

        // Press F2 to export CSV (recent frames)
        if (glfwGetKey(renderer.getWindow(), GLFW_KEY_F2) == GLFW_PRESS) {
            eng::debug::PerfViewer::ExportCSV("perf_recent.csv");
        }

        // Check for ESC key to exit
        if (glfwGetKey(renderer.getWindow(), GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(renderer.getWindow(), GLFW_TRUE);
        }
    }

    // Cleanup
    renderer.cleanup();

    LOG_INFO0("CORE", "Application closed successfully");
    Log::Shutdown();


    return 0;
}
#endif