#include "Precompiled.h"
#include "GraphicsSystem.h"
#include "Message.h"
#include "GL/glew.h"
#include "GL/gl.h"
#include <GLFW/glfw3.h>
#include "ECSComponent.h"
#include "ECSEntity.h"
#include "ECSEntityManager.h"
#include "Component.h"
#include "Shader.h"
#include "Mesh.h"
#include "MeshFactory.h"


namespace Framework
{
    GraphicsSystem::GraphicsSystem()
        : window(nullptr), shader(nullptr), triangleMesh(nullptr), entityManager(nullptr)
    {
    }

    GraphicsSystem::~GraphicsSystem()
    {
        std::cout << "GraphicsSystem: Cleaning up...\n";
        for (auto mesh : meshes) {
            if (mesh) {
                delete mesh;
            }
        }
        delete shader;
    }

    void GraphicsSystem::Initialize()
    {
        std::cout << "GraphicsSystem: Initializing...\n";

        if (!window) {
            std::cerr << "GraphicsSystem: No window set!\n";
            return;
        }

        // Make the window's context current
        glfwMakeContextCurrent(window);

        // After OpenGL context creation
        glewExperimental = GL_TRUE; // Ensures access to modern features
        if (glewInit() != GLEW_OK) {
            std::cerr << "GLEW Initialization failed!" << std::endl;
           // return -1;
        }

        // Print OpenGL information for debugging
        std::cout << "\n\n==============================================\n";
        std::cout << "        PRINTING OPENGL INFORMATION\n";
        std::cout << "==============================================\n\n";
        std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << "\n";
        std::cout << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << "\n";
        std::cout << "Vendor: " << glGetString(GL_VENDOR) << "\n";
        std::cout << "Renderer: " << glGetString(GL_RENDERER) << "\n";

        // Viewport
        glViewport(0, 0, 1600, 800);

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
        meshes.push_back(CreateCircle(40, 0.5f));

        meshColors.push_back(glm::vec3(1.0f, 0.0f, 0.0f)); // Red
        meshColors.push_back(glm::vec3(0.0f, 1.0f, 0.0f)); // Green
        meshColors.push_back(glm::vec3(0.0f, 0.0f, 1.0f)); // Blue
        meshColors.push_back(glm::vec3(1.0f, 1.0f, 0.0f)); // Yellow

        currentMeshIndex = 0;  // start with first mesh

        std::cout << "Multiple meshes created successfully\n";

        // Just draw the first mesh on init
        shader->Bind();
        if (!meshes.empty()) {
            meshes[currentMeshIndex]->Draw();
        }
    }

    void GraphicsSystem::Update(float dt)
    {
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
        RenderEntities();
        SetCurrentMeshColor();

        //if (currentMeshIndex >= 0 && currentMeshIndex < (int)meshes.size()) {
        //    if (meshes[currentMeshIndex])
        //        meshes[currentMeshIndex]->Draw();
        //}

        // Check for OpenGL errors
        GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
            std::cerr << "OpenGL error in Update: " << error << "\n";
        }

        EndFrame();

        ProcessInput();
    }

    void GraphicsSystem::RenderEntities()
    {
        if (!entityManager) return;

        for (Entity entity : entityManager->GetAllEntities())
        {
            // Only render entities with both Transform and Sprite
            if (entityManager->HasComponent<Transform>(entity) &&
                entityManager->HasComponent<Sprite>(entity))
            {
                 
                auto& transform = entityManager->GetComponent<Transform>(entity);
                std::cout << "Drawing at: " << transform.position.x << ", " << transform.position.y << "\n";
                auto& sprite = entityManager->GetComponent<Sprite>(entity);

                // Create transform matrix
                glm::mat4 model = glm::mat4(1.0f);
                model = glm::translate(model, glm::vec3(transform.position.x, transform.position.y, 0.0f));
                model = glm::rotate(model, glm::radians(transform.rotation), glm::vec3(0, 0, 1));
                model = glm::scale(model, glm::vec3(transform.scale.x, transform.scale.y, 1.0f));

                // Set uniform
                GLint modelLoc = glGetUniformLocation(shader->GetID(), "uModel");
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

                // Get mesh based on sprite name and draw
                Mesh* mesh = GetMeshForSprite(sprite.texturePath);
                if (mesh) mesh->Draw();
            }
        }
    }

    Mesh* GraphicsSystem::GetMeshForSprite(const std::string& spriteName)
    {
        // Map sprite names to your existing mesh indices
        if (spriteName == "triangle" && meshes.size() > 0)
            return meshes[0];
        if (spriteName == "quad" && meshes.size() > 1)
            return meshes[1];
        if (spriteName == "line" && meshes.size() > 2)
            return meshes[2];
        if (spriteName == "circle" && meshes.size() > 3)
            return meshes[3];

        // Default: return first mesh if available
        return meshes.empty() ? nullptr : meshes[0];
    }

    void GraphicsSystem::SendEngineMessage(Message* message)
    {
        if (message->MessageId == Status::Quit) {
            std::cout << "GraphicsSystem: Received quit message\n";
        }
    }

    void GraphicsSystem::BeginFrame()
    {
        // Use a brighter background color for debugging
        glClearColor(0.2f, 0.3f, 0.4f, 1.0f);  // Bright blue instead of dark
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void GraphicsSystem::EndFrame()
    {
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
                meshColors.clear();

                meshes.push_back(CreateTriangle());
                meshes.push_back(CreateQuad());
                meshes.push_back(CreateLine());
                meshes.push_back(CreateCircle(40, 0.5f));

                meshColors.push_back(glm::vec3(1.0f, 0.0f, 0.0f)); // Red
                meshColors.push_back(glm::vec3(0.0f, 1.0f, 0.0f)); // Green
                meshColors.push_back(glm::vec3(0.0f, 0.0f, 1.0f)); // Blue
                meshColors.push_back(glm::vec3(1.0f, 1.0f, 0.0f)); // Yellow

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

    void GraphicsSystem::SetCurrentMeshColor() {
        if (!shader || currentMeshIndex < 0 || currentMeshIndex >= (int)meshColors.size()) return;

        GLuint shaderID = shader->GetID();
        GLint colorLoc = glGetUniformLocation(shaderID, "uColor");

        glm::vec3 baseColor = meshColors[currentMeshIndex];
        glm::vec3 targetColor = glm::vec3(1.0f) - baseColor; // Invert color as a target, just for demo

        if (interpolateColor) {
            colorLerpTime += colorLerpSpeed * 0.016f; // assuming 60 FPS or pass dt
            if (colorLerpTime > 1.0f) colorLerpTime = 0.0f;

            glm::vec3 result = glm::mix(baseColor, targetColor, colorLerpTime);
            glUniform3f(colorLoc, result.r, result.g, result.b);
        }
        else {
            glUniform3f(colorLoc, baseColor.r, baseColor.g, baseColor.b);
        }
    }
}