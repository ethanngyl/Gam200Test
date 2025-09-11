#include "Platform.h"
// [IMPORTANT] This main entry file is only for Desktop Based applications support Window, Ubuntu, Mac etc
#ifdef PLATFORM_WINDOWS

#include "GLRenderer.h"
#include <iostream>

int main() {
    std::cout << "Starting OpenGL ES 3.0 Triangle Demo on Windows" << std::endl;

    // Create renderer
    GLRenderer renderer;

    // Initialize
    if (!renderer.initialize()) {
        std::cerr << "Failed to initialize renderer" << std::endl;
        return -1;
    }

    std::cout << "Renderer initialized successfully" << std::endl;
    std::cout << "Press ESC to exit" << std::endl;

    // Main render loop
    while (!glfwWindowShouldClose(renderer.getWindow())) {
        // Render frame
        renderer.render();

        // Check for ESC key to exit
        if (glfwGetKey(renderer.getWindow(), GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(renderer.getWindow(), GLFW_TRUE);
        }
    }

    // Cleanup
    renderer.cleanup();

    std::cout << "Application closed successfully" << std::endl;
    return 0;
}
#endif