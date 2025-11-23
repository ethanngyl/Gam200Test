/*
===============================================================================
File:        ImGuiSystem.h
Author:      Ethan Ng, Jiahao Zhou, Sim Kah Yan
Email:       n.ethanyongle@digipen.edu, jiahao.zhou@digipen.edu, kahyan.sim@digipen.edu
Date:        2025-11-07
Contribution: 42%(Ethan), 53%(Jiahao), 5%(kahyan)
-------------------------------------------------------------------------------
ImGui editor/overlay system. Integrates Dear ImGui with GLFW/
OpenGL, draws ImGui editor UI, and bridges runtime actions (play/stop, open/save,
drag–drop, asset browser) to ECS and subsystems.

@brief ImGui editor/overlay declarations: menu bar, panels, level I/O, drag-drop,
       and play/stop handoff to subsystems.

Safety: Headers only declare interfaces; no heavy logic here. Guard pointers.

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

    struct UndoStep {
        //the entity that user select and need to undo
        Entity entity;          
		//the previous position of the entity before user move it
        Vector2D oldPosition;   
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

		//undo function - jiahao
		void PerformUndo();
        void RecordUndoStep(Entity entity);


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
        std::string defaultLevelPath = "assets/defaultLevel.txt";

        AudioSystem* audioSystem;
        GraphicsSystemV2* graphicsSystem;
        // UI Windows
        void ShowEntityInspector();
        void ShowSpawnerWindow();
        void ShowDebugWindow();
        void ShowDemoWindow();
        //Asset windows - jiahao
        bool showAssets = false;
        std::string selectedAssetPath = "";
        Framework::Entity selectedEntity{};

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

        //file drop - jiahao
        static void FileDropCallBack(GLFWwindow* window, int count, const char** paths);
        void OnFileDrop(int count, const char** paths);
        bool IsLevelFile(const std::filesystem::path& path) const;
        bool IsTextureFile(const std::filesystem::path& path) const;
        bool IsAudioFile(const std::filesystem::path& path) const;
        bool AddAudioToJSON(const std::string& audioName, const std::string& fileName);
        bool IsAudioFileSupported(const std::filesystem::path& path, std::string& errorMsg) const;
        std::filesystem::path rootpath = "assets/";
        std::filesystem::path currentpath = "assets/";
        std::filesystem::path previouspath;
        bool showAudioErrorPopup;
        std::string audioErrorMessage;
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


        //undo step - jiahao
		std::vector<UndoStep> undoStack;
    };

} // namespace Framework
