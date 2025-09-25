#include "GLRenderer.h"
#include <cstring>
#include "Graphics/Shader.h"

#define LOG_TAG "GLRenderer"

GLRenderer::GLRenderer() : vbo(0), vao(0)
{
    LOGI("GLRenderer constructor called");
}

GLRenderer::~GLRenderer() {
    LOGI("GLRenderer destructor called");
    cleanup();
}

bool GLRenderer::initialize() {
    LOGI("Initializing OpenGL renderer");

    // Load shaders from external files
    try {
        shader = Shader("shaders/vertex_shader.vert", "shaders/fragment_shader.frag");
    }
    catch (std::exception& e) {
        LOGE("Failed to load shaders: %s", e.what());
        return false;
    }

    if (!createBuffers()) {
        LOGE("Failed to create buffers");
        return false;
    }

    LOGI("GLRenderer initialized successfully");
    return true;
}

bool GLRenderer::createBuffers() {
    LOGI("Creating buffers");

    // Triangle vertices (position + color)
    float vertices[] = {
        // Position        // Color
         0.0f,  0.5f, 0.0f,   1.0f, 0.0f, 0.0f, // Top (red)
        -0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f, // Left (green)
         0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f  // Right (blue)
    };

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    std::cout << "[GLRenderer] Buffers created, vao=" << vao << " vbo=" << vbo << std::endl;

    // Position attribute (location = 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Color attribute (location = 1)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    LOGI("Buffers created successfully");
    return true;
}

void GLRenderer::render() {
    std::cout << "[GLRenderer] Rendering frame, vao=" << vao << " vbo=" << vbo << std::endl;
    // Clear the screen
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    shader.use();
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
}

void GLRenderer::cleanup() {
    LOGI("Cleaning up OpenGL resources");

    if (vbo != 0) {
        glDeleteBuffers(1, &vbo);
        vbo = 0;
    }
    if (vao != 0) {
        glDeleteVertexArrays(1, &vao);
        vao = 0;
    }
}
