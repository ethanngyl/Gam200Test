// -----------------------------------------------------------------------------
// MainMenuSystem.cpp
// -----------------------------------------------------------------------------

#include "Precompiled.h"
#include "MainMenuSystem.h"
#include "Component.h"           // Transform
#include "RenderComponents.h"    // Renderable
#include "Message.h"
#include "Core.h"

#include <GLFW/glfw3.h>
#include <iostream>

namespace Framework {

    // ---------- helpers ----------
    bool MainMenuSystem::PointInRect(float px, float py, float cx, float cy, float w, float h)
    {
        const float hw = 0.5f * w;
        const float hh = 0.5f * h;
        return (px >= cx - hw && px <= cx + hw && py >= cy - hh && py <= cy + hh);
    }

    void MainMenuSystem::MouseToWorld(GLFWwindow* window, float& outX, float& outY, float& outAspect)
    {
        outX = outY = 0.0f; outAspect = 1.0f;
        if (!window) return;

        int w = 1, h = 1;
        glfwGetFramebufferSize(window, &w, &h);
        if (w <= 0 || h <= 0) return;

        double mx, my;
        glfwGetCursorPos(window, &mx, &my);

        const float xNdc = static_cast<float>((mx / double(w)) * 2.0 - 1.0);
        const float yNdc = static_cast<float>(1.0 - (my / double(h)) * 2.0);

        outAspect = static_cast<float>(w) / static_cast<float>(h);
        outX = xNdc * outAspect;
        outY = yNdc;
    }

    // ---------- wiring ----------
    MainMenuSystem::MainMenuSystem() = default;

    void MainMenuSystem::SetEntityManager(EntityManager* em) { m_em = em; }
    void MainMenuSystem::SetGraphics(GraphicsSystemV2* gfx) { m_gfx = gfx; }
    void MainMenuSystem::SetWindow(GLFWwindow* win) { m_window = win; }

    // ---------- lifecycle ----------
    void MainMenuSystem::Initialize()
    {
        if (!m_em || !m_gfx) {
            LOG_ERROR("ERROR", "[MainMenu] ERROR: missing EntityManager or GraphicsSystemV2");
            return;
        }

        auto& rm = m_gfx->GetResourceManager();

        ShaderHandle shader = rm.LoadShader("shaders/basic.vert", "shaders/basic.frag", "default");
        if (!shader.IsValid()) {
            LOG_ERROR("ERROR", "[MainMenu] ERROR: failed to load default shader");
        }

        m_texPlay = rm.LoadTexture("assets/ui_play.png");
        m_texExit = rm.LoadTexture("assets/ui_exit.png");

        m_matPlay = rm.CreateMaterial("ui_play_mat", shader);
        if (auto* m = rm.GetMaterial(m_matPlay)) {
            m->blendMode = BlendMode::AlphaBlend;
            m->depthTest = false;
            m->depthWrite = false;
            m->cullBackFace = false;
            m->albedoTexture = m_texPlay;
            m->tint = glm::vec4(1.0f);
        }

        m_matExit = rm.CreateMaterial("ui_exit_mat", shader);
        if (auto* m = rm.GetMaterial(m_matExit)) {
            m->blendMode = BlendMode::AlphaBlend;
            m->depthTest = false;
            m->depthWrite = false;
            m->cullBackFace = false;
            m->albedoTexture = m_texExit;
            m->tint = glm::vec4(1.0f);
        }

        m_meshQuad = FindOrCreateQuad(rm);

        CreateMenuEntities();
        LOG_INFO("InFo", "[MainMenu] Initialized (images attached)");
    }

    void MainMenuSystem::Update(float /*dt*/)
    {
        if (!m_visible || !m_em || !m_window)
            return;

        float mx = 0.f, my = 0.f, aspect = 1.f;
        MouseToWorld(m_window, mx, my, aspect);

        const bool mouseDown = (glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS);
        const bool clicked = (mouseDown && !m_lastMouseDown);
        m_lastMouseDown = mouseDown;

        // --- PLAY ---
        if (m_em->HasComponent<Transform>(m_btnPlay) && m_em->HasComponent<Renderable>(m_btnPlay)) {
            auto& tr = m_em->GetComponent<Transform>(m_btnPlay);
            auto& r = m_em->GetComponent<Renderable>(m_btnPlay);
            const bool hov = PointInRect(mx, my, tr.position.x, tr.position.y, tr.scale.x, tr.scale.y);

            r.tint = hov ? glm::vec4(1.10f, 1.10f, 1.10f, 1.0f)
                : glm::vec4(1.00f, 1.00f, 1.00f, 1.0f);

            if (hov && clicked) {
                SetVisible(false);
                LOG_INFO("InFo", "[MainMenu] Play clicked -> hide menu");
            }
        }

        // --- EXIT ---
        if (m_em->HasComponent<Transform>(m_btnExit) && m_em->HasComponent<Renderable>(m_btnExit)) {
            auto& tr = m_em->GetComponent<Transform>(m_btnExit);
            auto& r = m_em->GetComponent<Renderable>(m_btnExit);
            const bool hov = PointInRect(mx, my, tr.position.x, tr.position.y, tr.scale.x, tr.scale.y);

            r.tint = hov ? glm::vec4(1.10f, 1.10f, 1.10f, 1.0f)
                : glm::vec4(1.00f, 1.00f, 1.00f, 1.0f);

            if (hov && clicked) {
                LOG_INFO("InFo", "[MainMenu] Exit clicked -> quit");
                Message q(Status::Quit);
                CORE->BroadcastMessage(&q);
            }
        }
    }

    void MainMenuSystem::SendEngineMessage(Message* /*message*/) {}

    void MainMenuSystem::SetVisible(bool v)
    {
        m_visible = v;
        if (!m_em) return;

        auto setVis = [&](Entity e, bool vis) {
            if (m_em->HasComponent<Renderable>(e)) {
                m_em->GetComponent<Renderable>(e).visible = vis;
            }
            };

        setVis(m_titleBar, v);
        setVis(m_btnPlay, v);
        setVis(m_btnExit, v);
    }

    // ---------- entities ----------
    void MainMenuSystem::CreateMenuEntities()
    {
        
        // Play Button 
        {
            m_btnPlay = m_em->CreateEntity();
            auto& tr = m_em->AddComponent<Transform>(m_btnPlay, Vector2D(0.0f, 0.20f));
            tr.scale = m_btnSize;

            auto& r = m_em->AddComponent<Renderable>(m_btnPlay);
            r.mesh = m_meshQuad;
            r.material = m_matPlay;
            r.layer = 0;
            r.orderInLayer = 1;
            r.visible = true;
            r.tint = glm::vec4(1.0f); 
        }

        // Exit Button
        {
            m_btnExit = m_em->CreateEntity();
            auto& tr = m_em->AddComponent<Transform>(m_btnExit, Vector2D(0.0f, -0.15f));
            tr.scale = m_btnSize;

            auto& r = m_em->AddComponent<Renderable>(m_btnExit);
            r.mesh = m_meshQuad;
            r.material = m_matExit;
            r.layer = 0;
            r.orderInLayer = 2;
            r.visible = true;
            r.tint = glm::vec4(1.0f);
        }

        m_visible = true;
    }

    MeshHandle MainMenuSystem::FindOrCreateQuad(ResourceManager& rm)
    {
        std::vector<float> verts = {
            -0.5f,-0.5f,0, 1,0,0, 0,0,
             0.5f,-0.5f,0, 0,1,0, 1,0,
             0.5f, 0.5f,0, 0,0,1, 1,1,
            -0.5f, 0.5f,0, 1,1,0, 0,1
        };
        std::vector<unsigned> idx = { 0,1,2, 2,3,0 };
        return rm.CreateMesh("ui_quad_local", verts, idx, GL_TRIANGLES, true);
    }

} // namespace Framework
