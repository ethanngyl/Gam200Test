/**
===============================================================================
 File:           GraphicsSystemV2.h
 Author:         Graphics System Overhaul
 Date:           2025-10-07
 ------------------------------------------------------------------------------
 Brief:
 Complete overhaul of the graphics system with modern architecture.
 Implements resource management, material system, camera support,
 render queues, and efficient batching.

 Key Improvements:
 - Resource management with automatic caching
 - Material system for reusable rendering configs
 - Camera system with multiple viewport support
 - Render queue for efficient draw call sorting
 - Debug rendering utilities
 - Separation of concerns (rendering logic separated from ECS)
 - No hardcoded viewport size
 - Proper resource cleanup

 Migration from old system:
 - Old: GraphicsSystem
 - New: GraphicsSystemV2
 - Components: Sprite -> Renderable
 - Resources: Raw pointers -> Resource handles
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

    /**
     * @class GraphicsSystemV2
     * @brief Modern graphics rendering system
     * 
     * Manages the entire rendering pipeline:
     * 1. Resource loading and caching
     * 2. Camera management
     * 3. Render queue population
     * 4. Sorting and batching
     * 5. Draw call execution
     * 6. Debug visualization
     */
    class GraphicsSystemV2 : public EngineSystem {
    public:
        GraphicsSystemV2();
        virtual ~GraphicsSystemV2();

        // === CORE LIFECYCLE ===
        
        virtual void Initialize() override;
        virtual void Update(float dt) override;
        virtual void SendEngineMessage(Message* message) override;

        // === SETUP ===
        
        /**
         * @brief Set the rendering window
         * Must be called before Initialize()
         */
        void SetWindow(GLFWwindow* win);

        /**
         * @brief Set the entity manager for ECS integration
         */
        void SetEntityManager(EntityManager* em);

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
        Camera& GetCamera() { return mainCamera; }

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
        //void CreateLegacyMaterials();

        void RenderImGui();

        ///**
        // * @brief Get mesh for legacy sprite name
        // */
        //MeshHandle GetMeshForSpriteName(const std::string& spriteName);

        ///**
        // * @brief Get material for legacy sprite name
        // */
        //MaterialHandle GetMaterialForSpriteName(const std::string& spriteName);

        TextureHandle  GetTextureForSpriteName(const std::string& spriteName);


        void SetShowGrid(bool e) { showGrid = e; }
        void SetShowGridBlocked(bool e) { showGridBlocked = e; }

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

        // Draw grid to the debug queue (implemented in .cpp)
       // void RenderGridOverlay();  // NEW

        // === MEMBER VARIABLES ===
        
        // Core systems
        GLFWwindow* window;
        EntityManager* entityManager;
        ResourceManager resourceManager;

        // Camera
        Camera mainCamera;

        // Render queues
        RenderQueue renderQueue;
        DebugRenderQueue debugQueue;

        // Viewport
        int viewportWidth;
        int viewportHeight;

        // Default resources
        ShaderHandle defaultShader;
        ShaderHandle debugShader;
        MaterialHandle defaultMaterial;
        
        // Background rendering
        TextureHandle backgroundTexture;
        MeshHandle backgroundMesh;
        MaterialHandle backgroundMaterial;

        // Default primitive meshes
        MeshHandle triangleMesh;
        MeshHandle quadMesh;
        MeshHandle lineMesh;
        MeshHandle circleMesh;
        MeshHandle wireframeQMesh;

        // Default materials for primitives
        MaterialHandle triangleMaterial;
        MaterialHandle quadMaterial;
        MaterialHandle lineMaterial;
        MaterialHandle circleMaterial;
        MaterialHandle wireframeQMaterial;

        // Debug rendering
        bool debugRenderingEnabled;
        MeshHandle debugLineMesh;
        MeshHandle debugCircleMesh;

        //// Legacy support (for backwards compatibility)
        //std::unordered_map<std::string, MeshHandle> legacyMeshMap;
        //std::unordered_map<std::string, MaterialHandle> legacyMaterialMap;
        //std::unordered_map<std::string, TextureHandle>  
        // ;

        // State tracking
        MaterialHandle currentBoundMaterial;
        ShaderHandle currentBoundShader;
        TextRenderer text_;    // FreeType text renderer
        /**
         * @brief Makes the camera follow a target player entity
         */
        void FollowPlayer(EntityManager* em, Entity player);

        //Editor Camera functions - Jiahao
		void HandleEditorCamera(float dt);
		void ResetEditorCamera();

		glm::vec3 editorCameraStartPos{ 0.0f, 0.0f, 0.0f };
		float editorCameraZoom{ 1.0f };

        // Camera follow target (optional)
        Entity followTarget{ 0 };
        bool followEnabled{ false };

        // Statistics
        struct RenderStats {
            size_t drawCalls = 0;
            size_t trianglesRendered = 0;
            size_t materialSwitches = 0;
        } stats;

        // State: whether to show the grid overlay/fill
        bool showGrid = true;         // NEW
        bool showGridBlocked = true;  // NEW
    };

} // namespace Framework
