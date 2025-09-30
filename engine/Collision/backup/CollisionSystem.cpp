#include "CollisionSystem.h"
#include "Message.h"
#include "Math/Vector2D.h"
#include "Shader.h"
#include "Mesh.h"
#include "MeshFactory.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
//#include "input.h"

/*
===============================================================================
 CollisionSystem.cpp
------------------------------------------------------------------------------
 Implementation of the interactive collision test harness.

 Flow
   - Initialize: show key help, no shapes until a mode is chosen
   - Update:
       * If T toggled: enable/disable test mode (clears scene)
       * If active and no mode: lock to the first 1..5 pressed
       * If mode running:
           - WASD moves the active subject
           - Run collision checks and print results
       * R clears scene and unlocks mode selection

 Implementation notes
   - y-up coordinates (top = y + h/2)
   - Console output only (no on-screen HUD)
   

 Author: jiahao.zhou@digipen.edu
 Date:   2025-09-30
===============================================================================
*/

// ===== Collision Debug Draw (private to this .cpp) ===================
namespace {
    struct CollDebugGfx {
        Framework::Shader* shader = nullptr;
        Framework::Mesh* quad = nullptr;
        Framework::Mesh* tri = nullptr;
        Framework::Mesh* line = nullptr;
        Framework::Mesh* circle = nullptr;
        bool               ok = false;
    };

    CollDebugGfx& G() { static CollDebugGfx g; return g; }

    bool InitCollDebugGfx()
    {
        if (G().ok) return true;

        // Use the same shader paths your GraphicsSystem uses
        G().shader = new Framework::Shader("shaders/basic.vert", "shaders/basic.frag");
        G().quad = Framework::CreateQuad();
        G().tri = Framework::CreateTriangle();
        G().line = Framework::CreateLine();
        G().circle = Framework::CreateCircle(40, 0.5f);

        G().ok = (G().shader && G().quad && G().tri && G().line && G().circle);
        return G().ok;
    }

    // color as float rgb (0..1)
    void SetColor(float r, float g, float b)
    {
        GLint colorLoc = glGetUniformLocation(G().shader->GetID(), "uColor");
        glUniform3f(colorLoc, r, g, b);
    }

    void SetModel(const glm::mat4& M)
    {
        GLint modelLoc = glGetUniformLocation(G().shader->GetID(), "uModel");
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(M));
    }

    // Helpers assume your collision positions are in the same space your
    // GraphicsSystem uses (e.g., normalized -1..1). Adjust scale if needed.
    void DrawRect(float cx, float cy, float w, float h, float r, float g, float b, bool filled = false)
    {
        glm::mat4 M(1.0f);
        M = glm::translate(M, glm::vec3(cx, cy, 0.0f));
        M = glm::scale(M, glm::vec3(w * 0.5f, h * 0.5f, 1.0f)); // quad is [-0.5,0.5]
        SetModel(M);
        SetColor(r, g, b);
        G().quad->Draw();

        if (!filled) {
            // draw outline using 4 lines
            const float hw = w * 0.5f, hh = h * 0.5f;
            auto L = [&](float x1, float y1, float x2, float y2) {
                glm::mat4 ML(1.0f);
                // Build a line segment by translating/rotating/scaling the unit line
                // Simpler: draw the stored line mesh scaled + translated
                // The line mesh is from (-0.5,0) to (0.5,0): scale to length and rotate
                float dx = x2 - x1, dy = y2 - y1;
                float len = std::sqrt(dx * dx + dy * dy);
                float ang = std::atan2(dy, dx);
                ML = glm::translate(ML, glm::vec3(x1, y1, 0));
                ML = glm::rotate(ML, ang, glm::vec3(0, 0, 1));
                ML = glm::scale(ML, glm::vec3(len, 1.0f, 1.0f));
                SetModel(ML);
                G().line->Draw();
                };
            L(cx - hw, cy - hh, cx + hw, cy - hh);
            L(cx + hw, cy - hh, cx + hw, cy + hh);
            L(cx + hw, cy + hh, cx - hw, cy + hh);
            L(cx - hw, cy + hh, cx - hw, cy - hh);
        }
    }

    void DrawCircle(float cx, float cy, float radius, float r, float g, float b, bool filled = true)
    {
        glm::mat4 M(1.0f);
        M = glm::translate(M, glm::vec3(cx, cy, 0.0f));
        M = glm::scale(M, glm::vec3(radius, radius, 1.0f));
        SetModel(M);
        SetColor(r, g, b);
        G().circle->Draw(); // GL_TRIANGLE_FAN from MeshFactory
    }

    void DrawTriangle(float ax, float ay, float bx, float by, float cx, float cy,
        float r, float g, float b)
    {
        // We’ll reuse the triangle mesh and place it by building a model from its AABB.
        // Simpler: draw three lines between the points.
        auto L = [&](float x1, float y1, float x2, float y2) {
            glm::mat4 ML(1.0f);
            float dx = x2 - x1, dy = y2 - y1, len = std::sqrt(dx * dx + dy * dy), ang = std::atan2(dy, dx);
            ML = glm::translate(ML, glm::vec3(x1, y1, 0));
            ML = glm::rotate(ML, ang, glm::vec3(0, 0, 1));
            ML = glm::scale(ML, glm::vec3(len, 1.0f, 1.0f));
            SetModel(ML);
            G().line->Draw();
            };
        SetColor(r, g, b);
        L(ax, ay, bx, by);
        L(bx, by, cx, cy);
        L(cx, cy, ax, ay);
    }

    void DrawPoint(float x, float y, float r, float g, float b)
    {
        SetColor(r, g, b);
        DrawCircle(x, y, 0.01f, r, g, b, true); // tiny dot
    }
} // namespace

namespace Framework {

void CollisionSystem::Initialize()
{ 
  // inform system Do NOT create any shapes here—wait until a mode is chosen.
  testActive = false;
  sceneReady = false;
  mode = CollTest::None;
  collidedLastFrame = false;

  std::cout << "CollisionSystem: Initialized\n";
  std::cout << "[T] toggle Collision Test Mode. Type [1,2,3,4,5] choose test when active:\n"
               " 1) circle-rect  2) rect-rect  3) circle-circle  4) point-circle  5) point-rect\n"
               " [R] reset from current test scene.\n WASD moves the active object.\n"
               "Press [T] to start the test!\n";
}

void CollisionSystem::Update(float dt)
{
    // If input system is not wired, do nothing
    if (!m_input) return;

    //check whether test mode is activated
    if (m_input->IsKeyPressed(KEY_T)) {
        testActive = !testActive;
        if (!testActive) {
            clearScene();        // fully reset when disabling
            std::cout << "[CollisionTest] DISABLED\n";
        }
        else {
            clearScene();        // enabled -> start fresh, wait for 1..5
            std::cout << "[CollisionTest] is ENABLED  press [1,2,3,4,5] to choose a test.\n";
        }
    }

    if (!testActive) return;

    // Allow picking a mode only when active
    if (mode == CollTest::None) {
        if (m_input->IsKeyPressed(KEY_1)) { mode = CollTest::CircleToRect;   setupScene(mode);  std::cout << "Mode: Circle to Rect\n"; }
        else if (m_input->IsKeyPressed(KEY_2)) { mode = CollTest::RectToRect;     setupScene(mode);  std::cout << "Mode: Rect to Rect\n"; }
        else if (m_input->IsKeyPressed(KEY_3)) { mode = CollTest::CircleToCircle; setupScene(mode);  std::cout << "Mode: Circle to Circle\n"; }
        else if (m_input->IsKeyPressed(KEY_4)) { mode = CollTest::PointToCircle;  setupScene(mode);  std::cout << "Mode: Point to Circle (WASD moves point)\n"; }
        else if (m_input->IsKeyPressed(KEY_5)) { mode = CollTest::PointToRect;    setupScene(mode);  std::cout << "Mode: Point to Rect (WASD moves point)\n"; }

        return;
    }

    // Mode is locked now. Ignore further 1..5 until reset.
    if (m_input->IsKeyPressed(KEY_1) || m_input->IsKeyPressed(KEY_2) ||
        m_input->IsKeyPressed(KEY_3) || m_input->IsKeyPressed(KEY_4) ||
        m_input->IsKeyPressed(KEY_5))
    {
        std::cout << "[Info] A test is already running. Press [R] to reset, then pick a new mode.\n";
    }

    // Reset current test and go back to waiting for 1..5
    if (m_input->IsKeyPressed(KEY_R)) {
        clearScene();
        std::cout << "[Reset] Cleared. Press [1,2,3,4,5] to start a new test.\n";
        return;
    }

    // If somehow not ready yet, bail (defensive)
    if (!sceneReady) return;

        // Movement input (continuous) only affects the active test subject
        float dx = 0.0f, dy = 0.0f;
        if (m_input->IsKeyDown(KEY_D)) dx += moveSpeed * dt;
        if (m_input->IsKeyDown(KEY_A)) dx -= moveSpeed * dt;
        if (m_input->IsKeyDown(KEY_W)) dy += moveSpeed * dt;
        if (m_input->IsKeyDown(KEY_S)) dy -= moveSpeed * dt;

        switch (mode) {
        case CollTest::CircleToRect: {
            if (dx || dy) {
                circle.position.x += dx; circle.position.y += dy;
                std::cout << "Circle -> (" << circle.position.x << ", " << circle.position.y << ")\n";
            }
            bool hitAny = false;
            if (check_collision(circle, rectRight)) { std::cout << "Hit RectRight\n";  hitAny = true; }
            if (check_collision(circle, rectLeft)) { std::cout << "Hit RectLeft\n";   hitAny = true; }
            if (check_collision(circle, rectTop)) { std::cout << "Hit RectTop\n";    hitAny = true; }
            if (check_collision(circle, rectBottom)) { std::cout << "Hit RectBottom\n"; hitAny = true; }

            collidedLastFrame = hitAny;
        } break;

        case CollTest::RectToRect: {
            // Move rectLeft vs fixed rectB (or rectRight)
            if (dx || dy) {
                rect.position.x += dx; rect.position.y += dy;
                std::cout << "Rect -> (" << rect.position.x << ", " << rect.position.y << ")\n";
            }
            bool hitAny = false;
            if (check_collision(rect, rectRight)) { std::cout << "Hit RectRight\n";  hitAny = true; }
            if (check_collision(rect, rectLeft)) { std::cout << "Hit RectLeft\n";   hitAny = true; }
            if (check_collision(rect, rectTop)) { std::cout << "Hit RectTop\n";    hitAny = true; }
            if (check_collision(rect, rectBottom)) { std::cout << "Hit RectBottom\n"; hitAny = true; }
            
            collidedLastFrame = hitAny;
        } break;

        case CollTest::CircleToCircle: {
            if (dx || dy) {
                circle.position.x += dx; circle.position.y += dy;
                std::cout << "Circle -> (" << circle.position.x << ", " << circle.position.y << ")\n";
            }
            bool hitAny = false;
            if (check_collision(circle, circleRight)) { std::cout << "Hit CircleRight\n";  hitAny = true; }
            if (check_collision(circle, circleLeft)) { std::cout << "Hit CircleLeft\n";   hitAny = true; }
            if (check_collision(circle, circleTop)) { std::cout << "Hit CircleTop\n";    hitAny = true; }
			if (check_collision(circle, circleBottom)) { std::cout << "Hit CircleBottom\n"; hitAny = true; }
            collidedLastFrame = hitAny;
        } break;

        case CollTest::PointToCircle: {
            if (dx || dy) { point.x += dx; point.y += dy; std::cout << "Point -> (" << point.x << "," << point.y << ")\n"; }
            bool hitAny = false;
            if (point_in_circle(point, circleRight)) { std::cout << "Hit CircleRight\n";  hitAny = true; }
            if (point_in_circle(point, circleLeft)) { std::cout << "Hit CircleRight\n";  hitAny = true; }
            if (point_in_circle(point, circleTop)) { std::cout << "Hit CircleRight\n";  hitAny = true; }
            if (point_in_circle(point, circleBottom)) { std::cout << "Hit CircleRight\n";  hitAny = true; }
            collidedLastFrame = hitAny;
        } break;

        case CollTest::PointToRect: {
            if (dx || dy) { point.x += dx; point.y += dy; std::cout << "Point -> (" << point.x << "," << point.y << ")\n"; }
            bool hitAny = false;
            if (point_in_rect(point, rectRight)) { std::cout << "Hit RectRight\n";  hitAny = true; }
            if (point_in_rect(point, rectLeft)) { std::cout << "Hit RectRight\n";  hitAny = true; }
            if (point_in_rect(point, rectTop)) { std::cout << "Hit RectRight\n";  hitAny = true; }
            if (point_in_rect(point, rectBottom)) { std::cout << "Hit RectRight\n";  hitAny = true; }
            collidedLastFrame = hitAny;
        } break;

        case CollTest::None:
        default:
            // in test mode but no test chosen: do nothing
            break;
        }
       
        //collidedLastFrame = hitAny;
        return;
    
}


void CollisionSystem::SendEngineMessage(Message* message)
{
  if (message->MessageId == Status::Quit) {
    std::cout << "CollisionSystem: Received quit message\n";
  }
}

void CollisionSystem::printCollider(const char* name, const Collider& c)
{
  if (c.shapeType == ShapeType::Circle) {
    std::cout << name << " (Circle) pos=(" << c.position.x << "," << c.position.y
              << ") r=" << c.circle.radius << "\n";
  } else {
    std::cout << name << " (Rect)   pos=(" << c.position.x << "," << c.position.y
              << ") w=" << c.rect.width << " h=" << c.rect.height << "\n";
  }
}

// Create/place shapes for the chosen test (runs once per selection)
void CollisionSystem::setupScene(CollTest m)
{
    sceneReady = false;            // in case we early-exit

    switch (m) {
    case CollTest::CircleToRect: {
        circle = Collider::create_circle(20.0f, Vector2D{ -150.0f, 0.0f });
        rectRight = Collider::create_rect(60.0f, 40.0f, Vector2D{ 150.0f,   0.0f });
        rectLeft = Collider::create_rect(60.0f, 40.0f, Vector2D{ -300.0f,  0.0f });
        rectTop = Collider::create_rect(60.0f, 40.0f, Vector2D{ -150.0f, 150.0f });
        rectBottom = Collider::create_rect(60.0f, 40.0f, Vector2D{ -150.0f,-150.0f });
      
        printCollider("Circle", circle);
        printCollider("RectRight", rectRight);
        printCollider("RectLeft", rectLeft);
        printCollider("RectTop", rectTop);
        printCollider("RectBottom", rectBottom);
    } break;

    case CollTest::RectToRect: {
        rect = Collider::create_rect(60.0f, 40.0f, Vector2D{ -150.0f, 0.0f });
		rectRight = Collider::create_rect(60.0f, 40.0f, Vector2D{ 150.0f, 0.0f });
        rectLeft = Collider::create_rect(60.0f, 40.0f, Vector2D{ -300.0f, 0.0f });
		rectTop = Collider::create_rect(60.0f, 40.0f, Vector2D{ -150.0f, 150.0f });
		rectBottom = Collider::create_rect(60.0f, 40.0f, Vector2D{ -150.0f, -150.0f });
        printCollider("Rect", rect);
        printCollider("RectRight", rectRight);
        printCollider("RectLeft", rectLeft);
        printCollider("RectTop", rectTop);
        printCollider("RectBottom", rectBottom);
    } break;

    case CollTest::CircleToCircle: {
        circle = Collider::create_circle(20.0f, Vector2D{ -150.0f, 0.0f });
		circleLeft = Collider::create_circle(30.0f, Vector2D{ -270.0f, 0.0f });
        circleRight = Collider::create_circle(30.0f, Vector2D{ 120.0f, 0.0f });
		circleTop = Collider::create_circle(30.0f, Vector2D{ -150.0f, 120.0f });
		circleBottom = Collider::create_circle(30.0f, Vector2D{ -150.0f, -120.0f });
        printCollider("Circle", circle);
        printCollider("CircleRight", circleRight);
		printCollider("CircleLeft", circleLeft);
		printCollider("CircleTop", circleTop);
		printCollider("CircleBottom", circleBottom);

    } break;

    case CollTest::PointToCircle: {
        
        circleLeft = Collider::create_circle(30.0f, Vector2D{ -270.0f, 0.0f });
        circleRight = Collider::create_circle(30.0f, Vector2D{ 120.0f, 0.0f });
        circleTop = Collider::create_circle(30.0f, Vector2D{ -150.0f, 120.0f });
        circleBottom = Collider::create_circle(30.0f, Vector2D{ -150.0f, -120.0f });
        point = Vector2D{ -150.0f, 0.0f };
        printCollider("Circle", circle);
        printCollider("CircleRight", circleRight);
        printCollider("CircleLeft", circleLeft);
        printCollider("CircleTop", circleTop);
        printCollider("CircleBottom", circleBottom);
        std::cout << "Point starts at (-150,0).\n";
    } break;

    case CollTest::PointToRect: {
        rectRight = Collider::create_rect(60.0f, 40.0f, Vector2D{ 150.0f, 0.0f });
        rectLeft = Collider::create_rect(60.0f, 40.0f, Vector2D{ -300.0f, 0.0f });
        rectTop = Collider::create_rect(60.0f, 40.0f, Vector2D{ -150.0f, 150.0f });
        rectBottom = Collider::create_rect(60.0f, 40.0f, Vector2D{ -150.0f, -150.0f });
        point = Vector2D{ -150.0f, 0.0f };
        printCollider("RectRight", rectRight);
        printCollider("RectRight", rectRight);
        printCollider("RectLeft", rectLeft);
        printCollider("RectTop", rectTop);
        printCollider("RectBottom", rectBottom);
        std::cout << "Point starts at (-150,0).\n";
    } break;

    case CollTest::None:
    default:
        std::cout << "[setupScene] Invalid mode.\n";
        return;
    }

    collidedLastFrame = false;
    sceneReady = true;
}

// Clear current test and unlock mode selection
void CollisionSystem::clearScene()
{
    // Logically “despawn”: flip flags; objects will be rebuilt by setupScene().
    mode = CollTest::None;
    sceneReady = false;
    collidedLastFrame = false;
}


} // namespace Framework
