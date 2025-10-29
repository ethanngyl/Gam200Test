/**
===============================================================================
 File:           GraphicsSystemV2.cpp
 Author:         Graphics System Overhaul
 Date:           2025-10-07
 ------------------------------------------------------------------------------
 Brief:
 Implementation of the overhauled graphics system.
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

namespace Framework {

    static bool LooksLikeFilePath(const std::string& s) {
        const auto slash = s.find_last_of("/\\");
        const auto dot = s.find_last_of('.');
        return dot != std::string::npos && (slash == std::string::npos || dot > slash);
    }
    // === CONSTRUCTOR / DESTRUCTOR ===

    GraphicsSystemV2::GraphicsSystemV2()
        : window(nullptr)
        , entityManager(nullptr)
        , mainCamera(-2.0f, 2.0f, -1.0f, 1.0f, -1.0f, 1.0f)  // Orthographic: left, right, bottom, top, near, far
        , viewportWidth(1600)
        , viewportHeight(800)
        , debugRenderingEnabled(false)
        , currentBoundMaterial(INVALID_MATERIAL_HANDLE)
        , currentBoundShader(INVALID_SHADER_HANDLE)
    {
        std::cout << "GraphicsSystemV2: Constructor\n";
    }

    GraphicsSystemV2::~GraphicsSystemV2() {
        std::cout << "GraphicsSystemV2: Destructor - Cleaning up...\n";

        // Resource manager handles cleanup automatically
        resourceManager.Clear();
    }

    // === CORE LIFECYCLE ===

    void GraphicsSystemV2::Initialize() {
        std::cout << "\n========================================\n";
        std::cout << "  GraphicsSystemV2: Initializing\n";
        std::cout << "========================================\n\n";

        if (!window) {
            std::cerr << "ERROR: No window set! Call SetWindow() before Initialize()\n";
            return;
        }

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

        resourceManager.LoadFiles();// new

        glfwSwapInterval(1);  // ✅ ADD THIS - Enables VSync

        text_.init(viewportWidth, viewportHeight, "shaders/text.vert", "shaders/text.frag");

        text_.loadFont("Sans48", "assets/fonts/Roboto-Regular.ttf", 48);
        text_.loadFont("Serif32", "assets/fonts/NotoSerif-Bold.ttf", 32);
		//set the editor camera to default position - jiahao
		editorCameraStartPos = mainCamera.GetPosition();
		editorCameraZoom = mainCamera.GetZoom();

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

    void GraphicsSystemV2::FollowPlayer(EntityManager* em, Entity player)
    {
        if (!em || !em->HasComponent<Transform>(player))
            return;

        auto& playerTransform = em->GetComponent<Transform>(player);

        // Keep camera centered on player (XY plane)
        glm::vec3 targetPos(playerTransform.position.x, playerTransform.position.y, 0.0f);

        // Optional: smooth movement (camera lag)
        glm::vec3 currentPos = mainCamera.GetPosition();
        float smoothSpeed = 5.0f;  // adjust if needed
        glm::vec3 newPos = glm::mix(currentPos, targetPos, smoothSpeed * 0.016f);

        mainCamera.SetPosition(newPos);
    }


	//definition of ResetEditorCamera - Jiahao
    void GraphicsSystemV2::ResetEditorCamera() {
		mainCamera.SetPosition(editorCameraStartPos);
		mainCamera.SetZoom(editorCameraZoom);
    }

	//definition of HandleEditorCamera - Jiahao
    void GraphicsSystemV2::HandleEditorCamera(float dt) {
        const float panSpeed = 2.0f * dt;
		const float zoomSpeed = 1.5f * dt;

		//Panning with arrow keys
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
        if (GetAsyncKeyState(Framework::KEY_1)) {
            float zoom = mainCamera.GetZoom();
            mainCamera.SetZoom(zoom * (1.0f + zoomSpeed));
        }

        if (GetAsyncKeyState(Framework::KEY_2)) {
            float zoom = mainCamera.GetZoom();
            mainCamera.SetZoom(zoom * (1.0f - zoomSpeed));
        }

		//Reset camera position 

        if (GetAsyncKeyState(Framework::KEY_0)) {
            ResetEditorCamera();
        }
    }


    void GraphicsSystemV2::Update(float dt) {
        (void)dt;

        if (!window || glfwWindowShouldClose(window)) {
            return;
        }


        // === CAMERA FOLLOW LOGIC ===

        if (!Framework::CORE->IsPlaying()) {
			HandleEditorCamera(dt);
        }
        else {
            if (followEnabled && entityManager && followTarget.IsValid()) {
                FollowPlayer(entityManager, followTarget);
            }
        }


        // ============================================

        // Clear ONCE at the start
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        // Reset statistics
        stats = RenderStats();

        // Clear frame
        //BeginFrame();

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

        // Swap buffers
        //EndFrame();
        
        // === Text Rendering Pass ===
        text_.draw("Sans48", "Hello, StructSquad!", 30.f, viewportHeight - 60.f, 1.0f, { 1.0f, 1.0f, 1.0f });
        text_.draw("Serif32", "Score: 12345", viewportWidth - 250.f, 40.f, 1.0f, { 0.2f, 0.9f, 0.3f });

        // Clear queues for next frame
        renderQueue.Clear();
        debugQueue.Clear();

        // Check for OpenGL errors
        GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
            std::cerr << "OpenGL error in Update: 0x" << std::hex << error << std::dec << "\n";
        }
    }

    void GraphicsSystemV2::SendEngineMessage(Message* message) {
        if (message->MessageId == Status::Quit) {
            std::cout << "GraphicsSystemV2: Received quit message\n";
        }
    }

    // === SETUP ===

    void GraphicsSystemV2::SetWindow(GLFWwindow* win) {
        window = win;
    }

    void GraphicsSystemV2::SetEntityManager(EntityManager* em) {
        entityManager = em;
    }

    void GraphicsSystemV2::SetViewportSize(int width, int height) {
        viewportWidth = width;
        viewportHeight = height;

        glViewport(0, 0, width, height);

        // Update camera aspect ratio
        float aspectRatio = static_cast<float>(width) / static_cast<float>(height);
        mainCamera.SetOrthographic(-aspectRatio, aspectRatio, -1.0f, 1.0f);

        text_.setScreenSize(width, height);

        std::cout << "GraphicsSystemV2: Viewport resized to " << width << "x" << height << "\n";
    }

    void GraphicsSystemV2::SetCameraPosition(const glm::vec3& position) {
        mainCamera.SetPosition(position);
    }

    void GraphicsSystemV2::SetCameraZoom(float zoom) {
        mainCamera.SetZoom(zoom);
    }

    // === INITIALIZATION HELPERS ===

    void GraphicsSystemV2::InitializeOpenGL() {
        std::cout << "GraphicsSystemV2: Initializing OpenGL...\n";

        glfwMakeContextCurrent(window);

        // Initialize GLEW
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

    void GraphicsSystemV2::LoadDefaultResources() {
        std::cout << "GraphicsSystemV2: Loading default resources...\n";

        // Load default shader
        defaultShader = resourceManager.LoadShader(
            "shaders/basic.vert",
            "shaders/basic.frag",
            "default"
        );

        if (!defaultShader.IsValid()) {
            std::cerr << "ERROR: Failed to load default shader!\n";
        }

        // Load debug shader (could be the same as default for now)
        debugShader = defaultShader;
    }

    void GraphicsSystemV2::CreateDefaultMeshes() {
        std::cout << "GraphicsSystemV2: Creating default meshes...\n";

        // Create primitive meshes using the factory functions
        Mesh* triangle = CreateTriangle();
        Mesh* quad = CreateQuad();
        Mesh* line = CreateLine();
        Mesh* circle = CreateCircle(40, 0.5f);
        Mesh* wireframeQ = CreateWireframeQuad();

        // Register meshes with resource manager
        triangleMesh = resourceManager.CreateMesh("triangle",
            triangle->GetVertexCount() > 0 ? std::vector<float>() : std::vector<float>(),
            {}, GL_TRIANGLES, true);

        // Note: The current ResourceManager::CreateMesh needs the actual vertex data
        // For now, we'll create them directly. This should be refactored to accept Mesh*

        // Temporary workaround - store the created meshes
        // In a production system, you'd refactor CreateMesh to accept Mesh* or the factory would use ResourceManager

        // For now, just recreate them through the resource manager
        // Triangle vertices
        std::vector<float> triVerts = {
            0.0f,  0.5f, 0.0f,  1.0f, 0.0f, 0.0f,  0.5f, 1.0f,
           -0.5f, -0.5f, 0.0f,  0.0f, 1.0f, 0.0f,  0.0f, 0.0f,
            0.5f, -0.5f, 0.0f,  0.0f, 0.0f, 1.0f,  1.0f, 0.0f
        };
        triangleMesh = resourceManager.CreateMesh("triangle", triVerts, {}, GL_TRIANGLES, true);

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
            GL_LINE_STRIP,  // ✅ LINE_STRIP = connected lines
            true
        );

        // Clean up temporary meshes
        delete triangle;
        delete quad;
        delete line;
        delete circle;
        delete wireframeQ;
        std::cout << "Created " << 4 << " default meshes\n";
    }

    void GraphicsSystemV2::CreateDefaultMaterials() {
        std::cout << "GraphicsSystemV2: Creating default materials...\n";

        if (!defaultShader.IsValid()) {
            std::cerr << "ERROR: Cannot create materials without valid shader!\n";
            return;
        }

        // One Shot eveything method
        /* ALL MATERIALS.txt
        Default: defaultShader
        triangle_mat: shader2, triangleMaterial
        */

        // Only load when you need it method
        /* DefaultMaterial.txt
         param1
         param2
        */
        /* triangleMat.txt
         param1
         param2
        */

        /* DefaultScene
            Background Obj -> uses BackgrounMat, this position, has render component
            Obj1 -> uses Mat1, this position, has render component
            Obj2 -> ...
        */
        // Then WHen I See I need Mat1, I search if I loaded Mat1 already, if not, loads mat1 file


        // Create default material
        defaultMaterial = resourceManager.CreateMaterial("default", defaultShader);

        // Create materials for each primitive
        triangleMaterial = resourceManager.CreateMaterial("triangle_mat", defaultShader);
        auto* triMat = resourceManager.GetMaterial(triangleMaterial);
        if (triMat) {
            triMat->tint = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
        }

        quadMaterial = resourceManager.CreateMaterial("quad_mat", defaultShader);
        auto* quadMat = resourceManager.GetMaterial(quadMaterial);
        if (quadMat) {
            quadMat->tint = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
        }

        lineMaterial = resourceManager.CreateMaterial("line_mat", defaultShader);
        auto* lineMat = resourceManager.GetMaterial(lineMaterial);
        if (lineMat) {
            lineMat->tint = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
        }

        circleMaterial = resourceManager.CreateMaterial("circle_mat", defaultShader);
        auto* circleMat = resourceManager.GetMaterial(circleMaterial);
        if (circleMat) {
            circleMat->tint = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
        }

        wireframeQMaterial = resourceManager.CreateMaterial("wireframeq_mat", defaultShader);
        auto* wireframeQMat = resourceManager.GetMaterial(wireframeQMaterial);
        if (wireframeQMat) {
            wireframeQMat->tint = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
        }

        std::cout << "Created " << 5 << " default materials\n";
    }

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

    // Continued in next part...

} // namespace Framework
/**
===============================================================================
 File:           GraphicsSystemV2_Part2.cpp
 Author:         Graphics System Overhaul
 Date:           2025-10-07
 ------------------------------------------------------------------------------
 Brief:
 Second part of GraphicsSystemV2 implementation - rendering logic.

 NOTE: In production, merge Part1 and Part2 into single GraphicsSystemV2.cpp
===============================================================================
*/

namespace Framework {

    //// === LEGACY SUPPORT ===

    //void GraphicsSystemV2::CreateLegacyMaterials() {
    //    std::cout << "GraphicsSystemV2: Creating legacy material mappings...\n";

        // Map sprite names to meshes
        //legacyMeshMap["triangle"] = triangleMesh;
        //legacyMeshMap["quad"] = quadMesh;
        //legacyMeshMap["line"] = lineMesh;
        //legacyMeshMap["circle"] = circleMesh;
        //legacyMeshMap["wireframequad"] = wireframeQMesh;

        //// Map sprite names to materials
        //legacyMaterialMap["triangle"] = triangleMaterial;
        //legacyMaterialMap["quad"] = quadMaterial;
        //legacyMaterialMap["line"] = lineMaterial;
        //legacyMaterialMap["circle"] = circleMaterial;
        //legacyMaterialMap["wireframequad"] = wireframeQMaterial;

    //    std::cout << "Legacy material mappings created\n";
    //}

    //MeshHandle GraphicsSystemV2::GetMeshForSpriteName(const std::string& spriteName) {
    //    auto it = legacyMeshMap.find(spriteName);
    //    if (it != legacyMeshMap.end()) {
    //        return it->second;
    //    }
    //    return triangleMesh;  // Fallback
    //}

    //MaterialHandle GraphicsSystemV2::GetMaterialForSpriteName(const std::string& spriteName) {
    //    auto it = legacyMaterialMap.find(spriteName);
    //    if (it != legacyMaterialMap.end()) {
    //        return it->second;
    //    }
    //    return defaultMaterial;  // Fallback
    //}

    //TextureHandle GraphicsSystemV2::GetTextureForSpriteName(const std::string& spriteName) {
    //    auto it = legacyTextureMap.find(spriteName);
    //    if (it != legacyTextureMap.end())
    //        return it->second;

    //    // Try to load from file path (ResourceManager caches internally)
    //    TextureHandle th = resourceManager.LoadTexture(spriteName);
    //    if (th.IsValid()) {
    //        legacyTextureMap[spriteName] = th;
    //        return th;
    //    }

    //    return INVALID_TEXTURE_HANDLE;
    //}

    //MaterialHandle GraphicsSystemV2::GetMaterialForSpriteName(const std::string& spriteName) {
    //    auto it = legacyMaterialMap.find(spriteName);
    //    if (it != legacyMaterialMap.end()) {
    //        return it->second;
    //    }
    //    return defaultMaterial;  // Fallback
    //}

    TextureHandle GraphicsSystemV2::GetTextureForSpriteName(const std::string& name) {
        // Only try to load when it looks like a file path (e.g., "assets/x.png")
        if (LooksLikeFilePath(name)) {
            return resourceManager.EnsureTexture(name); // cache-aware; creates handle if missing
        }
        return INVALID_TEXTURE_HANDLE; // logical names like "quad" shouldn't bind a texture
    }

    // === RENDERING PHASES ===

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

        // --- Entity passes ---
        for (Entity e : entityManager->GetAllEntities()) {
            if (!entityManager->HasComponent<Transform>(e))
                continue;

            auto& transform = entityManager->GetComponent<Transform>(e);

            // prefer MeshRenderer over Sprite
            const bool hasRenderer = entityManager->HasComponent<MeshRenderer>(e);
            const bool hasSprite = entityManager->HasComponent<Sprite>(e);
            if (!hasRenderer && !hasSprite) continue;

            RenderCommand cmd;
            // Loads ONLY Renderer OR Sprite Data into draw Command
            // From Observstion, there are no Sprite Component using this currently
            if (hasRenderer) {
                auto& mr = entityManager->GetComponent<MeshRenderer>(e);
                if (!mr.visible) continue;

                // 4 Important Graphics Handles (Shader, Material, Mesh, Texture) [Shader is in Material]
                cmd.mesh = mr.mesh.IsValid() ? mr.mesh : quadMesh;
                cmd.material = mr.material.IsValid() ? mr.material : defaultMaterial;
                // ASC Note: I dun think you should be reading the sprite name here and then storing the tex id
                // Like, storing the mr.texture should be done way before or something
                // Okay maybe ask DX, becuase this might be the hot-loading part
                // But if it is hot-loading, then need check if spriteName name change
                if (mr.texture.IsValid()) {
                    cmd.texture = mr.texture;
                }
                else if (!mr.spriteName.empty()) {
                    TextureHandle tex = resourceManager.LoadTexture(mr.spriteName);
                    if (tex.IsValid()) {
                        cmd.texture = tex;
                        mr.texture = tex; // persist handle ✅
                    }
                }
                // To change Texture
                // mr.texture = resourceManager.LoadTexture("assets/name.png"); // Idealy, name.png is gotten from IMGUI


                // Other cmd command stuff
                cmd.tint = mr.tint;
                cmd.layer = mr.layer;
                cmd.orderInLayer = mr.orderInLayer;

            }
            else if (hasSprite) {
                auto& sp = entityManager->GetComponent<Sprite>(e);
                // 4 Important Graphics Handles (Shader, Material, Mesh, Texture) [Shader is in Material]
                cmd.mesh = quadMesh;
                cmd.material = defaultMaterial;
                if (!sp.texturePath.empty() && LooksLikeFilePath(sp.texturePath)) {
                    TextureHandle tex = resourceManager.LoadTexture(sp.texturePath);
                    cmd.texture = tex.IsValid() ? tex : INVALID_TEXTURE_HANDLE;
                }
                else {
                    cmd.texture = INVALID_TEXTURE_HANDLE;
                }
                // Other cmd command stuff
                cmd.tint = glm::vec4(1.0f);
                cmd.layer = sp.layer;

            }

            // === Transform to model matrix ===
            glm::mat4 model(1.0f);
            model = glm::translate(model, { transform.position.x, transform.position.y, 0.0f });
            model = glm::rotate(model, glm::radians(transform.rotation), { 0, 0, 1 });
            model = glm::scale(model, { transform.scale.x, transform.scale.y, 1.0f });
            cmd.modelMatrix = model;

            cmd.depth = glm::distance(
                glm::vec3(transform.position.x, transform.position.y, 0.0f),
                mainCamera.GetPosition()
            );

            renderQueue.Submit(cmd);
        }
    }

    void GraphicsSystemV2::ExecuteRenderQueue() {
        const auto& commands = renderQueue.GetCommands();

        if (commands.empty()) {
            return;
        }

        // Get camera matrices
        glm::mat4 projection = mainCamera.GetProjectionMatrix();
        glm::mat4 view = mainCamera.GetViewMatrix();

        // Reset state tracking
        currentBoundMaterial = INVALID_MATERIAL_HANDLE;
        currentBoundShader = INVALID_SHADER_HANDLE;

        // Execute each command
        for (const auto& cmd : commands) {
            if (!cmd.visible) {
                continue;
            }

            // Bind material if changed
            if (cmd.material != currentBoundMaterial) {
                // If Succeed, Bound new Material, Bound new Shader, Bound new Texture
                // ASC TA: For now, Remove the binding of material texture, and use binding of cmd.texture instead
                if (BindMaterial(cmd.material, cmd.tint))
                {
                    currentBoundMaterial = cmd.material;
                    stats.materialSwitches++;
                }
            }

            // Update shader uniforms
            Shader* shader = resourceManager.GetShader(currentBoundShader);
            if (shader) {
                // Set transformation matrices
                GLint modelLoc = glGetUniformLocation(shader->GetID(), "uModel");
                GLint projLoc = glGetUniformLocation(shader->GetID(), "uProjection");
                GLint viewLoc = glGetUniformLocation(shader->GetID(), "uView");

                if (modelLoc != -1) {
                    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(cmd.modelMatrix));
                }
                if (projLoc != -1) {
                    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
                }
                if (viewLoc != -1) {
                    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
                }

                //Texture* t;
                //if (cmd.texture.IsValid() && (t = resourceManager.GetTexture(cmd.texture))) {
                if (cmd.texture.IsValid()) {
                    // Either Use t->Bind(num), OR use glUniform1i method
                    //t->Bind(0);
                    GLint useTexLoc = glGetUniformLocation(shader->GetID(), "uUseTexture");
                    if (useTexLoc != -1) glUniform1i(useTexLoc, 1);
                    //GLint texLoc = glGetUniformLocation(shader->GetID(), "uTexture");
                    //if (texLoc != -1) glUniform1i(texLoc, 0);
                    glBindTextureUnit(0, cmd.texture.GetID());

                }
                else {
                    GLint useTexLoc = glGetUniformLocation(shader->GetID(), "uUseTexture");
                    if (useTexLoc != -1) glUniform1i(useTexLoc, 0);
                }
                
                // Draw mesh
                DrawMesh(cmd.mesh);
                stats.drawCalls++;
            }
        }

        // Unbind everything
        if (currentBoundShader.IsValid()) {
            Shader* shader = resourceManager.GetShader(currentBoundShader);
            if (shader) {
                shader->Unbind();
            }
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

        // Set shader uniforms
        //GLint useTexLoc = glGetUniformLocation(shader->GetID(), "uUseTexture");
        //if (useTexLoc != -1) {
        //    glUniform1i(useTexLoc, hasTexture ? 1 : 0);
        //}
        //
        //if (hasTexture) {
        //    GLint texLoc = glGetUniformLocation(shader->GetID(), "uTexture");
        //    if (texLoc != -1) {
        //        glUniform1i(texLoc, 0);
        //    }
        //}

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

    void GraphicsSystemV2::RenderImGui() {
        if (!window) return;

        // Just swap - DON'T clear!
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
} // namespace Framework