/*
===============================================================================
File:        GraphicsSystemV2.cpp
Author:      Sim Kah Yan
Email:       kahyan.sim@digipen.edu
Date:        2026-02-04 (yyyy-mm-dd)
Contribution: 100%
-------------------------------------------------------------------------------
Brief:
Implementation of GraphicsSystemV2: modern 2D rendering pipeline managing
resource loading, camera control (main/editor, follow target), render queue
gather/sort, batched instanced draws, debug primitives, and FreeType text.

Details:
- Initializes OpenGL (GLEW), loads default shaders/materials/meshes, sets
  render state (blend, cull, depth disabled for 2D).
- Each frame: updates viewport/resolution, runs camera logic (editor or
  follow-player), gathers renderables from ECS (MeshRenderer/Sprite/SpriteAnimation),
  sorts by layer/depth, batches by mesh+material+texture, executes instanced draws.
- Background pass (quad + texture), sprite-sheet UV animation, and optional
  render-to-FBO (e.g. ImGui viewport) supported.
- Debug overlay: lines, circles, boxes via DebugRenderQueue. Text via TextRenderer.

Notes:
- SetWindow(...) must be called before Initialize().
- EntityManager and InputSystem are injected; this system does not own ECS data.
- Y-up, orthographic camera; view/projection synced with viewport via SetViewportSize().
- Editor camera: pan (arrow keys), zoom (1/2), reset (0); play mode uses main camera + follow.

Safety:
- External pointers (window, entityManager, inputManager) null-checked before use.
- ResourceManager owns GPU resources; handles are validated before bind/draw.
- Minimal per-frame allocations; debug primitives use temporary meshes (batchable later).

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
===============================================================================
*/
#include "Precompiled.h"
#include "GraphicsSystemV2.h"
#include "ECSEntityManager.h"
#include "Component.h"
#include "MeshFactory.h"
#include "Graphics/RenderLayers.h"
#include <iostream>
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <chrono>
#include "Debugger/Trace.h"
#include "Input/Input.h"
#include "imgui.h"
#include "Core/Core.h"
#include "LevelEditor/ImguiSystem.h"
namespace Framework {

    // Check if a string looks like a file path (used to decide whether to load texture by name)
    static bool LooksLikeFilePath(const std::string& s) {
        const auto slash = s.find_last_of("/\\");
        const auto dot = s.find_last_of('.');
        return dot != std::string::npos && (slash == std::string::npos || dot > slash);
    }
    // === CONSTRUCTOR / DESTRUCTOR ===

     // Constructor: Set sane defaults and record initial state.
    GraphicsSystemV2::GraphicsSystemV2()
        : window(nullptr)
        , entityManager(nullptr)
        , inputManager(nullptr)
        , mainCamera(-2.0f, 2.0f, -1.0f, 1.0f, -1.0f, 1.0f)  // Orthographic: left, right, bottom, top, near, far
        , editorCamera(-2.0f, 2.0f, -1.0f, 1.0f, -1.0f, 1.0f)
        , viewportWidth(1600)
        , viewportHeight(800)
        , debugRenderingEnabled(false)
        , currentBoundMaterial(INVALID_MATERIAL_HANDLE)
        , currentBoundShader(INVALID_SHADER_HANDLE)
        , meshFactory()
    {
        std::cout << "GraphicsSystemV2: Constructor\n";
    }
    // Destructor: Release cached resources via ResourceManager and log teardown.
    GraphicsSystemV2::~GraphicsSystemV2() {
        std::cout << "GraphicsSystemV2: Destructor - Cleaning up...\n";

        // Resource manager handles cleanup automatically
        resourceManager.Clear();
    }

    // === CORE LIFECYCLE ===
    // 
    // Initialize: Create GL context state, load resources, create meshes/materials,
    // set render state, initialize text system, and print resource stats.
    void GraphicsSystemV2::Initialize() {
        std::cout << "\n========================================\n";
        std::cout << "  GraphicsSystemV2: Initializing\n";
        std::cout << "========================================\n\n";

        if (!window) {
            std::cerr << "ERROR: No window set! Call SetWindow() before Initialize()\n";
            return;
        }
        //0. Load Meshfactory Values

        meshFactory.MeshValueInitialize();
        // 1. Initialize OpenGL context
        InitializeOpenGL();

        // 2. Load default resources
        LoadDefaultResources();

        // 3. Create default meshes
        CreateDefaultMeshes();

        // 4. Create default materials
        CreateDefaultMaterials();

        // 5. Setup 

        SetupBackground();

        // 6. Create legacy material support
        //CreateLegacyMaterials();

        // 7. Setup initial render state

        SetupRenderState();

        // 8. Allow ResourceManager to load any queued files (if using manifests)
        resourceManager.LoadFiles();

        glfwSwapInterval(1);  //Enables VSync

        text_.init(viewportWidth, viewportHeight, "shaders/text.vert", "shaders/text.frag");

        //set the editor camera to default position - jiahao
        editorCameraStartPos = editorCamera.GetPosition();
        editorCameraZoom = editorCamera.GetZoom();

        // Load fonts (keys must match what DrawText uses)
        text_.loadFont("Sans48", "assets/Font/Orbitron-VariableFont_wght.ttf", 48);
        text_.loadFont("Serif32", "assets/Font/Roboto-VariableFont_wdth,wght.ttf", 32);
        text_.loadFont("Serif32", "assets/Font/EBGaramond_Italic_VariableFont_wght.ttf", 48);
        text_.loadFont("Playfair48", "assets/Font/PlayfairDisplay-Regular.otf", 48);
        text_.loadFont("JaquardaBastarda9", "assets/Font/JacquardaBastarda9-Regular.ttf", 48);
        text_.loadFont("Jacquard12Regular", "assets/Font/Jacquard12-Regular.ttf", 48);
        text_.loadFont("Jersey20Regular", "assets/Font/Jersey20-Regular.ttf", 48);
        std::cout << "\n========================================\n";
        std::cout << "  GraphicsSystemV2: Initialization Complete\n";
        std::cout << "========================================\n\n";

        // Print resource statistics
        auto resourceStats = resourceManager.GetStats();
        std::cout << "Resource Stats:\n";
        std::cout << "  Shaders: " << resourceStats.shaderCount << "\n";
        std::cout << "  Textures: " << resourceStats.textureCount << "\n";
        std::cout << "  Meshes: " << resourceStats.meshCount << "\n";
        std::cout << "  Materials: " << resourceStats.materialCount << "\n\n";
        std::cout << "TextRenderer initialized.\n";
    }

    // FollowPlayer: Smoothly move camera toward player's Transform (XY plane).
    void GraphicsSystemV2::FollowPlayer(EntityManager* em, Entity player)
    {
        if (!em || !em->HasComponent<Transform>(player))
            return;

        auto& playerTransform = em->GetComponent<Transform>(player);

        // Target camera position = player position (z fixed at 0)
        glm::vec3 targetPos(playerTransform.position.x, playerTransform.position.y, 0.0f);

        // Smoothly interpolate camera position toward target (simple exponential smoothing)
        glm::vec3 currentPos = mainCamera.GetPosition();
        float smoothSpeed = 5.0f;  // Tune responsiveness
        float dt = 0.016f;          // or pass your actual deltaTime into this function
        glm::vec3 newPos = glm::mix(currentPos, targetPos, smoothSpeed * dt);


        glm::vec2 orthoHalfExtents = mainCamera.GetOrthoHalfExtents();
        auto& grid = GetGrid();

        float minX = grid.worldbound_min.x + orthoHalfExtents.x;
        float maxX = grid.worldbound_max.x - orthoHalfExtents.x;
        float minY = grid.worldbound_min.y + orthoHalfExtents.y;
        float maxY = grid.worldbound_max.y - orthoHalfExtents.y;

        if (minX <= maxX) {
            newPos.x = std::clamp(newPos.x, minX, maxX);
        }

        if (minY <= maxY) {
            newPos.y = std::clamp(newPos.y, minY, maxY);
        }



        mainCamera.SetPosition(newPos);
    }


    void GraphicsSystemV2::EditorCamDefaultControl(float dt/*EntityManager* em, Entity player*/)
    {
        if (ImGui::GetCurrentContext() && ImGui::GetIO().WantTextInput) {
            return;
        }


        constexpr float cameraSpeed = 5.f;

        glm::vec3 delta(0.0f);
        if (inputManager->IsKeyDown(KeyCode::KEY_W))
            delta.y += cameraSpeed * dt;
        if (inputManager->IsKeyDown(KeyCode::KEY_S))
            delta.y -= cameraSpeed * dt;
        if (inputManager->IsKeyDown(KeyCode::KEY_D))
            delta.x += cameraSpeed * dt;
        if (inputManager->IsKeyDown(KeyCode::KEY_A))
            delta.x -= cameraSpeed * dt;

        editorCamera.Translate(delta);
    }

    // ============================================================================
    // /definition of ResetEditorCamera - Jiahao
    // // author: jiahao Zhou
    // Currently the editor camera share same class as main camera which is "Camera"
    // It is just in editor mode, the camera has different control method
    // so it still using mainCamera variable to represent editor camera
    // ============================================================================

    void GraphicsSystemV2::ResetEditorCamera() {
        editorCamera.SetPosition(editorCameraStartPos);
        editorCamera.SetZoom(editorCameraZoom);
    }

    // ============================================================================
    // /definition of HandleEditorCamera- Jiahao
    // author: jiahao Zhou
    // Currently the editor camera share same class as main camera which is "Camera"
    // It is just in editor mode, the camera has different control method
    // so it still using mainCamera variable to represent editor camera
    // ============================================================================

    void GraphicsSystemV2::HandleEditorCamera(float dt) {

        if (ImGui::GetCurrentContext() && ImGui::GetIO().WantTextInput) {
            return;
        }
        // Define pan and zoom speeds
        const float panSpeed = 0.5f * dt;
        const float zoomSpeed = 0.1f * dt;

        //Panning with arrow keys
        //Basically move the camera position based on arrow key input
        if (inputManager->IsKeyDown(Framework::KEY_LEFT)) {
            editorCamera.Translate({ -panSpeed, 0.0f, 0.0f });
        }
        if (inputManager->IsKeyDown(Framework::KEY_RIGHT)) {
            editorCamera.Translate({ panSpeed, 0.0f, 0.0f });
        }
        if (inputManager->IsKeyDown(Framework::KEY_UP)) {
            editorCamera.Translate({ 0.0f, panSpeed, 0.0f });
        }
        if (inputManager->IsKeyDown(Framework::KEY_DOWN)) {
            editorCamera.Translate({ 0.0f, -panSpeed, 0.0f });
        }

        //Zooming 
        // key 1 to zoom in, key 2 to zoom out
        if (inputManager->IsKeyDown(Framework::KEY_1)) {
            float zoom = editorCamera.GetZoom();
            editorCamera.SetZoom(zoom * (1.0f + zoomSpeed));
        }

        if (inputManager->IsKeyDown(Framework::KEY_2)) {
            float zoom = editorCamera.GetZoom();
            editorCamera.SetZoom(zoom * (1.0f - zoomSpeed));
        }

        //Reset camera position 
        // key 0 to reset camera, and call function ResetEditorCamera
        if (inputManager->IsKeyDown(Framework::KEY_0)) {
            ResetEditorCamera();
        }
    }

    // Update: Per-frame entry point. Handles viewport changes, camera logic, render queue gather/sort/execute, optional debug pass, and error checks.(continuation from kah yan)
    void GraphicsSystemV2::Update(float dt) {

        DBG_SCOPE_SYS("Graphics", eng::debug::Subsystem::Graphics);

        (void)dt;

        if (!window || glfwWindowShouldClose(window)) {
            return;
        }

        // ========================================================================
        // RENDER TARGET SETUP
        // ========================================================================

        if (renderingToTarget && targetFBO != 0) {
            // Bind the custom framebuffer (ImGui viewport)
            glBindFramebuffer(GL_FRAMEBUFFER, targetFBO);
            glViewport(0, 0, targetWidth, targetHeight);

            // Update camera aspect ratio for the viewport size
            float aspectRatio = static_cast<float>(targetWidth) / static_cast<float>(targetHeight);  // FIXED -- 22 Jan 26
            mainCamera.SetOrthographic(-aspectRatio, aspectRatio, -1.0f, 1.0f); // FIXED -- 22 Jan 26
            //mainCamera.SetOrthographic(-targetWidth * 0.5f, targetWidth * 0.5f,
            //    -targetHeight * 0.5f, targetHeight * 0.5f);

            // Update text renderer for viewport size
            text_.setScreenSize(targetWidth, targetHeight);
        }
        else {
            // Normal window rendering
            // Detect window resize each frame and update camera/viewport
            int fbWidth, fbHeight;
            glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
            if (fbWidth != viewportWidth || fbHeight != viewportHeight) {
                SetViewportSize(fbWidth, fbHeight);
            }
        }

        // ========================================================================
        // CAMERA LOGIC
        // ========================================================================

        // === EDITOR CAMERA LOGIC - jiahao
        if (current == LEVEL_2) {
            if (!Framework::CORE->IsPlaying()) {
                HandleEditorCamera(dt);
            }
        }

        // === CAMERA FOLLOW LOGIC ===
        if (Framework::CORE->IsPlaying()) {
            if (followEnabled && entityManager && followTarget.IsValid()/* && Framework::CORE->IsPlaying()*/) {
                FollowPlayer(entityManager, followTarget);
            }
        }
        else {
            HandleEditorCamera(dt);
        }

        // ========================================================================
        // CLEAR AND RENDER
        // ========================================================================

        // Clear the current target (either FBO or screen)
        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Reset statistics
        stats = RenderStats();

        // Gather render commands from ECS
        GatherRenderCommands();

        // Sort render queue for optimal rendering
        renderQueue.Sort();

        // Execute render commands
        ExecuteRenderQueue();

        // Render debug visualizations if enabled
        if (debugRenderingEnabled) {
            RenderDebugPrimitives();
        }

        // Clear queues for next frame
        renderQueue.Clear();
        debugQueue.Clear();

        // ========================================================================
        // FPS CALCULATION AND TOGGLE (E key)
        // ========================================================================
        
        // Toggle FPS display with E key (with debounce to prevent double-toggle)
        static bool lastEKeyState = false;
        bool currentEKeyState = inputManager && inputManager->IsKeyDown(KEY_E);
        
        if (currentEKeyState && !lastEKeyState) {
            // Check if ImGui wants keyboard input - if so, don't toggle
            bool imguiWantsKeyboard = false;
            if (ImGui::GetCurrentContext()) {
                imguiWantsKeyboard = ImGui::GetIO().WantCaptureKeyboard;
            }
            
            if (!imguiWantsKeyboard) {
                showFPS = !showFPS;
                std::cout << "[FPS] FPS display " << (showFPS ? "enabled" : "disabled") << " (Press E to toggle)\n";
            }
        }
        lastEKeyState = currentEKeyState;

        // FPS calculation moved to RenderImGui() which is called once per frame

        // ========================================================================
        // UNBIND RENDER TARGET
        // ========================================================================

        if (renderingToTarget && targetFBO != 0) {
            // Return to default framebuffer
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            // Restore viewport to window size
            int fbWidth, fbHeight;
            glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
            glViewport(0, 0, fbWidth, fbHeight);

            // Restore camera aspect ratio
            float aspectRatio = static_cast<float>(fbWidth) / static_cast<float>(fbHeight);                 //-- FIXED 22 Jan 2026
            mainCamera.SetOrthographic(-aspectRatio, aspectRatio, -1.0f, 1.0f);        //-- FIXED 22 Jan 2026
            //mainCamera.SetOrthographic(-fbWidth * 0.5f, fbWidth * 0.5f,
            //    -fbHeight * 0.5f, fbHeight * 0.5f);

            // Restore text renderer
            text_.setScreenSize(fbWidth, fbHeight);
        }

        // Check for OpenGL errors
        GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
            std::cerr << "OpenGL error in Update: 0x" << std::hex << error << std::dec << "\n";
        }
    }

    // SendEngineMessage: React to engine-wide messages (e.g., Quit).
    void GraphicsSystemV2::SendEngineMessage(Message* message) {
        if (message->MessageId == Status::Quit) {
            std::cout << "GraphicsSystemV2: Received quit message\n";
        }
    }

    // === SETUP ===
    // Provide GLFW window (must be set before Initialize()).
    void GraphicsSystemV2::SetWindow(GLFWwindow* win) {
        window = win;
    }
    // Connect ECS EntityManager for renderable collection and follow logic.
    void GraphicsSystemV2::SetEntityManager(EntityManager* em) {
        entityManager = em;
    }
    void GraphicsSystemV2::SetInputSystem(InputSystem* is) {
        inputManager = is;
    }
    // SetViewportSize: Update GL viewport, camera projection, and text renderer.
    void GraphicsSystemV2::SetViewportSize(int width, int height) {
        viewportWidth = width;
        viewportHeight = height;

        glViewport(0, 0, width, height);

        // Maintain orthographic projection with aspect-preserving extents
        float aspectRatio = static_cast<float>(width) / static_cast<float>(height);                 // -- BUG (FIXED 22 Jan 2026)
        mainCamera.SetOrthographic(-aspectRatio, aspectRatio, -1.0f, 1.0f);    // -- BUG (FIXED 22 Jan 2026)
        //mainCamera.SetOrthographic(-width * 0.5f, width * 0.5f,
        //    -height * 0.5f, height * 0.5f);
        editorCamera.SetOrthographic(-width * 0.5f, width * 0.5f,
            -height * 0.5f, height * 0.5f);
        
        // Keep text renderer aligned to screen size
        text_.setScreenSize(width, height);

        std::cout << "GraphicsSystemV2: Viewport resized to " << width << "x" << height << "\n";
    }
    // Set camera position explicitly (editor or scripted motion).
    void GraphicsSystemV2::SetCameraPosition(const glm::vec3& position) {
        mainCamera.SetPosition(position);
    }
    // Set camera zoom explicitly (orthographic scale).
    void GraphicsSystemV2::SetCameraZoom(float zoom) {
        mainCamera.SetZoom(zoom);
    }

    // === INITIALIZATION HELPERS ===
    // InitializeOpenGL: Make context current, initialize GLEW, and log device info.
    void GraphicsSystemV2::InitializeOpenGL() {
        std::cout << "GraphicsSystemV2: Initializing OpenGL...\n";

        glfwMakeContextCurrent(window);

        // Initialize GLEW for function loading
        glewExperimental = GL_TRUE;
        GLenum glewError = glewInit();
        if (glewError != GLEW_OK) {
            std::cerr << "GLEW initialization failed: "
                << glewGetErrorString(glewError) << "\n";
            return;
        }

        // Print OpenGL information
        std::cout << "\n=== OpenGL Information ===\n";
        std::cout << "Version: " << glGetString(GL_VERSION) << "\n";
        std::cout << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << "\n";
        std::cout << "Vendor: " << glGetString(GL_VENDOR) << "\n";
        std::cout << "Renderer: " << glGetString(GL_RENDERER) << "\n\n";

        // Set initial viewport
        glViewport(0, 0, viewportWidth, viewportHeight);
    }

    // LoadDefaultResources: Compile/link default shaders and assign debug shader.
    void GraphicsSystemV2::LoadDefaultResources() {
        std::cout << "GraphicsSystemV2: Loading default resources...\n";

        // Default shader (basic textured/colored)
        defaultShader = resourceManager.LoadShader(
            "shaders/basic.vert",
            "shaders/basic.frag",
            "default"
        );
        // Alternate fragment shader
        Shader2 = resourceManager.LoadShader(
            "shaders/basic.vert",
            "shaders/basic2.frag",
            "color"
        );

        if (!defaultShader.IsValid()) {
            std::cerr << "ERROR: Failed to load default shader!\n";
        }

        // Load debug shader
        debugShader = defaultShader;
    }

    // CreateDefaultMeshes: Build/Load basic shapes and register with ResourceManager.
    void GraphicsSystemV2::CreateDefaultMeshes() {
        std::cout << "GraphicsSystemV2: Creating default meshes...\n";
        // Create primitive meshes using the factory functions
        //Mesh* quad = meshFactory.CreateQuad();
        //Mesh* line = meshFactory.CreateLine();
        //Mesh* circle = meshFactory.CreateCircle(40, 0.5f);
        //Mesh* wireframeQ = meshFactory.CreateWireframeQuad();

        // Quad vertices
        std::vector<float> quadVerts = {
           -0.5f, -0.5f, 0.0f,  1.0f, 0.0f, 0.0f,  0.0f, 0.0f,
            0.5f, -0.5f, 0.0f,  0.0f, 1.0f, 0.0f,  1.0f, 0.0f,
            0.5f,  0.5f, 0.0f,  0.0f, 0.0f, 1.0f,  1.0f, 1.0f,
           -0.5f,  0.5f, 0.0f,  1.0f, 1.0f, 0.0f,  0.0f, 1.0f
        };
        std::vector<unsigned int> quadIndices = { 0, 1, 2, 2, 3, 0 };
        quadMesh = resourceManager.CreateMesh("quad", quadVerts, quadIndices, GL_TRIANGLES, true);

        // Line vertices
        std::vector<float> lineVerts = {
           -0.5f, 0.0f, 0.0f,  1.0f, 0.0f, 1.0f,  0.0f, 0.0f,
            0.5f, 0.0f, 0.0f,  0.0f, 1.0f, 1.0f,  1.0f, 0.0f
        };
        lineMesh = resourceManager.CreateMesh("line", lineVerts, {}, GL_LINES, true);

        // Circle vertices (procedural)
        std::vector<float> circleVerts;
        int segments = 40;
        float radius = 0.5f;

        // Center
        circleVerts.insert(circleVerts.end(), { 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.5f, 0.5f });

        // Perimeter
        for (int i = 0; i <= segments; i++) {
            float theta = (2.0f * 3.14159f * i) / segments;
            float x = radius * cos(theta);
            float y = radius * sin(theta);
            float r = (cos(theta) + 1.0f) * 0.5f;
            float g = (sin(theta) + 1.0f) * 0.5f;
            float b = 1.0f - r;
            float u = (x / radius + 1.0f) * 0.5f;
            float v = (y / radius + 1.0f) * 0.5f;
            circleVerts.insert(circleVerts.end(), { x, y, 0.0f, r, g, b, u, v });
        }

        circleMesh = resourceManager.CreateMesh("circle", circleVerts, {}, GL_TRIANGLE_FAN, true);

        // Register wireframe quad
        std::vector<float> wireframeQuadVerts = {
            // Bottom left
            -0.5f, -0.5f, 0.0f,  1.0f, 1.0f, 1.0f,  0.0f, 0.0f,
            // Bottom right
             0.5f, -0.5f, 0.0f,  1.0f, 1.0f, 1.0f,  1.0f, 0.0f,
             // Top right
              0.5f,  0.5f, 0.0f,  1.0f, 1.0f, 1.0f,  1.0f, 1.0f,
              // Top left
              -0.5f,  0.5f, 0.0f,  1.0f, 1.0f, 1.0f,  0.0f, 1.0f,
              // Close the loop
              -0.5f, -0.5f, 0.0f,  1.0f, 1.0f, 1.0f,  0.0f, 0.0f
        };

        wireframeQMesh = resourceManager.CreateMesh(
            "wireframequad",
            wireframeQuadVerts,
            {},
            GL_LINE_STRIP,
            true
        );

        std::cout << "Created " << 4 << " default meshes\n";
    }

    // CreateDefaultMaterials: Instantiate per-primitive materials with base tints.
    void GraphicsSystemV2::CreateDefaultMaterials() {
        std::cout << "GraphicsSystemV2: Creating default materials...\n";

        if (!defaultShader.IsValid()) {
            std::cerr << "ERROR: Cannot create materials without valid shader!\n";
            return;
        }

        // Create default material
        defaultMaterial = resourceManager.CreateMaterial("default", defaultShader);
        auto* defaultMat = resourceManager.GetMaterial(defaultMaterial);
        if (defaultMat) {
            defaultMat->blendMode = BlendMode::AlphaBlend;
            defaultMat->depthTest = false;   // Disable for 2D rendering
            defaultMat->depthWrite = false;
        }

        Material2 = resourceManager.CreateMaterial("color", Shader2);
        auto* mat2 = resourceManager.GetMaterial(Material2);
        if (mat2) {
            mat2->blendMode = BlendMode::AlphaBlend;
            mat2->depthTest = false;   // Disable for 2D rendering
            mat2->depthWrite = false;
        }

        // Create materials for each primitive
        // Quad material
        quadMaterial = resourceManager.CreateMaterial("quad_mat", defaultShader);
        auto* quadMat = resourceManager.GetMaterial(quadMaterial);
        if (quadMat) {
            quadMat->tint = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
            quadMat->blendMode = BlendMode::AlphaBlend;
            quadMat->depthTest = false;   // Disable for 2D rendering
            quadMat->depthWrite = false;
        }
        // Line material
        lineMaterial = resourceManager.CreateMaterial("line_mat", defaultShader);
        auto* lineMat = resourceManager.GetMaterial(lineMaterial);
        if (lineMat) {
            lineMat->tint = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
            lineMat->blendMode = BlendMode::AlphaBlend;
            lineMat->depthTest = false;   // Disable for 2D rendering
            lineMat->depthWrite = false;
        }
        // Circle material
        circleMaterial = resourceManager.CreateMaterial("circle_mat", defaultShader);
        auto* circleMat = resourceManager.GetMaterial(circleMaterial);
        if (circleMat) {
            circleMat->tint = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
            circleMat->blendMode = BlendMode::AlphaBlend;
            circleMat->depthTest = false;   // Disable for 2D rendering
            circleMat->depthWrite = false;
        }
        // Wireframe quad material
        wireframeQMaterial = resourceManager.CreateMaterial("wireframeq_mat", defaultShader);
        auto* wireframeQMat = resourceManager.GetMaterial(wireframeQMaterial);
        if (wireframeQMat) {
            wireframeQMat->tint = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
            wireframeQMat->blendMode = BlendMode::AlphaBlend;
            wireframeQMat->depthTest = false;   // Disable for 2D rendering
            wireframeQMat->depthWrite = false;
        }

        std::cout << "Created " << 5 << " default materials\n";
    }

    // SetupBackground: Load texture, assign quad mesh, and create a background material.
    void GraphicsSystemV2::SetupBackground() {
        std::cout << "GraphicsSystemV2: Setting up background...\n";

        // Load background texture
        backgroundTexture = resourceManager.LoadTexture("assets/Menu/Wood_Background.png");

        if (!backgroundTexture.IsValid()) {
            std::cerr << "WARNING: Failed to load background texture\n";
            return;
        }

        // Use the quad mesh for background
        backgroundMesh = quadMesh;

        // Create background material
        backgroundMaterial = resourceManager.CreateMaterial("background", defaultShader);
        auto* bgMat = resourceManager.GetMaterial(backgroundMaterial);
        if (bgMat) {
            bgMat->albedoTexture = backgroundTexture;
            bgMat->tint = glm::vec4(1.0f);  // No tinting
            bgMat->blendMode = BlendMode::AlphaBlend;
            bgMat->depthTest = false;   // Disable for 2D rendering
            bgMat->depthWrite = false;
        }

        std::cout << "Background setup complete\n";
    }

    // SetupRenderState: Global GL state for this renderer.
    void GraphicsSystemV2::SetupRenderState() {
        // Enable alpha blending
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // DISABLE depth testing for 2D sprite rendering
        // Traditional 2D approach: rely on painter's algorithm (render queue sorting)
        // This avoids AMD GPU precision issues and transparent sprite artifacts
        glDisable(GL_DEPTH_TEST);

        // Enable back-face culling
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
    }

    // Resolve sprite name to a texture if the string looks like a file path.
    TextureHandle GraphicsSystemV2::GetTextureForSpriteName(const std::string& name) {
        // Only try to load when it looks like a file path (e.g., "assets/x.png")
        if (LooksLikeFilePath(name)) {
            return resourceManager.EnsureTexture(name); // cache-aware; creates handle if missing
        }
        return INVALID_TEXTURE_HANDLE; // logical names like "quad" shouldn't bind a texture
    }

    // === RENDERING PHASES ===

    // GatherRenderCommands: Build RenderQueue from ECS (background + entities).
    void GraphicsSystemV2::GatherRenderCommands() {
        if (!entityManager) return;

        // Idealy this should also be an enetity, and loaded from a file
        // --- Background pass ---
        if (backgroundMaterial.IsValid() && backgroundMesh.IsValid()) {
            RenderCommand bg;
            bg.mesh = backgroundMesh;
            bg.material = backgroundMaterial;
            bg.texture = backgroundTexture;
            bg.layer = RenderLayers::Background;  // Renders first, behind everything
            bg.modelMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(4.0f));
            bg.tint = glm::vec4(1.0f);
            renderQueue.Submit(bg);
        }

        // Entity passes
        for (Entity e : entityManager->GetAllEntities()) {

            if (!entityManager->HasComponent<Transform>(e))
                continue;

            auto& transform = entityManager->GetComponent<Transform>(e);

            const bool hasRenderer = entityManager->HasComponent<MeshRenderer>(e);
            const bool hasSprite = entityManager->HasComponent<Sprite>(e);
            if (!hasRenderer && !hasSprite) continue;

            RenderCommand cmd;

            // ---------- MeshRenderer ----------
            if (hasRenderer) {
                auto& mr = entityManager->GetComponent<MeshRenderer>(e);
                if (!mr.visible) continue;

                // Use provided mesh or default to quad
                cmd.mesh = mr.mesh.IsValid() ? mr.mesh : quadMesh;

                // Ensure each renderer gets an instanced material (so UV edits don't leak)
                if (!mr.material.IsValid()) {
                    Material* base = resourceManager.GetMaterial(defaultMaterial);
                    if (!base) continue;

                    std::string matName = "entity_mat_" + std::to_string(e.GetID());

                    // DEBUG: Check if this material already exists to avoid duplicates
                    MaterialHandle existing = resourceManager.GetMaterialHandle(matName);
                    if (existing.IsValid()) {
                        mr.material = existing;
                        // Don't continue here - we still need to set up the render command!
                    }
                    else {
                        MaterialHandle inst = resourceManager.CreateMaterial(matName, base->shader);

                        Material* pm = resourceManager.GetMaterial(inst);
                        if (!pm) continue;

                        *pm = *base; // shallow copy of defaults
                        pm->tint = glm::vec4(1.0f); // Force material tint to white
                        mr.material = inst;
                    }
                }

                cmd.material = mr.material.IsValid() ? mr.material : defaultMaterial;

                // Choose texture: explicit handle first, else attempt load by sprite name
                if (mr.texture.IsValid()) {
                    cmd.texture = mr.texture;
                }
                else if (!mr.spriteName.empty()) {
                    TextureHandle tex = resourceManager.LoadTexture(mr.spriteName);
                    if (tex.IsValid()) {
                        cmd.texture = tex;
                        mr.texture = tex;
                    }
                }

                cmd.tint = mr.tint;
                cmd.layer = mr.layer;
                cmd.orderInLayer = mr.orderInLayer;

                if (Framework::CORE && Framework::CORE->IsEditorMode())
                {
                    ImGuiSystem* imgui = Framework::CORE->GetImGuiSystem();
                    if (imgui && !imgui->IsRenderLayerVisible(cmd.layer))
                    {
                        continue;
                    }
                }
            }

            //---------- SPRITE ----------
            else if (hasSprite) {
                auto& sp = entityManager->GetComponent<Sprite>(e);

                cmd.mesh = quadMesh;
                cmd.material = defaultMaterial;
                // Load texture only if spritePath looks like a real file
                if (!sp.texturePath.empty() && LooksLikeFilePath(sp.texturePath)) {
                    TextureHandle tex = resourceManager.LoadTexture(sp.texturePath);
                    cmd.texture = tex.IsValid() ? tex : INVALID_TEXTURE_HANDLE;
                }
                else {
                    cmd.texture = INVALID_TEXTURE_HANDLE;
                }

                cmd.tint = sp.tint;  // Use Sprite's tint instead of hardcoded white
                cmd.layer = sp.layer;

                if (Framework::CORE && Framework::CORE->IsEditorMode())
                {
                    ImGuiSystem* imgui = Framework::CORE->GetImGuiSystem();
                    if (imgui && !imgui->IsRenderLayerVisible(cmd.layer))
                    {
                        continue;
                    }
                }
            }

            // ============================================================================
            // Author:        Tan Wei Leong
            // Email:         weileong.tan@digipen.edu
            // Date:          2025-11-06
            // Contribution:  100% (Transformation matrix + UV animation mapping)
            // -----------------------------------------------------------------------------
            // Description:
            //   This section handles per-entity transformation and sprite sheet UV
            //   animation logic within the rendering pipeline.
            //
            //   - Transformation Block
            //     Converts ECS Transform component data (position, rotation, scale) into a
            //     model matrix used by the GPU for world-space rendering.
            //     Each entity receives its own transform, enabling independent movement
            //     and scaling across the scene.
            //
            //   - UV Animation Block
            //     Computes UV coordinates for the active animation frame defined by
            //     SpriteAnimation. It divides the sprite sheet into frame-sized cells and
            //     dynamically adjusts UVs each frame, also supporting horizontal flipping.
            //
            //   Integrated Systems:
            //     - Uses Transform (position, rotation, scale)
            //     - Uses SpriteAnimation (frame-based texture slicing)
            //     - Updates Material UVs before rendering (GraphicsSystemV2)
            //
            //   Key Features Implemented:
            //     - Real-time model matrix composition (translate, rotate, scale)
            //     - Frame-based UV mapping for sprite sheets
            //     - Horizontal flip support via flipX
            //     - UV shrink correction to avoid texture bleeding
            // ============================================================================

            // ---------- TRANSFORM ----------
            glm::mat4 model(1.0f);
            // Use layer, orderInLayer, and entity ID for Z-depth to prevent Z-fighting
            // Increased multipliers to avoid floating-point precision issues on AMD GPUs
            // Layer provides major depth separation (0.01 per layer = 100x precision margin)
            // OrderInLayer provides minor separation (0.0001 per order = 1.67x precision margin)
            // Entity ID provides guaranteed uniqueness (0.000001 per ID = 16.7x precision margin)
            // 24-bit depth precision ≈ 0.00000006, these values are well above that threshold
            float zDepth = (cmd.layer * 0.01f) + (cmd.orderInLayer * 0.0001f) + (e.GetID() * 0.000001f);
            model = glm::translate(model, { transform.position.x, transform.position.y, zDepth });
            model = glm::rotate(model, glm::radians(transform.rotation), { 0, 0, 1 });
            model = glm::scale(model, { transform.scale.x, transform.scale.y, 1.0f });
            cmd.modelMatrix = model;

            // Compute depth relative to camera (for correct draw order)
            cmd.depth = glm::distance(
                glm::vec3(transform.position.x, transform.position.y, zDepth),
                mainCamera.GetPosition()
            );

            // ---------- SPRITE SHEET UV ANIMATION ----------
            if (!entityManager->HasComponent<SpriteAnimation>(e))
            {
                // Non-animated entities: Enforce defaultMaterial for shader compatibility
                // This ensures tiles and static sprites render correctly
                Material* mat = resourceManager.GetMaterial(cmd.material);
                if (!mat || mat->shader != defaultShader) {
                    cmd.material = defaultMaterial;
                    mat = resourceManager.GetMaterial(cmd.material);
                }

                if (mat) {
                    mat->u1 = mat->v1 = 1.f;
                    mat->u0 = mat->v0 = 0.f;
                }

                // Entity has no animation - safe to submit as-is
                renderQueue.Submit(cmd);
                continue;
            }

            auto& anim = entityManager->GetComponent<SpriteAnimation>(e);

            cmd.texture = anim.spriteSheet;

            // Get the material for this command
            Material* mat = resourceManager.GetMaterial(cmd.material);

            // ANIMATED ENTITIES: Ensure material has correct shader for texture rendering
            // Players are spawned with Shader2 (color-only) which doesn't support textures
            // We need to switch to defaultShader (textured) for animations to work
            if (!mat)
            {
                // Material is missing - use default
                cmd.material = defaultMaterial;
                mat = resourceManager.GetMaterial(cmd.material);
            }
            else if (mat->shader != defaultShader)
            {
                // Material exists but has wrong shader (e.g., Shader2/color-only)
                // Fix the shader to support textures/UV coordinates
                LOG_INFO("ANIM_FIX", "Entity %u: Switching material shader from %u to defaultShader %u for animation support",
                    e.GetID(), mat->shader.GetID(), defaultShader.GetID());
                mat->shader = defaultShader;
            }

            // If still failed for some reason, skip this entity
            if (!mat)
                continue;
            // FIX: Reset UV to full-texture defaults before computing frame UVs.
            // Previously was: mat->u0 = mat->v0; (BUG - copied stale v0 into u0)
            // This caused garbage UV rects on early-exit paths during animation switches.
            mat->u0 = 0.0f;
            mat->v0 = 0.0f;
            mat->u1 = 1.0f;
            mat->v1 = 1.0f;

            // Ensure the material is bound to this sprite sheet
            mat->albedoTexture = anim.spriteSheet;

            Texture* tex = resourceManager.GetTexture(anim.spriteSheet);
            if (!tex) {
                // Still render with default UVs if texture is missing
                renderQueue.Submit(cmd);
                continue;
            }

            const int texW = tex->GetWidth();
            const int texH = tex->GetHeight();

            // CRITICAL FIX: If frame dimensions are invalid, compute them from texture and animation settings
            int safeFrameWidth = anim.frameWidth;
            int safeFrameHeight = anim.frameHeight;
            int safeCols = anim.columns;
            int safeRows = anim.rows;

            // If frame dimensions are 0 but we have valid columns/rows, compute from texture
            if ((safeFrameWidth <= 0 || safeFrameHeight <= 0) && texW > 0 && texH > 0) {
                if (safeCols <= 0) safeCols = 1;
                if (safeRows <= 0) safeRows = 1;
                safeFrameWidth = texW / safeCols;
                safeFrameHeight = texH / safeRows;
            }

            // Final validation - if still invalid, use full texture as single frame
            if (safeFrameWidth <= 0 || safeFrameHeight <= 0 || texW <= 0 || texH <= 0) {
                // Can't compute valid UVs, render with defaults (full texture)
                renderQueue.Submit(cmd);
                continue;
            }

            // Use safe values computed above for cols/rows calculation
            const int cols = (safeCols > 0) ? safeCols : 1;
            const int rows = (safeRows > 0) ? safeRows : 1;

            const int totalCells = cols * rows;
            if (cols <= 0 || rows <= 0 || totalCells <= 0) {
                // Still render with default UVs
                renderQueue.Submit(cmd);
                continue;
            }

            // Clamp frameCount so we never walk past the sheet
            int safeFrameCount = anim.frameCount;
            if (safeFrameCount <= 0)
                safeFrameCount = 1;

            const int maxFramesAvailable = totalCells - anim.startFrame;
            if (maxFramesAvailable <= 0)
            {
                safeFrameCount = 1;
            }
            else if (safeFrameCount > maxFramesAvailable)
            {
                safeFrameCount = maxFramesAvailable;
            }

            const int frameInRange = anim.currentFrame % safeFrameCount;
            int actualFrame = anim.startFrame + frameInRange;

            // Final absolute clamp (paranoia)
            if (actualFrame < 0) actualFrame = 0;
            if (actualFrame >= totalCells) actualFrame = totalCells - 1;

            const int x = actualFrame % cols;
            const int y = actualFrame / cols;
            if (y < 0 || y >= rows) {
                // Still render with default UVs
                renderQueue.Submit(cmd);
                continue;
            }


            // Treat uvShrinkPx as pixels trimmed from each side of the frame
            float shrink = anim.uvShrinkPx;
            if (shrink < 0.0f) shrink = 0.0f;
            if (shrink * 2.0f >= safeFrameWidth)  shrink = (safeFrameWidth - 1) * 0.5f;
            if (shrink * 2.0f >= safeFrameHeight) shrink = (safeFrameHeight - 1) * 0.5f;

            // Pixel coordinates inside the big texture
            float leftPx = x * safeFrameWidth + shrink;
            float rightPx = (x + 1) * safeFrameWidth - shrink;
            float topPx = y * safeFrameHeight + shrink;
            float bottomPx = (y + 1) * safeFrameHeight - shrink;

            // Convert to UV [0,1]
            float u0 = leftPx / float(texW);
            float u1 = rightPx / float(texW);
            float v1 = 1.0f - (topPx / float(texH));      // top
            float v0 = 1.0f - (bottomPx / float(texH));   // bottom

            // Apply horizontal flipping if enabled
            if (anim.flipX)
                std::swap(u0, u1);

            // Update material UV bounds
            mat->u0 = u0;
            mat->u1 = u1;
            mat->v0 = v0;
            mat->v1 = v1;

            // Ensure texture is valid before rendering
            if (!mat->albedoTexture.IsValid())
                mat->albedoTexture = anim.spriteSheet;

            renderQueue.Submit(cmd);
        }
    }

    // GraphicsSystemV2.cpp
    void GraphicsSystemV2::ExecuteRenderQueue() {
        const auto& commands = renderQueue.GetCommands();
        if (commands.empty()) return;

        Camera& activeCamera = Framework::CORE->IsPlaying() ? mainCamera : editorCamera;
        glm::mat4 projection = activeCamera.GetProjectionMatrix();
        glm::mat4 view = activeCamera.GetViewMatrix();

        currentBoundMaterial = INVALID_MATERIAL_HANDLE;
        currentBoundShader = INVALID_SHADER_HANDLE;

        // Linear Batching: This ensures Layer -10 draws BEFORE Layer 0
        std::vector<glm::mat4> batchMatrices;
        const RenderCommand* batchBase = nullptr;

        auto FlushBatch = [&]() {
            if (batchMatrices.empty() || !batchBase) return;

            // 1. Bind Material (ALWAYS rebind because tint may have changed)
            BindMaterial(batchBase->material, batchBase->tint);
            currentBoundMaterial = batchBase->material;

            Shader* shader = resourceManager.GetShader(currentBoundShader);
            if (shader) {
                // 2. Update Camera
                GLint projLoc = glGetUniformLocation(shader->GetID(), "uProjection");
                GLint viewLoc = glGetUniformLocation(shader->GetID(), "uView");
                if (projLoc != -1) glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
                if (viewLoc != -1) glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

                // 3. Bind Texture (CRITICAL FIX for switching between Wood and Button)
                if (batchBase->texture.IsValid())
                {
                    Texture* tex = resourceManager.GetTexture(batchBase->texture);
                    if (tex)
                    {
                        glBindTextureUnit(0, tex->GetID()); // <-- bind the REAL OpenGL texture ID
                        glUniform1i(glGetUniformLocation(shader->GetID(), "uUseTexture"), 1);
                    }
                    else
                    {
                        glUniform1i(glGetUniformLocation(shader->GetID(), "uUseTexture"), 0);
                    }
                }
                else
                {
                    glUniform1i(glGetUniformLocation(shader->GetID(), "uUseTexture"), 0);
                }


                // 4. Draw
                Mesh* mesh = resourceManager.GetMesh(batchBase->mesh);
                if (mesh) {
                    mesh->SetInstanceData();
                    glNamedBufferSubData(mesh->instanceVBO, 0, batchMatrices.size() * sizeof(glm::mat4), batchMatrices.data());
                    mesh->DrawInstanced(batchMatrices, static_cast<GLsizei>(batchMatrices.size()));
                    stats.drawCalls++;
                }
            }
            batchMatrices.clear();
            };

        for (const auto& cmd : commands) {
            if (!cmd.visible) continue;

            bool isSameBatch = false;
            if (batchBase && cmd.mesh == batchBase->mesh &&
                cmd.material == batchBase->material &&
                cmd.texture == batchBase->texture &&
                cmd.tint == batchBase->tint) {
                isSameBatch = true;
            }

            if (!isSameBatch) {
                FlushBatch();
                batchBase = &cmd;
            }
            batchMatrices.push_back(cmd.modelMatrix);
        }
        FlushBatch(); // Draw final batch

        if (currentBoundShader.IsValid()) {
            if (auto* sh = resourceManager.GetShader(currentBoundShader)) sh->Unbind();
        }
    }

    void GraphicsSystemV2::RenderDebugPrimitives() {
        if (!debugShader.IsValid()) {
            return;
        }

        Shader* shader = resourceManager.GetShader(debugShader);
        if (!shader) {
            return;
        }

        shader->Bind();

        glm::mat4 projection = mainCamera.GetProjectionMatrix();
        glm::mat4 view = mainCamera.GetViewMatrix();

        GLint projLoc = glGetUniformLocation(shader->GetID(), "uProjection");
        if (projLoc != -1) {
            glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
        }

        // Disable texturing for debug rendering
        glUniform1i(glGetUniformLocation(shader->GetID(), "uUseTexture"), 0);

        // Render debug lines
        for (const auto& line : debugQueue.GetLines()) {
            // Create line vertices
            std::vector<float> verts = {
                line.start.x, line.start.y, line.start.z, line.color.r, line.color.g, line.color.b, 0.0f, 0.0f,
                line.end.x, line.end.y, line.end.z, line.color.r, line.color.g, line.color.b, 1.0f, 0.0f
            };

            // Create temporary mesh (inefficient - should be batched in production)
            Mesh debugLine(verts, GL_LINES, true);

            glm::mat4 model = glm::mat4(1.0f);
            GLint modelLoc = glGetUniformLocation(shader->GetID(), "uModel");
            if (modelLoc != -1) {
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            }

            GLint colorLoc = glGetUniformLocation(shader->GetID(), "uColor");
            if (colorLoc != -1) {
                glUniform3f(colorLoc, line.color.r, line.color.g, line.color.b);
            }

            debugLine.Draw();
        }

        // Render debug circles
        for (const auto& circle : debugQueue.GetCircles()) {
            // Use the circle mesh and scale it
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, circle.center);
            model = glm::scale(model, glm::vec3(circle.radius * 2.0f));

            GLint modelLoc = glGetUniformLocation(shader->GetID(), "uModel");
            if (modelLoc != -1) {
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            }

            GLint colorLoc = glGetUniformLocation(shader->GetID(), "uColor");
            if (colorLoc != -1) {
                glUniform3f(colorLoc, circle.color.r, circle.color.g, circle.color.b);
            }

            if (circleMesh.IsValid()) {
                DrawMesh(circleMesh);
            }
        }

        // Render debug boxes
        for (const auto& box : debugQueue.GetBoxes()) {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, box.center);
            model = glm::scale(model, box.size);

            GLint modelLoc = glGetUniformLocation(shader->GetID(), "uModel");
            if (modelLoc != -1) {
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            }

            GLint colorLoc = glGetUniformLocation(shader->GetID(), "uColor");
            if (colorLoc != -1) {
                glUniform3f(colorLoc, box.color.r, box.color.g, box.color.b);
            }

            if (quadMesh.IsValid()) {
                DrawMesh(quadMesh);
            }
        }

        shader->Unbind();
    }

    // === RENDERING HELPERS ===

    bool GraphicsSystemV2::BindMaterial(MaterialHandle materialHandle, const glm::vec4& tint) {
        Material* material = resourceManager.GetMaterial(materialHandle);
        if (!material) {
            return false;
        }

        // Bind shader
        Shader* shader = resourceManager.GetShader(material->shader);
        if (!shader) {
            return false;
        }

        shader->Bind();
        currentBoundShader = material->shader;

        // DEBUG: Log UV rect being passed to shader (ALWAYS LOG FOR DEBUGGING)
        // Debug logging disabled
        // static int uvRectLogCounter = 0;
        // bool shouldLogUVRect = (++uvRectLogCounter % 60 == 0);
        // if (shouldLogUVRect) {
        //     LOG_INFO("UV_SHADER", "BindMaterial: material='%s' handle=%u uUVRect=(%.3f,%.3f,%.3f,%.3f)",
        //         material->name.c_str(), materialHandle.GetID(),
        //         material->u0, material->v0, material->u1, material->v1);
        // }

        glUniform4f(glGetUniformLocation(shader->GetID(), "uUVRect"),
            material->u0, material->v0, material->u1, material->v1);

        // Set blend mode
        switch (material->blendMode) {
        case BlendMode::Opaque:
            glDisable(GL_BLEND);
            break;
        case BlendMode::AlphaBlend:
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            break;
        case BlendMode::Additive:
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            break;
        case BlendMode::Multiply:
            glEnable(GL_BLEND);
            glBlendFunc(GL_DST_COLOR, GL_ZERO);
            break;
        }

        // Set alpha discard flag based on blend mode
        // Opaque mode: don't discard pixels based on alpha (render everything including black)
        // Other modes: discard transparent pixels for proper alpha blending
        GLint alphaDiscardLoc = glGetUniformLocation(shader->GetID(), "uUseAlphaDiscard");
        if (alphaDiscardLoc != -1) {
            bool useAlphaDiscard = (material->blendMode != BlendMode::Opaque);
            glUniform1i(alphaDiscardLoc, useAlphaDiscard ? 1 : 0);
        }

        // Set force opaque alpha flag
        // When true, ignores texture alpha channel and forces all pixels to alpha=1.0
        // This fixes black pixels with alpha=0 in PNG files
        GLint forceOpaqueAlphaLoc = glGetUniformLocation(shader->GetID(), "uForceOpaqueAlpha");
        if (forceOpaqueAlphaLoc != -1) {
            glUniform1i(forceOpaqueAlphaLoc, material->forceOpaqueAlpha ? 1 : 0);
        }

        // Set depth test
        if (material->depthTest) {
            glEnable(GL_DEPTH_TEST);
        }
        else {
            glDisable(GL_DEPTH_TEST);
        }

        // Set depth write
        glDepthMask(material->depthWrite ? GL_TRUE : GL_FALSE);

        // Set culling
        if (material->cullBackFace) {
            glEnable(GL_CULL_FACE);
        }
        else {
            glDisable(GL_CULL_FACE);
        }

        // Bind textures
        bool hasTexture = false;
        if (material->albedoTexture.IsValid()) {
            Texture* texture = resourceManager.GetTexture(material->albedoTexture);
            if (texture) {
                texture->Bind(0);
                hasTexture = true;
            }
        }

        // Set grayscale amount if provided by material parameters
        float grayAmount = 0.0f;
        auto grayIt = material->parameters.find("grayAmount");
        if (grayIt != material->parameters.end()) {
            if (auto value = std::get_if<float>(&grayIt->second)) {
                grayAmount = std::clamp(*value, 0.0f, 1.0f);
            }
        }
        GLint grayLoc = glGetUniformLocation(shader->GetID(), "uGrayAmount");
        if (grayLoc != -1) {
            glUniform1f(grayLoc, grayAmount);
        }

        // Set color tint (combine material tint with instance tint)
        glm::vec3 finalTint = glm::vec3(material->tint * tint);
        GLint colorLoc = glGetUniformLocation(shader->GetID(), "uColor");
        if (colorLoc != -1) {
            glUniform3f(colorLoc, finalTint.r, finalTint.g, finalTint.b);
        }
        return true;
    }

    void GraphicsSystemV2::DrawMesh(MeshHandle meshHandle) {
        Mesh* mesh = resourceManager.GetMesh(meshHandle);
        if (mesh) {
            mesh->Draw();
        }
    }

    void GraphicsSystemV2::BeginFrame() {
        glClearColor(0.2f, 0.3f, 0.4f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void GraphicsSystemV2::EndFrame() {
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    void GraphicsSystemV2::DrawText4(const std::string& fontKey,
        const std::string& text,
        float x, float y,
        float scale,
        const glm::vec3& color)
    {
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        text_.draw(fontKey, text, x, y, scale, color);

        glEnable(GL_DEPTH_TEST);
    }

    void GraphicsSystemV2::RenderImGui() {
        if (!window) return;

        // ========================================================================
        // FPS CALCULATION - Done here because this function is called once per frame
        // ========================================================================
        static auto lastFrameTime = std::chrono::high_resolution_clock::now();
        auto now = std::chrono::high_resolution_clock::now();
        float realDt = std::chrono::duration<float>(now - lastFrameTime).count();
        lastFrameTime = now;
        
        if (realDt > 0.001f && realDt < 1.0f) {  // Sanity check (ignore < 1ms or > 1s)
            float instantFPS = 1.0f / realDt;
            currentFPS = currentFPS * 0.9f + instantFPS * 0.1f;  // Smoothed
        }

        // ========================================================================
        // FPS DISPLAY - Draw LAST, right before swap, on top of everything
        // ========================================================================
        if (showFPS) {
            int fbWidth, fbHeight;
            glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
            
            // Ensure we're drawing to the default framebuffer
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glViewport(0, 0, fbWidth, fbHeight);
            
            // Update text renderer for screen size
            text_.setScreenSize(fbWidth, fbHeight);
            
            // Format FPS string
            char fpsText[32];
            snprintf(fpsText, sizeof(fpsText), "FPS: %.1f", currentFPS);
            
            // Position: top-right corner (moved left)
            float textX = static_cast<float>(fbWidth) - 400.0f;
            float textY = static_cast<float>(fbHeight) - 100.0f;
            
            // Draw with black color as requested
            DrawText4("Sans48", fpsText, textX, textY, 1.0f, glm::vec3(255.0f, 0.0f, 0.0f));
        }

        //    // Just swap - DON'T clear!
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    void GraphicsSystemV2::AssignMeshAndMaterial(MeshRenderer& mr, const std::string& spriteName) {
        // Use string comparisons instead of static unordered_map to avoid memory leak
        // Static containers never get freed, causing 8-byte leak reports

        if (spriteName == "wireframequad") {
            mr.mesh = resourceManager.GetMeshHandle("wireframequad");
            mr.material = resourceManager.GetMaterialHandle("wireframeq_mat");
        }
        else if (spriteName == "circle") {
            mr.mesh = resourceManager.GetMeshHandle("circle");
            mr.material = resourceManager.GetMaterialHandle("circle_mat");
        }
        else if (spriteName == "quad") {
            mr.mesh = resourceManager.GetMeshHandle("quad");
            mr.material = resourceManager.GetMaterialHandle("quad_mat");
        }
        else if (spriteName == "line") {
            mr.mesh = resourceManager.GetMeshHandle("line");
            mr.material = resourceManager.GetMaterialHandle("line_mat");
        }
        else {
            // Default case for textures
            mr.mesh = resourceManager.GetMeshHandle("quad");
            mr.material = defaultMaterial;  // Use defaultMaterial which supports tinting
        }
    }

    void GraphicsSystemV2::SetRenderTarget(GLuint fbo, int width, int height) {
        targetFBO = fbo;
        targetWidth = width;
        targetHeight = height;
        renderingToTarget = true;
    }

    void GraphicsSystemV2::ClearRenderTarget() {
        targetFBO = 0;
        targetWidth = 0;
        targetHeight = 0;
        renderingToTarget = false;
    }
} // namespace Framework
