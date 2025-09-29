/*
#include "GLRenderer.h"
#include <cstring>

// Define LOG_TAG for this file
#define LOG_TAG "GLRenderer"

GLRenderer::GLRenderer() : program(0), vertexShader(0),
fragmentShader(0), vbo(0), vao(0)
#ifdef PLATFORM_WINDOWS
, window(nullptr)
#endif
{
    LOGI("GLRenderer constructor called");
}

GLRenderer::~GLRenderer() {
    LOGI("GLRenderer destructor called");
    cleanup();
}

bool GLRenderer::initialize() {
    LOGI("Initializing OpenGL ES renderer");

#ifdef PLATFORM_WINDOWS
    if (!initializeGLFW()) {
        LOGE("Failed to initialize GLFW");
        return false;
    }
#endif

    if (!createShaders()) {
        LOGE("Failed to create shaders");
        return false;
    }

    if (!createBuffers()) {
        LOGE("Failed to create buffers");
        return false;
    }

    LOGI("OpenGL ES renderer initialized successfully");
    return true;
}

#ifdef PLATFORM_WINDOWS
bool GLRenderer::initializeGLFW() {
    LOGI("Initializing GLFW");

    // Initialize GLFW
    if (!glfwInit()) {
        LOGE("Failed to initialize GLFW");
        return false;
    }

    // Configure GLFW for OpenGL ES
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create window
    window = glfwCreateWindow(800, 600, "OpenGL ES 3.0 Triangle Demo", nullptr, nullptr);
    if (!window) {
        LOGE("Failed to create GLFW window");
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window);

    // Initialize GLEW
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        LOGE("Failed to initialize GLEW");
        return false;
    }

    LOGI("GLFW initialized successfully");
    return true;
}

void GLRenderer::setWindow(GLFWwindow* win) {
    window = win;
}
#endif

bool GLRenderer::createShaders() {
    LOGI("Creating shaders");

    // Determine OpenGL version
    const char* version = (const char*)glGetString(GL_VERSION);
    LOGI("OpenGL version: %s", version ? version : "unknown");

    bool useOpenGLES3 = false;
    if (version && strstr(version, "OpenGL ES 3")) {
        useOpenGLES3 = true;
        LOGI("Using OpenGL ES 3.0 shaders");
    }
    else {
        LOGI("Using OpenGL ES 2.0 shaders");
    }

    // Choose appropriate shader sources
    const char* vsSource = useOpenGLES3 ? vertexShaderSource : vertexShaderSource2;
    const char* fsSource = useOpenGLES3 ? fragmentShaderSource : fragmentShaderSource2;

    // Create vertex shader
    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    if (vertexShader == 0) {
        LOGE("Failed to create vertex shader");
        return false;
    }

    glShaderSource(vertexShader, 1, &vsSource, nullptr);
    glCompileShader(vertexShader);

    // Check vertex shader compilation
    GLint success;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar infoLog[512];
        glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
        LOGE("Vertex shader compilation failed: %s", infoLog);
        return false;
    }
    LOGI("Vertex shader compiled successfully");

    // Create fragment shader
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    if (fragmentShader == 0) {
        LOGE("Failed to create fragment shader");
        return false;
    }

    glShaderSource(fragmentShader, 1, &fsSource, nullptr);
    glCompileShader(fragmentShader);

    // Check fragment shader compilation
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar infoLog[512];
        glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
        LOGE("Fragment shader compilation failed: %s", infoLog);
        return false;
    }
    LOGI("Fragment shader compiled successfully");

    // Create shader program
    program = glCreateProgram();
    if (program == 0) {
        LOGE("Failed to create shader program");
        return false;
    }

    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    // Check program linking
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        GLchar infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        LOGE("Shader program linking failed: %s", infoLog);
        return false;
    }
    LOGI("Shader program linked successfully");

    return true;
}

bool GLRenderer::createBuffers() {
    LOGI("Creating buffers");

    // Triangle vertices (position + color)
    float vertices[] = {
        // Position (x, y, z)    // Color (r, g, b)
         0.0f,  0.5f, 0.0f,     1.0f, 0.0f, 0.0f,  // Top vertex (red)
        -0.5f, -0.5f, 0.0f,     0.0f, 1.0f, 0.0f,  // Bottom left (green)
         0.5f, -0.5f, 0.0f,     0.0f, 0.0f, 1.0f   // Bottom right (blue)
    };

    // Check if VAOs are supported (OpenGL ES 3.0+)
    const char* version = (const char*)glGetString(GL_VERSION);
    bool useVAO = (version && strstr(version, "OpenGL ES 3"));

    if (useVAO) {
        // Create VAO (OpenGL ES 3.0+)
        glGenVertexArrays(1, &vao);
        if (vao == 0) {
            LOGE("Failed to create VAO");
            return false;
        }
        glBindVertexArray(vao);
        LOGI("Using VAO for vertex attributes");
    }
    else {
        LOGI("VAO not supported, using direct attribute binding");
    }

    // Create VBO
    glGenBuffers(1, &vbo);
    if (vbo == 0) {
        LOGE("Failed to create VBO");
        return false;
    }
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    if (useVAO) {
        // Position attribute
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        // Color attribute
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        // Unbind
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
    else {
        // For OpenGL ES 2.0, we'll bind attributes in render function
        LOGI("Will bind attributes in render function for OpenGL ES 2.0");
    }

    LOGI("Buffers created successfully");
    return true;
}

void GLRenderer::render() {
    // Clear the screen
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Use shader program
    glUseProgram(program);

    // Check if VAOs are supported
    const char* version = (const char*)glGetString(GL_VERSION);
    bool useVAO = (version && strstr(version, "OpenGL ES 3"));

    if (useVAO) {
        // Bind VAO and draw (OpenGL ES 3.0+)
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }
    else {
        // For OpenGL ES 2.0, bind attributes manually
        glBindBuffer(GL_ARRAY_BUFFER, vbo);

        // Position attribute
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        // Color attribute
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glDrawArrays(GL_TRIANGLES, 0, 3);
    }

#ifdef PLATFORM_WINDOWS
    // Swap buffers and poll events for Windows
    if (window) {
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
#endif
}

void GLRenderer::cleanup() {
    LOGI("Cleaning up OpenGL resources");

    // Clean up OpenGL resources
    if (program != 0) {
        glDeleteProgram(program);
    }
    if (vertexShader != 0) {
        glDeleteShader(vertexShader);
    }
    if (fragmentShader != 0) {
        glDeleteShader(fragmentShader);
    }
    if (vbo != 0) {
        glDeleteBuffers(1, &vbo);
    }
    if (vao != 0) {
        glDeleteVertexArrays(1, &vao);
    }

#ifdef PLATFORM_WINDOWS
    if (window) {
        glfwDestroyWindow(window);
        window = nullptr;
    }
    glfwTerminate();
#endif

    LOGI("OpenGL resources cleaned up");
}

*/