/*
===============================================================================
File:        ImGuiSystem.h
Author:      Ethan Ng, Jiahao Zhou, Sim Kah Yan
Email:       n.ethanyongle@digipen.edu, jiahao.zhou@digipen.edu, kahyan.sim@digipen.edu
Date:        2025-11-07
Contribution: 40%(Ethan), 50%(Jiahao), 10%(kahyan)
-------------------------------------------------------------------------------
ImGui editor/overlay system. Integrates Dear ImGui with GLFW/
OpenGL, draws ImGui editor UI, and bridges runtime actions (play/stop, open/save,
drag–drop, asset browser) to ECS and subsystems.

@brief ImGui editor/overlay declarations: menu bar, panels, level I/O, drag-drop,
       and play/stop handoff to subsystems.

Safety: Headers only declare interfaces; no heavy logic here. Guard pointers.

Modified: 2025-11-26
- Added SetupDockSpace() for docking system support
- Enables Game viewport to auto-fit window size

Modified: 2026-01-08
- Added Script Browser functionality for ScriptComponent
- Added GetLuaFilesInDirectory() for recursive .lua file search
- Added ShowScriptBrowserPopup() for script selection UI

*/


#pragma once
#include "Precompiled.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "AudioSystem.h"

namespace Framework {

    class EntitySpawner;
    class EntityManager;

    /*
    * @brief declare a structure to store the undo step information
    */

    //Define types of actions we can undo
    enum class UndoType {
        Transform,  // Moving, Scaling, Rotating
        Creation,   // Spawning a new entity
        Deletion    // Deleting an entity
    };

    struct UndoStep {
        UndoType type;          // What kind of action was this?
        Entity entity;          // Which entity was affected?

        // Data for Transform Undo
        Vector2D oldPosition;
        Vector2D oldScale;
        float oldRotation;

        // Data for Deletion Undo (To restore it, we save it as a temp file)
        std::string tempFilePath;
    };

    /**
     * @brief ImGui System - Provides visual debugging and entity inspection UI
     */
    class ImGuiSystem : public EngineSystem {
    public:
        ImGuiSystem();
        ~ImGuiSystem();

        void Initialize() override;
        void Update(float dt) override;
        void SendEngineMessage(Message* msg) override;

        //void UpdateUI();

        // Setup methods
        void SetWindow(GLFWwindow* win);

        void SetEntityManager(EntityManager* em);
        void SetEntitySpawner(EntitySpawner* spawner);

        // Call this ONCE per visual frame, BEFORE the physics loop
        void NewFrame();

        // Load/Save level from text file
        // jiahao
        bool OpenLevelFromTxt(const std::string& file, bool clearAll);
        bool SaveLevelToTxt(const std::string& file);

        void SetAudioSystem(AudioSystem* audio);
        void SetGraphicsSystem(GraphicsSystemV2* graphics);
        // Render ImGui (call after game rendering, before swap buffers)
        void Render();

        // Shutdown ImGui
        void Shutdown();

        void Enable() { enabled = true; }
        void Disable() { enabled = false; }
        bool IsEnabled() const { return enabled; }

        //File drag and drop support - jiahao
        void EnableFileDragAndDrop();
        void SetPlayerEntity(Entity player) { playerEntity = player; }
        // Call this to bind the framebuffer before game rendering
        void BeginGameRender();
        // Call this to unbind after game rendering
        void EndGameRender();
        // Get the framebuffer ID for external use
        GLuint GetViewportFBO() const { return viewportFBO; }
        int GetViewportWidth() const { return viewportWidth; }
        int GetViewportHeight() const { return viewportHeight; }
        bool IsRenderingToViewport() const {
            return renderToViewport &&
                enabled &&
                showGameViewport &&
                viewportFBO != 0 &&
                viewportWidth >= 100 &&
                viewportHeight >= 100;
        }
        void RequestToggle() { pendingToggle = true; }

        // ========================================================================
        // VIEWPORT INFORMATION ACCESS
        // ========================================================================

        /**
         * @brief Get the screen position of the viewport (top-left corner)
         */
        ImVec2 GetViewportPos() const { return m_viewportPos; }

        /**
         * @brief Get the size of the viewport in pixels
         */
        ImVec2 GetViewportSize() const { return m_viewportSize; }

        /**
         * @brief Check if mouse is currently hovering the viewport
         */
        bool IsViewportHovered() const { return m_isViewportHovered; }

        /**
         * @brief Check if viewport is currently focused
         */
        bool IsViewportFocused() const { return m_isViewportFocused; }


        //undo function - jiahao
        void PerformUndo();
        void RecordUndoStep(Entity entity);
        void RecordCreationStep(Entity entity);         // For Spawning
        void RecordDeletionStep(Entity entity);         // For Deleting

        //jiahao
        Framework::Vector2D EditorScreenWorld();

        bool IsAudioFile(const std::filesystem::path& path) const;
        bool IsAudioFileSupported(const std::filesystem::path& path, std::string& outExtension) const;
        bool AddAudioToJSON(const std::string& jsonPath, const std::string& audioPath);



    private:
        GLFWwindow* window;
        EntityManager* entityManager;
        EntitySpawner* entitySpawner;
        Entity playerEntity;
        Entity lastSpawnedEntity;
        //jiahao
        //the below 2 std::string are used to record the file path
        //in order to save and load level files
        std::string currentLevelPath;
        std::string openPath;

        //pending gsm state when loading lua
        std::string currentLuaLevelPath = "";
        int pendingLuaGsmState = -1;

        AudioSystem* audioSystem;
        GraphicsSystemV2* graphicsSystem;

        // UI Windows
        void ShowEntityInspector();
        void ShowSpawnerWindow();
        void ShowDebugWindow();
        void ShowDemoWindow();

        // ========================================================================
        // ADDED: DockSpace setup function
        // ========================================================================
        void SetupDockSpace();

        //Asset windows - jiahao
        bool showAssets = false;
        std::string selectedAssetPath = "";
        Framework::Entity selectedEntity{};

        // Script Browser - jiahao (2026-01-08)
        bool showScriptBrowser = false;
        std::string selectedScriptPath = "";
        Entity entityPendingScriptAssignment{};

        bool showAudioErrorPopup = false;
        std::string audioErrorMessage = "";

        //object picking - jiahao
        void UpdatePicking();
        Framework::Entity GetSelectedEntity() const {
            return selectedEntity;
        };

        //dragging state for object dragging - jiahao
        bool isDraggingEntity = false;
        bool isScalingEntity = false;
        bool isRotatingEntity = false;

        Framework::Entity draggingEntity{};
        Vector2D dragOffset;

        Vector2D scaleStartMouse;

        Vector2D scaleStartScale;

        float rotateStartAngle = 0.0f;
        float rotateStartRotation = 0.0f;

        void UpdateEntityDragging();



        void ShowAssetsWindow();
        void SetupDefaultDockLayout();
        void ShowPrefabWindow();
        void SpawnPrefabAtMouse(const std::string& prefabPath);

        /**
         * @brief Shows script browser popup for selecting Lua scripts
         * Displays all .lua files in assets/scripts/ directory with search functionality
         */
        void ShowScriptBrowserPopup();

        /**
         * @brief Recursively gets all .lua files in a directory
         * @param directory The directory to search (e.g., "assets/scripts/")
         * @return Vector of relative paths to .lua files
         */
        std::vector<std::string> GetLuaFilesInDirectory(const std::string& directory);

        //file drop - jiahao
        static void FileDropCallBack(GLFWwindow* window, int count, const char** paths);
        void OnFileDrop(int count, const char** paths);
        bool IsLevelFile(const std::filesystem::path& path) const;
        bool IsTextureFile(const std::filesystem::path& path) const;

        std::filesystem::path rootpath = "assets/";
        std::filesystem::path currentpath = "assets/";
        std::filesystem::path previouspath;

        // State
        bool showDemo;
        bool showEntityInspector;
        bool showSpawner;
        bool showDebug;
        bool showPrefabWindow;
        bool pendingToggle = false;
        //
        // Entity selectedEntity;
        std::string selectedPrefabPath;

        bool enabled;

        bool imguiInitialized = false;  // Track if ImGui was successfully initialized

        float frameTime;
        int entityCount;

        int currentPage = 0;           // Current page in entity inspector
        int entitiesPerPage = 20;

        // Game viewport framebuffer
        GLuint viewportFBO = 0;
        GLuint viewportTexture = 0;
        GLuint viewportRBO = 0;
        int viewportWidth = 1280;
        int viewportHeight = 720;
        bool showGameViewport = true;
        bool renderToViewport = true;

        void CreateViewportFramebuffer(int width, int height);
        void ResizeViewportFramebuffer(int width, int height);
        void DeleteViewportFramebuffer();
        void ShowGameViewport();

        // jiahao
        ImVec2 m_viewportPos = { 0.0f, 0.0f };   // Position of the game image on screen
        ImVec2 m_viewportSize = { 0.0f, 0.0f };  // Size of the game image
        bool m_isViewportHovered = false;        // Is mouse hovering the viewport?
        bool m_isViewportFocused = false;        // Is viewport focused?

        //undo step - jiahao
        std::vector<UndoStep> undoStack;

        //audio pop up window variables - jiahao
        bool showAudioNamePopup = false;
        char newAudioKeyBuffer[256] = "";
        std::filesystem::path pendingAudioPath;
    };

} // namespace Framework