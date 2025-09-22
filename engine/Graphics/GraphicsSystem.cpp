#include "Precompiled.h"
#include "GraphicsSystem.h"
#include "WindowSystem.h"
#include "Message.h"
#include "Core.h"
#include "Shader.h"
#include "Mesh.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

namespace Framework {

    GraphicsSystem::GraphicsSystem()
        : window(nullptr), shader(nullptr), triangleMesh(nullptr)
    {
    }

    GraphicsSystem::~GraphicsSystem() {
        std::cout << "GraphicsSystem: Cleaning up...\n";
        delete triangleMesh;
        delete shader;
    }

    void GraphicsSystem::Initialize() {
        std::cout << "GraphicsSystem: Initializing...\n";

        if (!window) {
            std::cerr << "GraphicsSystem: No window set!\n";
            return;
        }

        // Make the window's context current
        glfwMakeContextCurrent(window);

        // Initialize GLAD
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
            std::cerr << "GLAD init failed\n";
            return;
        }

        // Print OpenGL information for debugging
        std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << "\n";
        std::cout << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << "\n";
        std::cout << "Vendor: " << glGetString(GL_VENDOR) << "\n";
        std::cout << "Renderer: " << glGetString(GL_RENDERER) << "\n";

        glViewport(0, 0, 800, 600);

        // Load shaders with better error handling
        try {
            shader = new Shader("shaders/basic.vert", "shaders/basic.frag");
            std::cout << "Shaders loaded successfully\n";
        }
        catch (const std::exception& e) {
            std::cerr << "Failed to load shaders: " << e.what() << "\n";
            return;
        }

        // Create triangle mesh with larger, more visible triangle
        std::vector<float> vertices = {
            // Larger triangle that should be clearly visible
             0.0f,  0.8f, 0.0f,  // top (larger Y value)
            -0.8f, -0.8f, 0.0f,  // bottom left  
             0.8f, -0.8f, 0.0f   // bottom right
        };

        triangleMesh = new Mesh(vertices);
        std::cout << "Triangle mesh created successfully\n";

        // Test immediate rendering to verify everything works
        BeginFrame();
        shader->Bind();
        triangleMesh->Draw();
        EndFrame();

        std::cout << "GraphicsSystem: Initialized successfully - test frame rendered\n";
    }

    void GraphicsSystem::Update(float dt) {
        if (!window) {
            std::cerr << "GraphicsSystem: No window in Update!\n";
            return;
        }

        if (glfwWindowShouldClose(window)) {
            return;
        }

        BeginFrame();

        if (!shader) {
            std::cerr << "GraphicsSystem: No shader!\n";
            return;
        }

        if (!triangleMesh) {
            std::cerr << "GraphicsSystem: No triangle mesh!\n";
            return;
        }

        shader->Bind();

        // Animate the triangle with simpler, more visible movement
        static float time = 0.0f;
        time += dt;

        // Create new vertices with animation
        std::vector<float> vertices = {
            0.0f, 0.8f + sinf(time) * 0.2f, 0.0f,  // animated top vertex
            -0.8f, -0.8f, 0.0f,
            0.8f, -0.8f, 0.0f
        };

        triangleMesh->UpdateVertices(vertices);
        triangleMesh->Draw();

        // Check for OpenGL errors
        GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
            std::cerr << "OpenGL error in Update: " << error << "\n";
        }

        EndFrame();
    }

    void GraphicsSystem::SendMessage(Message* message) {
        if (message->MessageId == Status::Quit) {
            std::cout << "GraphicsSystem: Received quit message\n";
        }
    }

    void GraphicsSystem::BeginFrame() {
        // Use a brighter background color for debugging
        glClearColor(0.2f, 0.3f, 0.4f, 1.0f);  // Bright blue instead of dark
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void GraphicsSystem::EndFrame() {
        glfwSwapBuffers(window);
        glfwPollEvents();  // Important: poll events here too
    }
}