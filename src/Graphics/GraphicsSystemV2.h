/*
===============================================================================
File:        GraphicsSystemV2.h
Author:      Sim Kah Yan
Email:       kahyan.sim@digipen.edu
Date:        2025-11-07
Contribution: 75%
-------------------------------------------------------------------------------
Brief:
Declaration of GraphicsSystemV2, a modern rendering system that manages the full
2D/3D draw pipeline: resource loading, camera control, render queue population,
sorting/batching, material binding, mesh drawing, debug visualization, and text.

Details:
- Owns a ResourceManager for shaders, textures, meshes, and materials.
- Maintains a Camera for view/projection and optional follow-target logic.
- Collects MeshRenderer data from the ECS, builds a RenderQueue each frame,
  sorts/batches, and executes Draw calls with minimal state switches.
- Provides a DebugRenderQueue for overlays (lines, circles, helpers).
- Integrates a FreeType-based TextRenderer for UI/HUD text.
- Supports a configurable background pass (mesh+texture+material).
- Exposes editor camera helpers (pan/zoom reset) and grid toggles.

Notes:
- Requires GLFW, GLEW/GL, GLM; assumes a valid context on Initialize().
- Must call SetWindow(...) before Initialize().
- Uses y-up convention and orthographic camera by default (configurable).
- Viewport size should be kept in sync via SetViewportSize() on resize.

Safety:
- All external pointers (GLFWwindow*, EntityManager*) are null-checked before use.
- Materials/shaders are bound through handles with current-bound caching.
- No ownership of EntityManager or GLFWwindow; they are provided by the engine.

===============================================================================
*/
#pragma once
#include "Interface.h"
#include "ResourceManager.h"
#include "Camera.h"
#include "RenderCommand.h"
#include "RenderComponents.h"
#include "Material.h"
#include "Precompiled.h"
#include <memory>
#include <unordered_map>   
#include "ECSEntity.h" 
#include "TextRenderer.h"
#include "MeshFactory.h"

// Forward declarations
struct GLFWwindow;

namespace Framework {
    class EntityManager;
    class Mesh;
    class InputSystem;

    /**
     * @class GraphicsSystemV2
     * @brief Modern graphics rendering system coordinating resources, camera,
     *        render queues, materials, meshes, debug draw, and text.
     *
     * Pipeline overview:
     * 1) Resource loading and caching (ResourceManager)
     * 2) Camera management (view/projection, editor helpers, follow target)
     * 3) Gather renderables into RenderQueue (from ECS MeshRenderer/Transform)
     * 4) Sort/batch commands to reduce state changes
     * 5) Execute draw calls (BindMaterial → DrawMesh)
     * 6) Overlay debug primitives and text (TextRenderer, DebugRenderQueue)
     */
    class GraphicsSystemV2 : public EngineSystem {
    public:
        // Construct with default state; window/entity manager are set externally.
        GraphicsSystemV2();

        // Release owned GPU/CPU resources and transient caches.
        virtual ~GraphicsSystemV2();

        // === CORE LIFECYCLE ===================================================
        
        // Initialize OpenGL state, default resources, meshes, materials, background.
        virtual void Initialize() override;

        // Per-frame: begin frame, gather+execute render queue, debug draw, end frame.
        virtual void Update(float dt) override;

        // Respond to engine messages (e.g., quit, reload, resize).
        virtual void SendEngineMessage(Message* message) override;

        // === SETUP ===
        
        /**
        * @brief Get TextRenderer for external use
        */
        TextRenderer& GetTextRenderer() { return text_; }

        void DrawText4(const std::string& fontKey, const std::string& text,
            float x, float y, float scale = 1.0f,
            const glm::vec3& color = glm::vec3(1, 1, 1));

        /**
         * @brief Set the rendering window
         * Must be called before Initialize()
         */
        void SetWindow(GLFWwindow* win);

        /**
         * @brief Assign mesh+material to a MeshRenderer based on a sprite name.
         *        Keeps legacy sprite-name flow working on the new material system.
         */
        void AssignMeshAndMaterial(MeshRenderer& mr, const std::string& spriteName);
        /**
         * @brief Set the entity manager for ECS integration
         */
        void SetEntityManager(EntityManager* em);
        void SetInputSystem(InputSystem* is);

        /**
         * @brief Set viewport size (handles window resize)
         */
        void SetViewportSize(int width, int height);

        // === RESOURCE MANAGEMENT ===
        
        /**
         * @brief Get the resource manager
         */
        ResourceManager& GetResourceManager() { return resourceManager; }

        // === CAMERA MANAGEMENT ===
        
        /**
         * @brief Get the main camera
         */
        Camera& GetCamera() { 
            
            return mainCamera;
        }

        Camera& GetEditorCamera() {
            return editorCamera;
		}

        /**
         * @brief Set camera position
         */
        void SetCameraPosition(const glm::vec3& position);

        /**
         * @brief Set camera zoom
         */
        void SetCameraZoom(float zoom);

        /**
         * @brief Set which entity the camera should follow
         */
        void SetFollowTarget(Entity e) { followTarget = e; followEnabled = true; }

        /**
         * @brief Stop following any entity
         */
        void ClearFollowTarget() { followEnabled = false; }


        //Editor Camera functions - Jiahao
        // Handle editor-style panning/zooming per-frame.
        void HandleEditorCamera(float dt);

        // Reset editor camera position/zoom to defaults.
        void ResetEditorCamera();

        // Access the built-in quad mesh handle (useful for UI/fullscreen passes).
        MeshHandle GetQuadMesh() const { return quadMesh; }

        // === DEBUG RENDERING ===
        
        /**
         * @brief Get debug render queue
         * Use this to add debug visualization
         */
        DebugRenderQueue& GetDebugQueue() { return debugQueue; }

        /**
         * @brief Enable/disable debug rendering
         */
        void SetDebugRenderingEnabled(bool enabled) { debugRenderingEnabled = enabled; }

        // === LEGACY SUPPORT ===
        
        /**
         * @brief Create default materials for legacy sprite names
         * Maintains backwards compatibility with old Sprite component
         */
        void RenderImGui();

        // Resolve a sprite name into a texture handle (legacy sprite-to-texture path).
        TextureHandle  GetTextureForSpriteName(const std::string& spriteName);

        // Toggle the grid overlay (visual guide).
        void SetShowGrid(bool e) { showGrid = e; }

        // Toggle the blocked-grid overlay/fill (e.g., nav blockers).
        void SetShowGridBlocked(bool e) { showGridBlocked = e; }

        // Set a custom render target (framebuffer)
        void SetRenderTarget(GLuint fbo, int width, int height);

        // Clear render target (return to default screen rendering)
        void ClearRenderTarget();

        // Check if currently rendering to a target
        bool IsRenderingToTarget() const { return renderingToTarget; }

        // Get current render dimensions
        int GetRenderWidth() const { return renderingToTarget ? targetWidth : viewportWidth; }
        int GetRenderHeight() const { return renderingToTarget ? targetHeight : viewportHeight; }

    private:
        // === RENDERING PHASES ===
        MeshFactory meshFactory;
        /**
         * @brief Gather all renderables and populate render queue
         */
        void GatherRenderCommands();

        /**
         * @brief Execute all render commands
         */
        void ExecuteRenderQueue();

        /**
         * @brief Render debug visualizations
         */
        void RenderDebugPrimitives();

        /**
         * @brief Clear frame buffers
         */
        void BeginFrame();

        /**
         * @brief Swap buffers and poll events
         */
        void EndFrame();

        // === RENDERING HELPERS ===
        
        /**
         * @brief Bind material and set uniforms
         */
        bool BindMaterial(MaterialHandle materialHandle, const glm::vec4& tint);

        /**
         * @brief Draw a mesh
         */
        void DrawMesh(MeshHandle meshHandle);

        /**
         * @brief Set up default rendering state
         */
        void SetupRenderState();

        // === INITIALIZATION HELPERS ===
        
        /**
         * @brief Initialize OpenGL context
         */
        void InitializeOpenGL();

        /**
         * @brief Load default resources
         */
        void LoadDefaultResources();

        /**
         * @brief Create default meshes
         */
        void CreateDefaultMeshes();

        /**
         * @brief Create default materials
         */
        void CreateDefaultMaterials();

        /**
         * @brief Create background rendering resources
         */
        void SetupBackground();

        // === MEMBER VARIABLES ===
        
        // Core systems
        GLFWwindow* window;
        EntityManager* entityManager;
        ResourceManager resourceManager;
        InputSystem* inputManager;

        // Camera
        Camera mainCamera;               // View/projection and zoom control.
        Camera editorCamera;

        // Render queues
        RenderQueue renderQueue;         // Batches of draw commands.
        DebugRenderQueue debugQueue;     // Overlay primitives.

        // Viewport
        int viewportWidth;
        int viewportHeight;

        // Default resources
        ShaderHandle defaultShader;
        ShaderHandle debugShader;
        MaterialHandle defaultMaterial;
        ShaderHandle Shader2;
        
        // Background rendering
        TextureHandle backgroundTexture;
        MeshHandle backgroundMesh;
        MaterialHandle backgroundMaterial;
        public:
        inline static MaterialHandle Material2;
        private:

        // Default primitive meshes
        MeshHandle quadMesh;
        MeshHandle lineMesh;
        MeshHandle circleMesh;
        MeshHandle wireframeQMesh;

        // Default materials for primitives
        MaterialHandle quadMaterial;
        MaterialHandle lineMaterial;
        MaterialHandle circleMaterial;
        MaterialHandle wireframeQMaterial;

        // Debug rendering
        bool debugRenderingEnabled;
        MeshHandle debugLineMesh;
        MeshHandle debugCircleMesh;

        // State tracking
        MaterialHandle currentBoundMaterial;         // Last bound material.
        ShaderHandle currentBoundShader;             // Last bound shader.
        TextRenderer text_;                          // FreeType text renderer

        /**
         * @brief Makes the camera follow a target player entity
         */
        void FollowPlayer(EntityManager* em, Entity player);
        void EditorCamDefaultControl(float dt);

        // Editor camera state
		glm::vec3 editorCameraStartPos{ 0.0f, 0.0f, 0.0f };
		float editorCameraZoom{ 1.0f };

        // Camera follow target
        Entity followTarget{ 0 };
        bool followEnabled{ false };

        // Statistics (for HUD/profiling/ImGui)
        struct RenderStats {
            size_t drawCalls = 0;
            size_t materialSwitches = 0;
        } stats;

        // Grid overlay toggles (editor visualization)
        bool showGrid = true; 
        bool showGridBlocked = true; 

        GLuint targetFBO = 0;
        int targetWidth = 0;
        int targetHeight = 0;
        bool renderingToTarget = false;
    };

} // namespace Framework
