#pragma once
// -----------------------------------------------------------------------------
// MainMenuSystem.h
// Simple main menu (Play / Exit) using existing engine systems only.
// Adds images to buttons via Material->albedoTexture, no engine changes.
// -----------------------------------------------------------------------------

#include "Interface.h"
#include "GraphicsSystemV2.h"
#include "ECSEntityManager.h"
#include "Vector2D.h"
#include "ResourceManager.h"   // for handles & loading

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

        // EngineSystem
        void Initialize() override;
        void Update(float dt) override;
        void SendEngineMessage(Message* message) override;

        // Show / hide the whole menu (keeps entities alive; only toggles visibility)
        void SetVisible(bool visible);

        // Optional: enable/disable click actions (kept off by default)
        void SetInteractive(bool on) { m_interactive = on; }

    private:
        void CreateMenuEntities();
        MeshHandle FindOrCreateQuad(ResourceManager& rm);

        static bool PointInRect(float px, float py, float cx, float cy, float w, float h);
        static void MouseToWorld(GLFWwindow* window, float& outX, float& outY, float& outAspect);

    private:
        // Engine refs
        EntityManager* m_em = nullptr;
        GraphicsSystemV2* m_gfx = nullptr;
        GLFWwindow* m_window = nullptr;

        // UI entities
        Entity m_titleBar;
        Entity m_btnPlay;
        Entity m_btnExit;

        // Resources
        MeshHandle     m_meshQuad;
        MaterialHandle m_matPlay;
        MaterialHandle m_matExit;
        MaterialHandle m_matTitle;

        TextureHandle  m_texPlay;
        TextureHandle  m_texExit;

        // State
        bool  m_visible = false;
        bool  m_lastMouseDown = false;
        bool  m_interactive = false;   // keep buttons non-functional by default

        // Layout
        Vector2D m_btnSize = Vector2D(0.9f, 0.28f);
    };

} // namespace Framework
