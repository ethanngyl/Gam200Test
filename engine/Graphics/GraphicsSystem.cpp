#include "Precompiled.h"
#include "Texture.h"
/*
===============================================================================
File:        GraphicsSystem.cpp
Author:      Sim Kah Yan
Email:       kahyan.sim@digipen.edu
Date:        2025-10-02
Contribution: 80%(Kah Yan)
-------------------------------------------------------------------------------
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
namespace Framework
{
    /*
    -------------------------------------------------------------------------------
    Constructor: Initializes member variables to default values.
    -------------------------------------------------------------------------------
    */
    GraphicsSystem::GraphicsSystem()
        : window(nullptr), shader(nullptr), triangleMesh(nullptr),
        currentMeshIndex(0),
        interpolateColor(true),
        colorLerpTime(0.0f),
        colorLerpSpeed(1.0f),
        entityManager(nullptr),
        backgroundTexture(nullptr),
        backgroundQuad(nullptr)
    {
    }
    /*
    -------------------------------------------------------------------------------
    Destructor: Cleans up dynamically allocated shader, meshes, and textures.
    -------------------------------------------------------------------------------
    */
    GraphicsSystem::~GraphicsSystem()
    {
        std::cout << "GraphicsSystem: Cleaning up...\n";

        // Free all meshes stored in the vector
        for (auto mesh : meshes) {
            if (mesh) delete mesh;
        }

        // Delete the compiled shader program
        delete shader;

        // Delete the background texture object
        delete backgroundTexture;

        // Delete the background quad mesh
        delete backgroundQuad;
    }

    /*
    -------------------------------------------------------------------------------
    Initialize:
    Sets up OpenGL context, loads shaders, creates meshes, and loads the
    background texture. Must be called after setting the GLFW window.
    -------------------------------------------------------------------------------
    */
    void GraphicsSystem::Initialize()
    {
        std::cout << "GraphicsSystem: Initializing...\n";

        // Check that the GLFW window is set before doing any GL calls
        if (!window) {
            std::cerr << "GraphicsSystem: No window set!\n";
            return;
        }

        // Make the given GLFW window's context current so OpenGL calls affect it
        glfwMakeContextCurrent(window);

        // Enable experimental features so GLEW can load modern OpenGL extensions
        glewExperimental = GL_TRUE;
        if (glewInit() != GLEW_OK) {
            std::cerr << "GLEW Initialization failed!" << std::endl;
        }

        // Print OpenGL and GPU information to help with debugging
        std::cout << "\n\n==============================================\n";
        std::cout << "        PRINTING OPENGL INFORMATION\n";
        std::cout << "==============================================\n\n";
        std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << "\n";
        std::cout << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << "\n";
        std::cout << "Vendor: " << glGetString(GL_VENDOR) << "\n";
        std::cout << "Renderer: " << glGetString(GL_RENDERER) << "\n";

        // Set viewport to match fixed resolution (1600x800)
        glViewport(0, 0, 1600, 800);

        // Load and compile the GLSL shaders (vertex + fragment)
        try {
            shader = new Shader("shaders/basic.vert", "shaders/basic.frag");
            std::cout << "Shaders loaded successfully\n";
        }
        catch (const std::exception& e) {
            std::cerr << "Failed to load shaders: " << e.what() << "\n";
            return;
        }

        // Load the background texture to render behind everything else
        backgroundTexture = new Texture();
        if (!backgroundTexture->LoadFromFile("./assets/background.jpg")) {
            std::cerr << "Failed to load background image\n";
        }
        else {
            std::cout << "Background texture loaded successfully\n";
        }
        // Create a fullscreen quad mesh to display the background texture
        backgroundQuad = CreateQuad();

        // Create basic mesh primitives used in rendering
        meshes.push_back(CreateTriangle());
        meshes.push_back(CreateQuad());
        meshes.push_back(CreateLine());
        meshes.push_back(CreateCircle(40, 0.5f));

        // Assign default colors to meshes (used if rainbow mode is off)
        meshColors.push_back(glm::vec3(1.0f, 0.0f, 0.0f)); // Red
        meshColors.push_back(glm::vec3(0.0f, 1.0f, 0.0f)); // Green
        meshColors.push_back(glm::vec3(0.0f, 0.0f, 1.0f)); // Blue
        meshColors.push_back(glm::vec3(1.0f, 1.0f, 0.0f)); // Yellow

        // Start with the first mesh selected (triangle)
        currentMeshIndex = 0;

        std::cout << "Meshes and colors initialized successfully\n";
    }

    /*
    ------------------------------------------------------------------------------
    Update: Called once per frame. Clears buffers, sets up projection,
            draws background, renders entities, and processes input.
    ------------------------------------------------------------------------------
    */
    void GraphicsSystem::Update(float dt)
    {
        // If the window doesn't exist or is closing, skip rendering
        if (!window) return;
        if (glfwWindowShouldClose(window)) return;

        // Clear the screen and depth buffer to prepare for new frame
        BeginFrame();

        // Make sure the shader is available before proceeding
        if (!shader) {
            std::cerr << "GraphicsSystem: No shader!\n";
            return;
        }
        // Set up orthographic projection for 2D rendering
        float aspectRatio = 1600.0f / 800.0f;  // Fixed aspect ratio (2.0)
        glm::mat4 projection = glm::ortho(-aspectRatio, aspectRatio, -1.0f, 1.0f, -1.0f, 1.0f);

        // Bind the shader program and upload projection matrix
        shader->Bind();
        GLint projLoc = glGetUniformLocation(shader->GetID(), "uProjection");
        
        if (projLoc == -1) {
            std::cerr << "WARNING: uProjection uniform not found in shader!\n";
        }
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

        // Draw background first (fullscreen textured quad)
        if (backgroundTexture && backgroundQuad) {
            // Create transform matrix
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::scale(model, glm::vec3(2.0f, 2.0f, 2.0f));

            // Set uniform
            GLint modelLoc = glGetUniformLocation(shader->GetID(), "uModel");
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

            glUniform1i(glGetUniformLocation(shader->GetID(), "uUseTexture"), 1);
            glUniform1i(glGetUniformLocation(shader->GetID(), "uTexture"), 0);

            backgroundTexture->Bind(0);
            backgroundQuad->Draw();
            backgroundTexture->Unbind();
        }

        // Draw all entities (non-textured mesh rendering)
        glUniform1i(glGetUniformLocation(shader->GetID(), "uUseTexture"), 0);

        SetCurrentMeshColor();
        RenderEntities();

        // Check OpenGL error state
        GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
            std::cerr << "OpenGL error in Update: " << error << "\n";
        }

        EndFrame();
        ProcessInput();
    }

    /*
    ------------------------------------------------------------------------------
    RenderEntities: Renders all entities that have both Transform and Sprite
                    components. Calculates transform and draws the corresponding mesh.
    ------------------------------------------------------------------------------
    */
    void GraphicsSystem::RenderEntities()
    {
        if (!entityManager) return;

        // Loop through all entities registered in the EntityManager
        for (Entity entity : entityManager->GetAllEntities())
        {
            // Only render entities that have Transform + Sprite components
            if (entityManager->HasComponent<Transform>(entity) &&
                entityManager->HasComponent<Sprite>(entity))
            {

                // Get the entity's transform and sprite data
                auto& transform = entityManager->GetComponent<Transform>(entity);
                auto& sprite = entityManager->GetComponent<Sprite>(entity);

                // Construct model matrix (translate → rotate → scale)
                glm::mat4 model = glm::mat4(1.0f);
                model = glm::translate(model, glm::vec3(transform.position.x, transform.position.y, 0.0f));
                model = glm::rotate(model, glm::radians(transform.rotation), glm::vec3(0, 0, 1));
                model = glm::scale(model, glm::vec3(transform.scale.x, transform.scale.y, 1.0f));

                // Upload model matrix to shader
                GLint modelLoc = glGetUniformLocation(shader->GetID(), "uModel");
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

                // Find appropriate mesh based on sprite name and draw it
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

        // Return fallback mesh if name doesn't match
        return meshes.empty() ? nullptr : meshes[0];
    }

    /*
    ------------------------------------------------------------------------------
    SendEngineMessage: Handles engine-wide messages such as Quit.
    ------------------------------------------------------------------------------
    */
    void GraphicsSystem::SendEngineMessage(Message* message)
    {
        // Check for quit message and print notification
        if (message->MessageId == Status::Quit) {
            std::cout << "GraphicsSystem: Received quit message\n";
        }
    }

    /*
    ------------------------------------------------------------------------------
    BeginFrame / EndFrame: Handles clearing and buffer swapping each frame.
    ------------------------------------------------------------------------------
    */
    void GraphicsSystem::BeginFrame()
    {
        // Clear screen to bluish background color
        glClearColor(0.2f, 0.3f, 0.4f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void GraphicsSystem::EndFrame()
    {
        // Swap front and back buffers to present rendered image
        glfwSwapBuffers(window);
        // Poll events (keyboard, mouse, etc.)
        glfwPollEvents();
    }

    /*
    ------------------------------------------------------------------------------
    ProcessInput: Handles keyboard input for mesh switching (ENTER) and toggling
                  rainbow/static color mode (SPACE).
    ------------------------------------------------------------------------------
    */
    void GraphicsSystem::ProcessInput()
    {
        static bool enterPressedLast = false;
        static bool spacePressedLast = false;

        // Handle ENTER key: cycle through mesh list
        bool enterNow = glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS;
        if (enterNow && !enterPressedLast) {
            if (!meshes.empty()) {
                currentMeshIndex = (currentMeshIndex + 1) % (int)meshes.size();
                std::cout << "Switched to mesh index: " << currentMeshIndex << "\n";
            }
        }
        enterPressedLast = enterNow;

        // Handle SPACE key: toggle rainbow/static color mode
        bool spaceNow = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
        if (spaceNow && !spacePressedLast) {
            interpolateColor = !interpolateColor;
            std::cout << "Space pressed → interpolateColor = " << interpolateColor << "\n";
        }
        spacePressedLast = spaceNow;
    }

    /*
    ------------------------------------------------------------------------------
    SetCurrentMeshColor: Updates shader uniform uColor to use either rainbow
                         animated color or static mesh color.
    ------------------------------------------------------------------------------
    */
    void GraphicsSystem::SetCurrentMeshColor()
    {
        // Validate mesh index and shader before using
        if (!shader || currentMeshIndex < 0 || currentMeshIndex >= (int)meshColors.size())
            return;

        GLuint shaderID = shader->GetID();
        GLint colorLoc = glGetUniformLocation(shaderID, "uColor");

        if (interpolateColor) {
            // Generate rainbow color based on elapsed time
            float t = glfwGetTime();
            glm::vec3 rainbow = glm::vec3(
                (sin(t * 1.0f) * 0.5f) + 0.5f,
                (sin(t * 1.3f) * 0.5f) + 0.5f,
                (sin(t * 1.7f) * 0.5f) + 0.5f
            );

            // Send rainbow color to shader
            glUniform3f(colorLoc, rainbow.r, rainbow.g, rainbow.b);
        }
        else {
            // Fallback: use the base mesh color
            glm::vec3 baseColor = meshColors[currentMeshIndex];
            glUniform3f(colorLoc, baseColor.r, baseColor.g, baseColor.b);
        }
    }
}
