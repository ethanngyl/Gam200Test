/*
===============================================================================
 File:          GraphicsSystem.cpp
 Author:        Sim Kah Yan, TAN WEI LEONG
 Email:         kahyan.sim@digipen.edu, weileong.tan@digipen.edu
 Date:          2025-10-02
 Contribution:  80%(Kah Yan), 20%(TAN WEI LEONG)
 ------------------------------------------------------------------------------
 Implementation of the GraphicsSystem class.

 Brief:
 Implementation of the GraphicsSystem class, which handles rendering,
 OpenGL context setup, shader loading, mesh creation, background rendering,
 and per-frame drawing of entities within the engine framework.

 Details:
 - Initializes the OpenGL rendering context and prints GPU information.
 - Loads and compiles GLSL shaders.
 - Creates basic meshes (triangle, quad, line, circle).
 - Handles drawing a background textured quad.
 - Renders entities based on their Transform and Sprite components.
 - Supports mesh switching and rainbow/static color modes via keyboard input.
 - Uses an orthographic projection for 2D rendering.

 Notes:
 - Uses a fixed viewport of 1600x800 and y-up coordinate system.
 - Requires GLFW, GLEW, GLM, and custom Shader/Texture classes.
 - No dynamic allocations during rendering except for initialization time.

 Safety:
 - All pointers checked before use.
 - Proper cleanup in destructor to avoid memory leaks.
 - Graceful fallback for missing shaders, textures, or meshes.
===============================================================================
*/

#include "Precompiled.h"  // Includes essential precompiled headers for the graphics system.

namespace Framework {

    /*
    -------------------------------------------------------------------------------
    Constructor: Initializes member variables to default values.
    -------------------------------------------------------------------------------
    */
    GraphicsSystem::GraphicsSystem()
        : window(nullptr),                // Pointer to the GLFW window, initialized to nullptr.
        shader(nullptr),                  // Pointer to the shader program, initialized to nullptr.
        triangleMesh(nullptr),            // Pointer to the triangle mesh, initialized to nullptr.
        currentMeshIndex(0),              // Index of the currently selected mesh, starting from 0.
        interpolateColor(true),           // Flag to toggle between static and dynamic (rainbow) colors, enabled by default.
        colorLerpTime(0.0f),              // Time used for color interpolation, initialized to 0.0f.
        colorLerpSpeed(1.0f),             // Speed of color interpolation, set to 1.0f.
        entityManager(nullptr)            // Pointer to the EntityManager, initialized to nullptr.
    {
        // Constructor: Initialize member variables and set defaults.
    }

    /*
    -------------------------------------------------------------------------------
    Destructor: Cleans up dynamically allocated shader, meshes, and textures.
    -------------------------------------------------------------------------------
    */
    GraphicsSystem::~GraphicsSystem() {
        std::cout << "GraphicsSystem: Cleaning up...\n";

        // Deallocate memory for each mesh in the `meshes` vector.
        // If any mesh pointer is valid (not nullptr), delete it to free memory.
        for (auto mesh : meshes) {
            if (mesh) delete mesh;
        }

        // Delete the shader object to free its memory.
        delete shader;
    }

    /*
    -------------------------------------------------------------------------------
    Initialize:
    Sets up OpenGL context, loads shaders, creates meshes, and loads the
    background texture. Must be called after setting the GLFW window.
    -------------------------------------------------------------------------------
    */
    void GraphicsSystem::Initialize() {
        std::cout << "GraphicsSystem: Initializing...\n";

        // Ensure that a window is set
        if (!window) {
            std::cerr << "GraphicsSystem: No window set!\n";
            return;
        }

        // Set OpenGL context to the current window
        glfwMakeContextCurrent(window);

        // Query framebuffer size from the window
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        // Divide window into 4 equal quadrants
        AddViewport(0, height / 2, width / 2, height / 2);         // Top-left
        AddViewport(width / 2, height / 2, width / 2, height / 2); // Top-right
        AddViewport(0, 0, width / 2, height / 2);                  // Bottom-left
        AddViewport(width / 2, 0, width / 2, height / 2);          // Bottom-right

        // After OpenGL context creation
        glewExperimental = GL_TRUE; // Ensures access to modern features
        // Initialize GLEW for extension handling
        glewExperimental = GL_TRUE;
        if (glewInit() != GLEW_OK) {
            std::cerr << "GLEW Initialization failed!" << std::endl;
        }

        // Output OpenGL info (useful for debugging and version checking)
        std::cout << "\n\n==============================================\n";
        std::cout << "        PRINTING OPENGL INFORMATION\n";
        std::cout << "==============================================\n\n";
        std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << "\n";
        std::cout << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << "\n";
        std::cout << "Vendor: " << glGetString(GL_VENDOR) << "\n";
        std::cout << "Renderer: " << glGetString(GL_RENDERER) << "\n";

        // Set the viewport to match the window dimensions
        glViewport(0, 0, 1600, 800);

        try {
            // Attempt to load shaders from file paths
            shader = new Shader("shaders/basic.vert", "shaders/basic.frag");
            std::cout << "Shaders loaded successfully\n";
        }
        catch (const std::exception& e) {
            std::cerr << "Failed to load shaders: " << e.what() << "\n";
            return;
        }

        // Create and initialize meshes (triangle, quad, line, circle)
        meshes.push_back(CreateTriangle());
        meshes.push_back(CreateQuad());
        meshes.push_back(CreateLine());
        meshes.push_back(CreateCircle(40, 0.5f));

        // Static colors for meshes (used if interpolateColor is false)
        meshColors.push_back(glm::vec3(1.0f, 0.0f, 0.0f)); // Red
        meshColors.push_back(glm::vec3(0.0f, 1.0f, 0.0f)); // Green
        meshColors.push_back(glm::vec3(0.0f, 0.0f, 1.0f)); // Blue
        meshColors.push_back(glm::vec3(1.0f, 1.0f, 0.0f)); // Yellow

        currentMeshIndex = 0;

        std::cout << "Meshes and colors initialized successfully\n";
    }

    /*
    ------------------------------------------------------------------------------
    Update: Called once per frame. Clears buffers, sets up projection,
            draws background, renders entities, and processes input.
    ------------------------------------------------------------------------------
    */
    void GraphicsSystem::Update(float dt) {
        (void)dt;
        if (!window) return;  // Ensure window exists

        if (glfwWindowShouldClose(window)) return;  // Check if the window should close

        BeginFrame();  // Prepare for new frame by clearing the screen

        if (!shader) {
            std::cerr << "GraphicsSystem: No shader!\n";
            return;
        }

        // Calculate aspect ratio and create orthographic projection matrix
        float aspectRatio = 1600.0f / 800.0f;  // 2.0
        glm::mat4 projection = glm::ortho(-aspectRatio, aspectRatio, -1.0f, 1.0f, -1.0f, 1.0f);

        shader->Bind();  // Bind the shader before rendering

        // Set the projection matrix to the shader
        GLint projLoc = glGetUniformLocation(shader->GetID(), "uProjection");
        //std::cout << "Projection uniform location: " << projLoc << "\n";
        if (projLoc == -1) {
            std::cerr << "WARNING: uProjection uniform not found in shader!\n";
        }
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

        // Render all entities
        RenderEntities();

        // Set the current mesh color based on interpolation setting
        SetCurrentMeshColor();

        // Check for OpenGL errors
        GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
            std::cerr << "OpenGL error in Update: " << error << "\n";
        }

        EndFrame();      // Swap buffers and poll events
        ProcessInput();  // Handle input events
    }

    /*
    ------------------------------------------------------------------------------
    RenderEntities: Renders all entities that have both Transform and Sprite
                    components. Calculates transform and draws the corresponding mesh.
    ------------------------------------------------------------------------------
    */
    void GraphicsSystem::RenderEntities() {
        if (!entityManager) return;

        for (Entity entity : entityManager->GetAllEntities()) {
            // Only render entities with both Transform and Sprite
            if (entityManager->HasComponent<Transform>(entity) &&
                entityManager->HasComponent<Sprite>(entity)) {
                 
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

    /*
    ------------------------------------------------------------------------------
    GetMeshForSprite: Maps sprite names to preloaded meshes and returns the
                      appropriate mesh pointer.
    ------------------------------------------------------------------------------
    */
    Mesh* GraphicsSystem::GetMeshForSprite(const std::string& spriteName) {
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

    /*
    ------------------------------------------------------------------------------
    SendEngineMessage: Handles engine-wide messages such as Quit.
    ------------------------------------------------------------------------------
    */
    void GraphicsSystem::SendEngineMessage(Message* message) {
        if (message->MessageId == Status::Quit) {
            std::cout << "GraphicsSystem: Received quit message\n";
        }
    }


    /*
    ------------------------------------------------------------------------------
    BeginFrame / EndFrame: Handles clearing and buffer swapping each frame.
    ------------------------------------------------------------------------------
    */
    void GraphicsSystem::BeginFrame() {
        glClearColor(0.2f, 0.3f, 0.4f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    
    void GraphicsSystem::EndFrame() {
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    /*
    ------------------------------------------------------------------------------
    ProcessInput: Handles keyboard input for mesh switching (ENTER) and toggling
                  rainbow/static color mode (SPACE).
    ------------------------------------------------------------------------------
    */
    void GraphicsSystem::ProcessInput() {
        static bool enterPressedLast = false;
        static bool spacePressedLast = false;

        // --- Handle ENTER: Cycle through meshes ---
        bool enterNow = glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS;
        if (enterNow && !enterPressedLast) {
            if (!meshes.empty()) {
                // Switch to the next mesh in the list
                currentMeshIndex = (currentMeshIndex + 1) % (int)meshes.size();
                std::cout << "Switched to mesh index: " << currentMeshIndex << "\n";
            }
        }
        enterPressedLast = enterNow;

        // --- Handle SPACE: Toggle rainbow/static color mode ---
        bool spaceNow = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
        if (spaceNow && !spacePressedLast) {
            // Toggle color interpolation mode
            interpolateColor = !interpolateColor;
            std::cout << "Space pressed -> interpolateColor = " << interpolateColor << "\n";
        }
        spacePressedLast = spaceNow;
    }

    /*
    ------------------------------------------------------------------------------
    SetCurrentMeshColor: Updates shader uniform uColor to use either rainbow
                         animated color or static mesh color.
    ------------------------------------------------------------------------------
    */
    void GraphicsSystem::SetCurrentMeshColor() {
        if (!shader || currentMeshIndex < 0 || currentMeshIndex >= (int)meshColors.size())
            return;

        GLuint shaderID = shader->GetID();
        GLint colorLoc = glGetUniformLocation(shaderID, "uColor");

        // If interpolation is enabled, use a dynamic rainbow color
        if (interpolateColor) {
            float t = static_cast<float>(glfwGetTime());
            glm::vec3 rainbow = glm::vec3(
                (sin(t * 1.0f) * 0.5f) + 0.5f,  // Red component
                (sin(t * 1.3f) * 0.5f) + 0.5f,  // Green component
                (sin(t * 1.7f) * 0.5f) + 0.5f   // Blue component
            );
            glUniform3f(colorLoc, rainbow.r, rainbow.g, rainbow.b);
        }
        else {
            // Otherwise, use the static color for the current mesh
            glm::vec3 baseColor = meshColors[currentMeshIndex];
            glUniform3f(colorLoc, baseColor.r, baseColor.g, baseColor.b);
        }
    }

} // namespace Framework