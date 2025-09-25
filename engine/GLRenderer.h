#ifndef GLRENDERER_H
#define GLRENDERER_H

#include "Platform.h"
#include "Graphics/Shader.h"

class GLRenderer {
public:
    GLRenderer();
    ~GLRenderer();

    bool initialize();
    void render();
    void cleanup();

#ifdef PLATFORM_WINDOWS
    // Windows-specific methods
    bool initializeGLFW();
    void setWindow(GLFWwindow* window);
    GLFWwindow* getWindow() const { return window; }
#endif

private:
    bool createShaders();
    bool createBuffers();

    // OpenGL variables
    //GLuint program;
    //GLuint vertexShader;
    //GLuint fragmentShader;
    Shader shader;
    GLuint vbo;
    GLuint vao;

#ifdef PLATFORM_WINDOWS
    GLFWwindow* window;
#endif

    // Shader source code - compatible with both OpenGL ES 2.0 and 3.0
    const char* vertexShaderSource = R"(#version 300 es
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
out vec3 fragColor;
void main() {
    gl_Position = vec4(position, 1.0);
    fragColor = color;
})";

    const char* fragmentShaderSource = R"(#version 300 es
precision mediump float;
in vec3 fragColor;
out vec4 outColor;
void main() {
    outColor = vec4(fragColor, 1.0);
})";

    // Fallback shaders for OpenGL ES 2.0
    const char* vertexShaderSource2 = R"(attribute vec3 position;
attribute vec3 color;
varying vec3 fragColor;
void main() {
    gl_Position = vec4(position, 1.0);
    fragColor = color;
})";

    const char* fragmentShaderSource2 = R"(precision mediump float;
varying vec3 fragColor;
void main() {
    gl_FragColor = vec4(fragColor, 1.0);
})";
};

#endif // GLRENDERER_H 