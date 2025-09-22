#include "Precompiled.h"
#include "GraphicsSystem.h"
#include "WindowSystem.h"
#include "Message.h"
#include "Core.h"
#include "Shader.h"
#include "Mesh.h"
#include "MeshFactory.h"
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
        for (auto mesh : meshes) {
            if (mesh) {
                delete mesh;
            }
        }
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

        // Create multiple meshes
        meshes.push_back(CreateTriangle());
        meshes.push_back(CreateQuad());
        meshes.push_back(CreateLine());

        currentMeshIndex = 0;  // start with first mesh

        std::cout << "Multiple meshes created successfully\n";

        // Just draw the first mesh on init
        BeginFrame();
        shader->Bind();
        if (!meshes.empty()) {
            meshes[currentMeshIndex]->Draw();
        }
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

        // Rendering
        BeginFrame();

        if (!shader) {
            std::cerr << "GraphicsSystem: No shader!\n";
            return;
        }

        shader->Bind();

        if (currentMeshIndex >= 0 && currentMeshIndex < (int)meshes.size()) {
            if (meshes[currentMeshIndex])
                meshes[currentMeshIndex]->Draw();
        }

        // Check for OpenGL errors
        GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
            std::cerr << "OpenGL error in Update: " << error << "\n";
        }

        EndFrame();

        ProcessInput();
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

    void GraphicsSystem::ProcessInput() {
        static bool dPressedLastFrame = false;
        bool dPressedNow = glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS;

        if (dPressedNow && !dPressedLastFrame) {
            std::cout << "Enter press! Next mesh displayed.\n";

            if (meshes.empty()) return;

            // Delete current mesh if valid
            if (currentMeshIndex >= 0 && currentMeshIndex < (int)meshes.size()) {
                if (meshes[currentMeshIndex]) {
                    delete meshes[currentMeshIndex];
                    meshes[currentMeshIndex] = nullptr;
                }
            }

            // Check if all meshes are deleted
            bool allDeleted = true;
            for (auto m : meshes) {
                if (m != nullptr) {
                    allDeleted = false;
                    break;
                }
            }

            if (allDeleted) {
                // Recreate all meshes since all are deleted (reset)
                meshes.clear();
                meshes.push_back(CreateTriangle());
                meshes.push_back(CreateQuad());
                meshes.push_back(CreateLine());

                currentMeshIndex = 0;
            }
            else {
                // Move to next valid mesh (skip deleted ones)
                int nextIndex = currentMeshIndex;
                do {
                    nextIndex = (nextIndex + 1) % (int)meshes.size();
                } while (meshes[nextIndex] == nullptr);

                currentMeshIndex = nextIndex;
            }
        }

        dPressedLastFrame = dPressedNow;
    }
}