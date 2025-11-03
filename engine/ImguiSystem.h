/**
===============================================================================
 File:           ImGuiSystem.h
 Description:    ImGui integration for visual debugging and entity inspection
===============================================================================
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
        // Setup methods
        void SetWindow(GLFWwindow* win);
        
        void SetEntityManager(EntityManager* em);
        void SetEntitySpawner(EntitySpawner* spawner);

		// Load/Save level from text file
        // jiahao
        bool OpenLevelFromTxt(const std::string& file, bool clearAll);
		bool SaveLevelToTxt(const std::string& file);

        void SetAudioSystem(AudioSystem* audio);
        // Render ImGui (call after game rendering, before swap buffers)
        void Render();

        // Shutdown ImGui
        void Shutdown();

        void Enable() { enabled = true; }
        void Disable() { enabled = false; }
        bool IsEnabled() const { return enabled; }

		//File drag and drop support - jiahao
        void EnableFileDragAndDrop();

    private:
        GLFWwindow* window;
        EntityManager* entityManager;
        EntitySpawner* entitySpawner;

        //jiahao
        //the below 2 std::string are used to record the file path
		//in order to save and load level files
        std::string currentLevelPath;
        std::string openPath;
		std::string defaultLevelPath = "assets/defaultLevel.txt";

        AudioSystem* audioSystem;
        // UI Windows
        void ShowEntityInspector();
        void ShowSpawnerWindow();
        void ShowDebugWindow();
        void ShowDemoWindow();
        //Asset windows - jiahao
        bool showAssets = false;
		std::string selectedAssetPath = "";
        Framework::Entity selectedEntity{};
		void ShowAssetsWindow();

        //file drop - jiahao
		static void FileDropCallBack(GLFWwindow* window, int count, const char** paths);
		void OnFileDrop(int count, const char** paths);
        bool IsLevelFile(const std::filesystem::path& path) const;
		bool IsTextureFile(const std::filesystem::path& path) const;


        // State
        bool showDemo;
        bool showEntityInspector;
        bool showSpawner;
        bool showDebug;

        bool enabled;

        float frameTime;
        int entityCount;
    };

} // namespace Framework
