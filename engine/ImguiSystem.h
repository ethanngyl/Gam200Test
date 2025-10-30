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
        void SetGraphicsSystem(GraphicsSystemV2* graphics);
        // Render ImGui (call after game rendering, before swap buffers)
        void Render();

        // Shutdown ImGui
        void Shutdown();

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
        GraphicsSystemV2* graphicsSystem;
        // UI Windows
        void ShowEntityInspector();
        void ShowSpawnerWindow();
        void ShowDebugWindow();
        void ShowDemoWindow();

        // State
        bool showDemo;
        bool showEntityInspector;
        bool showSpawner;
        bool showDebug;

        float frameTime;
        int entityCount;
    };

} // namespace Framework
