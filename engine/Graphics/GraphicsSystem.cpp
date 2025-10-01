#include "Precompiled.h"

namespace Framework
{
    GraphicsSystem::GraphicsSystem()
        : window(nullptr), shader(nullptr), triangleMesh(nullptr),
        currentMeshIndex(0),
        interpolateColor(true),    // 🔹 enable color animation by default
        colorLerpTime(0.0f),
        colorLerpSpeed(1.0f),
        entityManager(nullptr)
    {
    }

    GraphicsSystem::~GraphicsSystem()
    {
        std::cout << "GraphicsSystem: Cleaning up...\n";
        for (auto mesh : meshes) {
            if (mesh) delete mesh;
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

        glfwMakeContextCurrent(window);

        glewExperimental = GL_TRUE;
        if (glewInit() != GLEW_OK) {
            std::cerr << "GLEW Initialization failed!" << std::endl;
        }

        std::cout << "\n\n==============================================\n";
        std::cout << "        PRINTING OPENGL INFORMATION\n";
        std::cout << "==============================================\n\n";
        std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << "\n";
        std::cout << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << "\n";
        std::cout << "Vendor: " << glGetString(GL_VENDOR) << "\n";
        std::cout << "Renderer: " << glGetString(GL_RENDERER) << "\n";

        // Viewport
        glViewport(0, 0, 1600, 800);

        try {
            shader = new Shader("shaders/basic.vert", "shaders/basic.frag");
            std::cout << "Shaders loaded successfully\n";
        }
        catch (const std::exception& e) {
            std::cerr << "Failed to load shaders: " << e.what() << "\n";
            return;
        }

        // Create meshes
        meshes.push_back(CreateTriangle());
        meshes.push_back(CreateQuad());
        meshes.push_back(CreateLine());
        meshes.push_back(CreateCircle(40, 0.5f));

        // Initial static colors (used if interpolateColor = false)
        meshColors.push_back(glm::vec3(1.0f, 0.0f, 0.0f)); // Red
        meshColors.push_back(glm::vec3(0.0f, 1.0f, 0.0f)); // Green
        meshColors.push_back(glm::vec3(0.0f, 0.0f, 1.0f)); // Blue
        meshColors.push_back(glm::vec3(1.0f, 1.0f, 0.0f)); // Yellow

        currentMeshIndex = 0;

        std::cout << "Meshes and colors initialized successfully\n";
    }

    void GraphicsSystem::Update(float dt)
    {
        if (!window) return;
        if (glfwWindowShouldClose(window)) return;

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
                //std::cout << "Drawing at: " << transform.position.x << ", " << transform.position.y << "\n";
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
        glClearColor(0.2f, 0.3f, 0.4f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void GraphicsSystem::EndFrame()
    {
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    void GraphicsSystem::ProcessInput()
    {
        static bool enterPressedLast = false;
        static bool spacePressedLast = false;

        // --- handle ENTER: cycle through meshes ---
        bool enterNow = glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS;
        if (enterNow && !enterPressedLast) {
            if (!meshes.empty()) {
                currentMeshIndex = (currentMeshIndex + 1) % (int)meshes.size();
                std::cout << "Switched to mesh index: " << currentMeshIndex << "\n";
            }
        }
        enterPressedLast = enterNow;

        // --- handle SPACE: toggle rainbow/static color ---
        bool spaceNow = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
        if (spaceNow && !spacePressedLast) {
            interpolateColor = !interpolateColor;
            std::cout << "Space pressed → interpolateColor = " << interpolateColor << "\n";
        }
        spacePressedLast = spaceNow;
    }

    void GraphicsSystem::SetCurrentMeshColor()
    {
        if (!shader || currentMeshIndex < 0 || currentMeshIndex >= (int)meshColors.size())
            return;

        GLuint shaderID = shader->GetID();
        GLint colorLoc = glGetUniformLocation(shaderID, "uColor");

        if (interpolateColor) {
            // 🔹 rainbow animation
            float t = glfwGetTime();
            glm::vec3 rainbow = glm::vec3(
                (sin(t * 1.0f) * 0.5f) + 0.5f,
                (sin(t * 1.3f) * 0.5f) + 0.5f,
                (sin(t * 1.7f) * 0.5f) + 0.5f
            );
            glUniform3f(colorLoc, rainbow.r, rainbow.g, rainbow.b);
        }
        else {
            // 🔹 fallback: use the base mesh color
            glm::vec3 baseColor = meshColors[currentMeshIndex];
            glUniform3f(colorLoc, baseColor.r, baseColor.g, baseColor.b);
        }
    }
}
