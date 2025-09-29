#include "CollisionSystem.h"
#include "Message.h"
#include "Math/Vector2D.h"
//#include "input.h"
namespace Framework {

void CollisionSystem::Initialize()
{ 
  // Do NOT create any shapes here—wait until a mode is chosen.
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
