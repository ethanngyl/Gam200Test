// ----------------------------------------------------------------------------
// MainMenuSystem.cpp
// ----------------------------------------------------------------------------

#include "Precompiled.h"
#include "MainMenuSystem.h"
#include "Component.h"           // Transform
#include "RenderComponents.h"    // Renderable
#include "Message.h"
#include "Core.h"
#include <GLFW/glfw3.h>
#include <iostream>

namespace Framework {

    // ----------------------------------------------------------------------------
    // Utility functions
    // ----------------------------------------------------------------------------
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
        glfwGetWindowSize(window, &w, &h);
        if (w <= 0 || h <= 0) return;

        double mx, my;
        glfwGetCursorPos(window, &mx, &my);

        const float xNdc = static_cast<float>((mx / double(w)) * 2.0 - 1.0);
        const float yNdc = static_cast<float>(1.0 - (my / double(h)) * 2.0);

        outAspect = static_cast<float>(w) / static_cast<float>(h);
        outX = xNdc * outAspect;
        outY = yNdc;
    }

    // ----------------------------------------------------------------------------
    // MainMenuSystem methods
    // ----------------------------------------------------------------------------
    MainMenuSystem::MainMenuSystem() = default;

    void MainMenuSystem::SetEntityManager(EntityManager* em) { m_em = em; }
    void MainMenuSystem::SetGraphics(GraphicsSystemV2* gfx) { m_gfx = gfx; }
    void MainMenuSystem::SetWindow(GLFWwindow* win) { m_window = win; }

    void MainMenuSystem::Initialize()
    {

        if (!m_em || !m_gfx) {
            std::cerr << "[MainMenu] ERROR: missing EntityManager or GraphicsSystemV2\n";
            return;
        }

        auto& rm = m_gfx->GetResourceManager();
        ShaderHandle shader = rm.LoadShader("shaders/basic.vert", "shaders/basic.frag", "default");
        if (!shader.IsValid()) {
            std::cerr << "[MainMenu] ERROR: failed to load shader.\n";
        }

        // Create two materials (opaque, no depth)
        m_matButton = rm.CreateMaterial("ui_button_opaque", shader);
        if (auto* m = rm.GetMaterial(m_matButton)) {
            m->blendMode = BlendMode::Opaque;
            m->depthTest = false;
            m->depthWrite = false;
            m->cullBackFace = false;
        }

        m_matHighlight = rm.CreateMaterial("ui_highlight_opaque", shader);
        if (auto* m = rm.GetMaterial(m_matHighlight)) {
            m->blendMode = BlendMode::Opaque;
            m->depthTest = false;
            m->depthWrite = false;
            m->cullBackFace = false;
        }

        m_meshQuad = FindOrCreateQuad(rm);
        CreateMenuEntities();
        std::cout << "[MainMenu] Initialized.\n";
        std::cout << "[MainMenu] Initialize OK (visible=" << std::boolalpha << m_visible << ")\n";

    }

    void MainMenuSystem::Update(float /*dt*/)
    {
        static int frames = 0;
        if (++frames % 120 == 0) {
            std::cout << "[MainMenu] Update... visible=" << m_visible << "\n";
        }

        if (!m_visible || !m_em || !m_window)
            return;

        float mx = 0.f, my = 0.f, aspect = 1.f;
        MouseToWorld(m_window, mx, my, aspect);

        const bool mouseDown = (glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS);
        const bool clicked = (mouseDown && !m_lastMouseDown);
        m_lastMouseDown = mouseDown;

        // Play button
        if (m_em->HasComponent<Transform>(m_btnPlay) && m_em->HasComponent<Renderable>(m_btnPlay)) {
            auto& tr = m_em->GetComponent<Transform>(m_btnPlay);
            auto& r = m_em->GetComponent<Renderable>(m_btnPlay);
            const bool hov = PointInRect(mx, my, tr.position.x, tr.position.y, m_btnSize.x, m_btnSize.y);
            r.tint = hov ? glm::vec4(0.3f, 0.85f, 1.0f, 1.0f)
                : glm::vec4(0.2f, 0.75f, 0.9f, 1.0f);
            if (hov && clicked) {
                SetVisible(false);
                std::cout << "[MainMenu] Play clicked -> hide menu\n";
            }
        }

        // Exit button
        if (m_em->HasComponent<Transform>(m_btnExit) && m_em->HasComponent<Renderable>(m_btnExit)) {
            auto& tr = m_em->GetComponent<Transform>(m_btnExit);
            auto& r = m_em->GetComponent<Renderable>(m_btnExit);
            const bool hov = PointInRect(mx, my, tr.position.x, tr.position.y, m_btnSize.x, m_btnSize.y);
            r.tint = hov ? glm::vec4(0.3f, 1.0f, 0.5f, 1.0f)
                : glm::vec4(0.2f, 0.9f, 0.4f, 1.0f);
            if (hov && clicked) {
                std::cout << "[MainMenu] Exit clicked -> quit\n";
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

        setVis(m_btnPlay, v);
        setVis(m_btnExit, v);
        setVis(m_titleBar, v);
    }

    void MainMenuSystem::CreateMenuEntities()
    {
       

        // Play
        {
            m_btnPlay = m_em->CreateEntity();
            auto& tr = m_em->AddComponent<Transform>(m_btnPlay, Vector2D(0.0f, 0.20f));
            tr.scale = m_btnSize;

            auto& r = m_em->AddComponent<Renderable>(m_btnPlay);
            r.mesh = m_meshQuad;
            r.material = m_matButton;
            r.layer = 0;
            r.orderInLayer = 1;
            r.visible = true;
            r.tint = glm::vec4(0.20f, 0.75f, 0.90f, 1.0f);
        }

        // Exit
        {
            m_btnExit = m_em->CreateEntity();
            auto& tr = m_em->AddComponent<Transform>(m_btnExit, Vector2D(0.0f, -0.15f));
            tr.scale = m_btnSize;

            auto& r = m_em->AddComponent<Renderable>(m_btnExit);
            r.mesh = m_meshQuad;
            r.material = m_matButton;
            r.layer = 0;
            r.orderInLayer = 2;
            r.visible = true;
            r.tint = glm::vec4(0.20f, 0.90f, 0.40f, 1.0f);
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
