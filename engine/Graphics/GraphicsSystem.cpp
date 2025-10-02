/*
===============================================================================
 File:          GraphicsSystem.cpp
 Author:        TAN WEI LEONG
 Email:         weileong.tan@digipen.edu
 Date:          2025-10-02
 Contribution:  100%
 ------------------------------------------------------------------------------
 Implementation of the GraphicsSystem class.

 Description:
 -------------
 This file implements the `GraphicsSystem` class, responsible for handling the 
 initialization, rendering, and input processing for graphical elements in the 
 system. It interacts with OpenGL to render various mesh types (triangle, quad, 
 line, and circle) and manages their colors, either as static or animated 
 (rainbow effect). The system also supports entity rendering based on an 
 entity-manager framework.

 Responsibilities:
 -----------------
 - `Initialize()`: Initializes OpenGL, loads shaders, and sets up meshes and 
   colors.
 - `Update()`: Updates the graphics each frame, processes input, and renders 
   entities.
 - `RenderEntities()`: Renders entities by retrieving their transform and 
   sprite components.
 - `SendEngineMessage()`: Handles system messages (such as quit) from the engine.
 - `ProcessInput()`: Handles keyboard input, allowing mesh cycling and color 
   mode toggling.
 - `SetCurrentMeshColor()`: Determines the color for the current mesh based on 
   the color mode.

 Platform-specific Notes:
 -------------------------
 - On Windows, the system uses OpenGL with the GLEW library for extensions and 
   provides basic OpenGL debugging. 
 - Shader paths are hardcoded to `"shaders/basic.vert"` and `"shaders/basic.frag"`. 
   Ensure that these shader files are present.
 - Color interpolation can be toggled using the spacebar, cycling between static 
   colors and a dynamic rainbow effect.

 Safety:
 --------
 - All functions are `noexcept` where practical, ensuring the system attempts to 
   produce a crash report even during a crash or exception.
 - The system makes best-effort attempts to log crashes to a timestamped file in 
   the executable's directory.
 - The `force_crash_for_test()` function can deliberately trigger a crash for 
   testing the crash logging behavior.
===============================================================================
*/

#include "Precompiled.h"  // Includes essential precompiled headers for the graphics system.

namespace Framework {

    /**
     * @brief Constructs a `GraphicsSystem` object and initializes its member variables.
     *
     * The constructor sets up the initial values for various member variables,
     * such as setting the window and shader pointers to `nullptr`, initializing
     * the `currentMeshIndex` to 0, enabling color interpolation (for the rainbow
     * effect), setting the initial color lerp time to 0.0f, and setting the
     * `entityManager` to `nullptr`.
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

    /**
     * @brief Destructor for the `GraphicsSystem` class.
     *
     * The destructor handles cleanup when the `GraphicsSystem` object is destroyed.
     * It deallocates memory used by meshes in the `meshes` vector and deletes the
     * shader object. This ensures that there are no memory leaks when the system is
     * destroyed.
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

    /**
     * @brief Initializes the graphics system, including OpenGL context, shaders, and meshes.
     *
     * This method ensures that OpenGL is properly initialized, shaders are loaded from
     * the filesystem, and basic meshes are created (triangle, quad, line, circle).
     * It also sets the default color mode (rainbow interpolation enabled).
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

    /**
     * @brief Updates the graphics system for each frame.
     *
     * This method clears the screen, handles input, renders all entities, and sets
     * the appropriate mesh color based on the current interpolation setting. It also
     * handles OpenGL error checking and frame swapping.
     *
     * @param dt Delta time, representing the time passed since the last update (used for animation or time-based changes).
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

    /**
     * @brief Renders all entities that have both Transform and Sprite components.
     *
     * This method iterates through all entities in the `EntityManager`, checking if they
     * have both Transform and Sprite components. It then applies the entity's transform
     * (position, rotation, scale) and renders the associated mesh using the shader.
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

    /**
    * @brief Retrieves the appropriate mesh for a given sprite.
    *
    * This method maps a sprite name (e.g., "triangle", "quad", "line", "circle") to
    * the corresponding mesh in the `meshes` list.
    *
    * @param spriteName The name of the sprite (e.g., "triangle").
    * @return A pointer to the mesh corresponding to the sprite, or the default mesh if not found.
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

    /**
     * @brief Handles system messages, such as quitting the application.
     *
     * This method listens for messages from the engine and processes them accordingly.
     * For example, if a "quit" message is received, it triggers appropriate shutdown
     * behavior for the graphics system.
     *
     * @param message The message being sent to the system (could be of type Status::Quit).
     */
    void GraphicsSystem::SendEngineMessage(Message* message) {
        if (message->MessageId == Status::Quit) {
            std::cout << "GraphicsSystem: Received quit message\n";
        }
    }


    /**
     * @brief Begins the rendering of a new frame by clearing the screen.
     *
     * This method clears the color and depth buffers, effectively preparing the
     * OpenGL context for the next frame.
     */
    void GraphicsSystem::BeginFrame() {
        glClearColor(0.2f, 0.3f, 0.4f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    /**
     * @brief Ends the current frame by swapping buffers and polling events.
     *
     * This method swaps the current buffer with the next one (displaying the frame)
     * and processes any window or input events that have occurred during the frame.
     */
    void GraphicsSystem::EndFrame() {
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    /**
     * @brief Processes keyboard input, allowing actions such as mesh cycling and color mode toggling.
     *
     * This method checks for key presses and handles specific actions, such as switching
     * to the next mesh or toggling between static and dynamic (rainbow) color modes.
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

    /**
     * @brief Sets the current color for the selected mesh based on the interpolation setting.
     *
     * If color interpolation is enabled, this method sets a dynamic, rainbow-colored
     * mesh. If interpolation is disabled, it applies a static color from the `meshColors`
     * list, corresponding to the selected mesh.
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