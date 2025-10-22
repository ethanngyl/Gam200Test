#pragma once
// ----------------------------------------------------------------------------
// MainMenuSystem.h
// Simple in-game main menu using existing engine systems (no engine changes).
// ----------------------------------------------------------------------------

#include "Interface.h"
#include "GraphicsSystemV2.h"
#include "ECSEntityManager.h"
#include "Vector2D.h"

struct GLFWwindow;

namespace Framework {

    class MainMenuSystem final : public EngineSystem
    {
    public:
        MainMenuSystem();

        // Wiring
        void SetEntityManager(EntityManager* em);
        void SetGraphics(GraphicsSystemV2* gfx);
        void SetWindow(GLFWwindow* win);

        // EngineSystem interface
        void Initialize() override;
        void Update(float dt) override;
        void SendEngineMessage(Message* message) override;

        // Menu control
        void SetVisible(bool visible);

    private:
        void CreateMenuEntities();
        MeshHandle FindOrCreateQuad(ResourceManager& rm);
        static bool PointInRect(float px, float py, float cx, float cy, float w, float h);
        static void MouseToWorld(GLFWwindow* window, float& outX, float& outY, float& outAspect);

    private:
        // Engine references
        EntityManager* m_em = nullptr;
        GraphicsSystemV2* m_gfx = nullptr;
        GLFWwindow* m_window = nullptr;

        // UI entities
        Entity m_titleBar;
        Entity m_btnPlay;
        Entity m_btnExit;

        // Resources
        MeshHandle     m_meshQuad;
        MaterialHandle m_matButton;
        MaterialHandle m_matHighlight;

        // State
        bool  m_visible = false;
        bool  m_lastMouseDown = false;

        // Button size
        Vector2D m_btnSize = Vector2D(0.8f, 0.25f);
    };

} // namespace Framework
