/*
===============================================================================
File:        GraphicsSystemV2.cpp
Author:      Sim Kah Yan
Email:       kahyan.sim@digipen.edu
Date:        2025-11-07
Contribution: 100% (remaining of the code)
-------------------------------------------------------------------------------
Brief:
Implementation of GraphicsSystemV2, the modern rendering system that manages
resource loading, camera control (including editor utilities and follow target),
render queue gathering/sorting, batched and instancing draw execution, debug primitives, and
text rendering.

Details:
- Initializes OpenGL, loads default shaders/materials/meshes, and sets baseline
  render state (blend/cull/depth).
- Every frame: updates viewport/resolution, manages camera, gathers renderables
  from ECS (MeshRenderer/Sprite/SpriteAnimation), sorts/batches, binds material
  once per batch, and draws meshes efficiently.
- Provides debug overlay drawing (lines/circles/boxes) and simple background pass.
- Integrates a FreeType-based TextRenderer for HUD/UI text.

Notes:
- Expects SetWindow(...) to be called before Initialize().
- EntityManager is injected; this system does not own or create ECS data.
- Uses y-up, orthographic camera by default; view/projection are kept in sync
  with viewport size via SetViewportSize().

Safety:
- All external pointers are checked before use.
- ResourceManager handles lifetime for GPU resources via handles.
- Minimal per-frame allocations; debug rendering uses temporary meshes only
  for clarity (can be batched later for performance).
===============================================================================
*/
#include "Precompiled.h"
#include "GraphicsSystemV2.h"
#include "ECSEntityManager.h"
#include "Component.h"
#include "MeshFactory.h"
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Debugger/Trace.h"
#include "Input/Input.h"

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
        editorCameraStartPos = mainCamera.GetPosition();
        editorCameraZoom = mainCamera.GetZoom();

        // Load fonts (keys must match what DrawText uses)
        text_.loadFont("Sans48", "assets/Orbitron-VariableFont_wght.ttf", 48);
        text_.loadFont("Serif32", "assets/Roboto-VariableFont_wdth,wght.ttf", 32);

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
        glm::vec3 newPos = glm::mix(currentPos, targetPos, smoothSpeed * 0.016f);

        mainCamera.SetPosition(newPos);
    }

    void GraphicsSystemV2::EditorCamDefaultControl(float dt/*EntityManager* em, Entity player*/)
    {
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
        mainCamera.SetPosition(editorCameraStartPos);
        mainCamera.SetZoom(editorCameraZoom);
    }

    // ============================================================================
    // /definition of HandleEditorCamera- Jiahao
    // author: jiahao Zhou
    // Currently the editor camera share same class as main camera which is "Camera"
    // It is just in editor mode, the camera has different control method
    // so it still using mainCamera variable to represent editor camera
    // ============================================================================
    
    void GraphicsSystemV2::HandleEditorCamera(float dt) {
		// Define pan and zoom speeds
        const float panSpeed = 2.0f * dt;
        const float zoomSpeed = 1.5f * dt;

        //Panning with arrow keys
		//Basically move the camera position based on arrow key input
        if (GetAsyncKeyState(Framework::KEY_LEFT)) {
            mainCamera.Translate({ -panSpeed, 0.0f, 0.0f });
        }
        if (GetAsyncKeyState(Framework::KEY_RIGHT)) {
            mainCamera.Translate({ panSpeed, 0.0f, 0.0f });
        }
        if (GetAsyncKeyState(Framework::KEY_UP)) {
            mainCamera.Translate({ 0.0f, panSpeed, 0.0f });
        }
        if (GetAsyncKeyState(Framework::KEY_DOWN)) {
            mainCamera.Translate({ 0.0f, -panSpeed, 0.0f });
        }

        //Zooming 
		// key 1 to zoom in, key 2 to zoom out
        if (GetAsyncKeyState(Framework::KEY_1)) {
            float zoom = mainCamera.GetZoom();
            mainCamera.SetZoom(zoom * (1.0f + zoomSpeed));
        }

        if (GetAsyncKeyState(Framework::KEY_2)) {
            float zoom = mainCamera.GetZoom();
            mainCamera.SetZoom(zoom * (1.0f - zoomSpeed));
        }

        //Reset camera position 
		// key 0 to reset camera, and call function ResetEditorCamera
        if (GetAsyncKeyState(Framework::KEY_0)) {
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
            float aspectRatio = static_cast<float>(targetWidth) / static_cast<float>(targetHeight);
            mainCamera.SetOrthographic(-aspectRatio, aspectRatio, -1.0f, 1.0f);

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
            EditorCamDefaultControl(dt);
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
            float aspectRatio = static_cast<float>(fbWidth) / static_cast<float>(fbHeight);
            mainCamera.SetOrthographic(-aspectRatio, aspectRatio, -1.0f, 1.0f);

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
        float aspectRatio = static_cast<float>(width) / static_cast<float>(height);
        mainCamera.SetOrthographic(-aspectRatio, aspectRatio, -1.0f, 1.0f);
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
        Mesh* quad = meshFactory.CreateQuad();
        Mesh* line = meshFactory.CreateLine();
        Mesh* circle = meshFactory.CreateCircle(40, 0.5f);
        Mesh* wireframeQ = meshFactory.CreateWireframeQuad();

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

        // Clean up temporary meshes
        delete quad;
        delete line;
        delete circle;
        delete wireframeQ;
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
        Material2 = resourceManager.CreateMaterial("color", Shader2);
        // Create materials for each primitive
        // Quad material
        quadMaterial = resourceManager.CreateMaterial("quad_mat", defaultShader);
        auto* quadMat = resourceManager.GetMaterial(quadMaterial);
        if (quadMat) {
            quadMat->tint = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
        }
        // Line material
        lineMaterial = resourceManager.CreateMaterial("line_mat", defaultShader);
        auto* lineMat = resourceManager.GetMaterial(lineMaterial);
        if (lineMat) {
            lineMat->tint = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
        }
        // Circle material
        circleMaterial = resourceManager.CreateMaterial("circle_mat", defaultShader);
        auto* circleMat = resourceManager.GetMaterial(circleMaterial);
        if (circleMat) {
            circleMat->tint = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
        }
        // Wireframe quad material
        wireframeQMaterial = resourceManager.CreateMaterial("wireframeq_mat", defaultShader);
        auto* wireframeQMat = resourceManager.GetMaterial(wireframeQMaterial);
        if (wireframeQMat) {
            wireframeQMat->tint = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
        }

        std::cout << "Created " << 5 << " default materials\n";
    }

    // SetupBackground: Load texture, assign quad mesh, and create a background material.
    void GraphicsSystemV2::SetupBackground() {
        std::cout << "GraphicsSystemV2: Setting up background...\n";

        // Load background texture
        backgroundTexture = resourceManager.LoadTexture("assets/background.png");

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
        }

        std::cout << "Background setup complete\n";
    }

    // SetupRenderState: Global GL state for this renderer.
    void GraphicsSystemV2::SetupRenderState() {
        // Enable alpha blending
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // Disable depth testing for 2D rendering (can be enabled for 3D)
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
            bg.layer = -1000;
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

                    MaterialHandle inst = resourceManager.CreateMaterial(
                        "entity_mat_" + std::to_string(e.GetID()),
                        base->shader
                    );

                    Material* pm = resourceManager.GetMaterial(inst);
                    if (!pm) continue;

                    *pm = *base; // shallow copy of defaults
                    mr.material = inst;
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
            }

            // ---------- SPRITE ----------
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

                cmd.tint = glm::vec4(1.0f);
                cmd.layer = sp.layer;
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
            //   • Transformation Block
            //     Converts ECS Transform component data (position, rotation, scale) into a
            //     model matrix used by the GPU for world-space rendering.
            //     Each entity receives its own transform, enabling independent movement
            //     and scaling across the scene.
            //
            //   • UV Animation Block
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
            // Use layer for Z-depth to prevent Z-fighting (flickering textures)
            // Each layer gets 0.001 depth offset (e.g., layer 0 = 0.0, layer 1 = 0.001, etc.)
            float zDepth = cmd.layer * 0.001f;
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
            if (entityManager->HasComponent<SpriteAnimation>(e))
            {
                auto& anim = entityManager->GetComponent<SpriteAnimation>(e);

                Material* mat = resourceManager.GetMaterial(cmd.material);
                if (!mat) continue;

                Texture* tex = resourceManager.GetTexture(anim.spriteSheet);
                if (!tex) continue;

                const int texW = tex->GetWidth();
                const int texH = tex->GetHeight();
                if (texW <= 0 || texH <= 0 || anim.frameWidth <= 0 || anim.frameHeight <= 0)
                    continue;

                const int cols = texW / anim.frameWidth;
                const int frame = anim.currentFrame % max(1, anim.frameCount);
                const int x = frame % cols;
                const int y = frame / cols;

                float u0 = (x * anim.frameWidth) / float(texW);
                float u1 = ((x + anim.uvShrinkPx) * anim.frameWidth) / float(texW);
                float v1 = 1.0f - (y * anim.frameHeight) / float(texH);
                float v0 = 1.0f - ((y + anim.uvShrinkPx) * anim.frameHeight) / float(texH);

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
            }

            renderQueue.Submit(cmd);
        }
    }

    void GraphicsSystemV2::ExecuteRenderQueue() {
        const auto& commands = renderQueue.GetCommands();
        if (commands.empty()) return;
        //glEnable(GL_BLEND);
        //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // Camera matrices
        Camera& activeCamera = Framework::CORE->IsPlaying() ? mainCamera : editorCamera;
        glm::mat4 projection = activeCamera.GetProjectionMatrix();
        glm::mat4 view = activeCamera.GetViewMatrix();

        currentBoundMaterial = INVALID_MATERIAL_HANDLE;
        currentBoundShader = INVALID_SHADER_HANDLE;

        // ---- STEP 1: Group by (mesh + material + texture) ----
        struct Key {
            MeshHandle mesh;
            MaterialHandle mat;
            TextureHandle tex;
        };
        struct KeyHash {
            size_t operator()(const Key& k) const noexcept {
                return ((size_t)k.mesh.GetID() << 32) ^ (size_t)k.mat.GetID() ^ (size_t)k.tex.GetID();
            }
        };
        struct KeyEq {
            bool operator()(const Key& a, const Key& b) const noexcept {
                return a.mesh == b.mesh && a.mat == b.mat && a.tex == b.tex;
            }
        };

        std::unordered_map<Key, std::vector<glm::mat4>, KeyHash, KeyEq> batches;

        for (const auto& cmd : commands) {
            if (!cmd.visible) continue;
            batches[{cmd.mesh, cmd.material, cmd.texture}].push_back(cmd.modelMatrix);
        }

        // ---- STEP 2: Render each batch once ----
        for (auto& [key, matrices] : batches) {
            if (matrices.empty()) continue;

            // Bind material and shader once
            if (key.mat != currentBoundMaterial) {
                if (BindMaterial(key.mat, glm::vec4(1.0f))) { // tint uniform already handled
                    currentBoundMaterial = key.mat;
                    stats.materialSwitches++;
                }
            }

            Shader* shader = resourceManager.GetShader(currentBoundShader);
            if (!shader) continue;

            GLint projLoc = glGetUniformLocation(shader->GetID(), "uProjection");
            GLint viewLoc = glGetUniformLocation(shader->GetID(), "uView");
            if (projLoc != -1) glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
            if (viewLoc != -1) glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

            // Texture binding
            if (key.tex.IsValid()) {
                glBindTextureUnit(0, key.tex.GetID());
                GLint useTexLoc = glGetUniformLocation(shader->GetID(), "uUseTexture");
                if (useTexLoc != -1) glUniform1i(useTexLoc, 1);
            }
            else {
                GLint useTexLoc = glGetUniformLocation(shader->GetID(), "uUseTexture");
                if (useTexLoc != -1) glUniform1i(useTexLoc, 0);
            }

            // Get mesh
            Mesh* mesh = resourceManager.GetMesh(key.mesh);
            if (!mesh) continue;

            // ---- STEP 3: Upload instance data and draw ----
            mesh->SetInstanceData(); // sets up the VAO attributes
            // Upload matrices to GPU buffer (modern DSA version)
            glNamedBufferSubData(mesh->instanceVBO, 0,
                matrices.size() * sizeof(glm::mat4),
                matrices.data());

            // Draw once for all instances
            mesh->DrawInstanced(matrices, static_cast<GLsizei>(matrices.size()));

            stats.drawCalls++;
        }

        // Cleanup
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

    //    // Just swap - DON'T clear!
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    void GraphicsSystemV2::AssignMeshAndMaterial(MeshRenderer& mr, const std::string& spriteName) {
        static std::unordered_map<std::string, std::pair<std::string, std::string>> lookup = {
            {"wireframequad", {"wireframequad", "wireframeq_mat"}},
            {"circle", {"circle", "circle_mat"}},
            {"quad", {"quad", "quad_mat"}},
            {"line", {"line", "line_mat"}}
        };

        auto it = lookup.find(spriteName);
        if (it != lookup.end()) {
            mr.mesh = resourceManager.GetMeshHandle(it->second.first);
            mr.material = resourceManager.GetMaterialHandle(it->second.second);
        }
        else {
            mr.mesh = resourceManager.GetMeshHandle("quad");
            mr.material = Material2;
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