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
        void SetAudioSystem(AudioSystem* audio);
        // Render ImGui (call after game rendering, before swap buffers)
        void Render();

        // Shutdown ImGui
        void Shutdown();

        void SetPlayerEntity(Entity player) { playerEntity = player; }


    private:
        GLFWwindow* window;
        EntityManager* entityManager;
        EntitySpawner* entitySpawner;
        AudioSystem* audioSystem;
        Entity playerEntity;

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
