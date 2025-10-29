/*
===============================================================================
File:        CollisionSystem.h
Author:      Jiahao Zhou
Co-Author:   Ethan Ng
Email:       jiahao.zhou@digipen.edu, n.ethanyongle@digipen.edu
Date:        2025-09-30
Contribution: 85%(Jiahao), 15%(Ethan)
-------------------------------------------------------------------------------
Test harness interface for running interactive collision cases.

Responsibilities:
- Initialize CollTest modes (circle–rect, rect–rect, circle–circle, point tests,
  triangle tests, bounds checks).
- (Before render system is done)Create a simple test scenes and routes input to test all collision functions.
- (After render system is done)Create ECS ...

Controls for:
- T: toggle test mode
- 1 to 0: pick a test mode (locked until reset)
- R: reset current test
- WASD: move the active subject/point

Notes:
- y-up coordinates; centered AABB rectangles.
- simple test scenes is Console-only output; no HUD required.

Safety:
- Guard against null input system/entity manager.
- Keep state flags (testActive, sceneReady, collidedLastFrame) consistent on reset.
===============================================================================
*/

#include "CollisionSystem.h"
#include "Message.h"
#include "Math/Vector2D.h"
#include "Shader.h"
#include "Mesh.h"
#include "MeshFactory.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "ECSEntityManager.h"

namespace Framework {

void CollisionSystem::Initialize()
{ 
  // inform system Do NOT create any shapes here—wait until a mode is chosen.
  testActive = false;
  sceneReady = false;
  mode = CollTest::None;
  collidedLastFrame = false;

  //test instructions for user in console, use it to test before render system is done
  //after render system is done, we are using checkECSCollisions() to test collisions in ECS
  //but still keep this for reference
  std::cout << "CollisionSystem: Initialized\n";
  std::cout << "[T] toggle Collision Test Mode. Type [1,2,3,4,5] choose test when active:\n"
               " 1) circle-rect  2) rect-rect  3) circle-circle  4) point-circle  5) point-rect\n"
               " 6) circle-triangle 7) rect-triangle\n"
               " 8) circle vs bounds  9) rect vs bounds  0) point vs bounds\n"
               " [R] reset from current test scene.\n WASD moves the active object.\n"
               "Press [T] to start the test!\n";
}

void CollisionSystem::Update(float dt)
{
    CheckECSCollisions();
    // If input system is not wired, do nothing
    if (!m_input) return;
    if (!Framework::CORE || !Framework::CORE->IsPlaying()) return;
    //check whether test mode is activated
    if (m_input->IsKeyPressed(KEY_T)) {
        testActive = !testActive;
        if (!testActive) {
            clearScene();        // fully reset when disabling
            std::cout << "[CollisionTest] DISABLED\n";
        }
        else {
            clearScene();        // enabled -> start fresh, wait for 1..5
            std::cout << "[CollisionTest] is ENABLED  press [1,2,3,4,5,6,7,8,9,0] to choose a test.\n";
        }
    }

    if (!testActive) return;

    //test instructions for user in console, use it to test before render system is done
    //after render system is done, we are using checkECSCollisions() to test collisions in ECS
    //but still keep this for reference
    // Allow picking a mode only when active
    /*if (mode == CollTest::None) {
        if (m_input->IsKeyPressed(KEY_1)) { mode = CollTest::CircleToRect;   setupScene(mode);  std::cout << "Mode: Circle to Rect\n"; }
        else if (m_input->IsKeyPressed(KEY_2)) { mode = CollTest::RectToRect;     setupScene(mode);  std::cout << "Mode: Rect to Rect\n"; }
        else if (m_input->IsKeyPressed(KEY_3)) { mode = CollTest::CircleToCircle; setupScene(mode);  std::cout << "Mode: Circle to Circle\n"; }
        else if (m_input->IsKeyPressed(KEY_4)) { mode = CollTest::PointToCircle;  setupScene(mode);  std::cout << "Mode: Point to Circle (WASD moves point)\n"; }
        else if (m_input->IsKeyPressed(KEY_5)) { mode = CollTest::PointToRect;    setupScene(mode);  std::cout << "Mode: Point to Rect (WASD moves point)\n"; }
        else if (m_input->IsKeyPressed(KEY_6)) { mode = CollTest::TriCircle;      setupScene(mode);  std::cout << "Mode: Circle to Triangle\n"; }
        else if (m_input->IsKeyPressed(KEY_7)) { mode = CollTest::TriRect;        setupScene(mode);  std::cout << "Mode: Rect to Triangle\n"; }
        else if (m_input->IsKeyPressed(KEY_8)) { mode = CollTest::BoundsCircle;   setupScene(mode); std::cout << "Mode: Circle vs Bounds\n"; }
        else if (m_input->IsKeyPressed(KEY_9)) { mode = CollTest::BoundsRect;     setupScene(mode); std::cout << "Mode: Rect vs Bounds\n"; }
        else if (m_input->IsKeyPressed(KEY_0)) { mode = CollTest::BoundsPoint;    setupScene(mode); std::cout << "Mode: Point vs Bounds\n"; }
        return;
    }*/

    // Mode is locked now. Ignore further 1..5 until reset.
    if (m_input->IsKeyPressed(KEY_1) || m_input->IsKeyPressed(KEY_2) ||
        m_input->IsKeyPressed(KEY_3) || m_input->IsKeyPressed(KEY_4) ||
        m_input->IsKeyPressed(KEY_5) || m_input->IsKeyPressed(KEY_6) || 
        m_input->IsKeyPressed(KEY_7) || m_input->IsKeyPressed(KEY_8) || 
        m_input->IsKeyPressed(KEY_9) || m_input->IsKeyPressed(KEY_0))
    {
        std::cout << "[Info] A test is already running. Press [R] to reset, then pick a new mode.\n";
    }

    // Reset current test and go back to waiting for 1 to 7
    if (m_input->IsKeyPressed(KEY_R)) {
        clearScene();
        std::cout << "[Reset] Cleared. Press [1,2,3,4,5,6,7] to start a new test.\n";
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

		//base on the mode it activated, move the active object and check collisions
        //after render system is done, we are using checkECSCollisions() to test collisions in ECS
        //but still keep this for reference
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

        case CollTest::TriCircle: {
            if (dx || dy) {
                circle.position.x += dx; circle.position.y += dy;
                std::cout << "Circle -> (" << circle.position.x << ", " << circle.position.y << ")\n";
            }
            bool hit = circle_to_triangle(circle, sTriangle);
            if (hit) std::cout << "Hit Triangle\n";
            collidedLastFrame = hit;
        } break;

        
        case CollTest::TriRect: {
            if (dx || dy) {
                rect.position.x += dx; rect.position.y += dy;
                std::cout << "Rect -> (" << rect.position.x << ", " << rect.position.y << ")\n";
            }
            bool hit = rect_to_triangle(rect, sTriangle);
            if (hit) std::cout << "Hit Triangle\n";
            collidedLastFrame = hit;
        } break;

        case CollTest::BoundsCircle: {
            if (dx || dy) {
                circle.position.x += dx; circle.position.y += dy;
                std::cout << "Circle -> (" << circle.position.x << ", " << circle.position.y << ")\n";
            }
            bool outNow = circle_out_of_bounds(circle, world);
            //std::cout << outNow;
            if (outNow) std::cout << "OUT OF BOUNDS\n";
            if (!outNow) std::cout << "Back inside bounds\n";
            collidedLastFrame = outNow;
        } break;

        case CollTest::BoundsRect: {
            if (dx || dy) {
                rect.position.x += dx; rect.position.y += dy;
                std::cout << "Rect -> (" << rect.position.x << ", " << rect.position.y << ")\n";
            }
            bool outNow = rect_out_of_bounds(rect, world);
            if (outNow) std::cout << "OUT OF BOUNDS\n";
            if (!outNow) std::cout << "Back inside bounds\n";
            collidedLastFrame = outNow;
        } break;

        case CollTest::BoundsPoint: {
            if (dx || dy) {
                point.x += dx; point.y += dy;
                std::cout << "Point -> (" << point.x << "," << point.y << ")\n";
            }
            bool outNow = point_out_of_bounds(point, world);
            if (outNow) std::cout << "OUT OF BOUNDS\n";
            if (!outNow) std::cout << "Back inside bounds\n";
            collidedLastFrame = outNow;
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

// Simple console logging of a collider's type, position, and size
void CollisionSystem::printCollider(const char* name, const Collider& c)
{
  if (c.shapeType == ShapeType::Circle) {
    std::cout << name << " (Circle) pos=(" << c.position.x << "," << c.position.y
              << ") r=" << c.circle.radius << "\n";
  } else if (c.shapeType == ShapeType::Rect) {
    std::cout << name << " (Rect)   pos=(" << c.position.x << "," << c.position.y
              << ") w=" << c.rect.width << " h=" << c.rect.height << "\n";
  }
  else {
      std::cout << name << " (Triangle)/n"
          << " v0=(" << c.triangle.v0.x << "," << c.triangle.v0.y << ")\n"
          << " v1=(" << c.triangle.v1.x << "," << c.triangle.v1.y << ")\n"
		  << " v2=(" << c.triangle.v2.x << "," << c.triangle.v2.y << ")\n";
  }
}

void CollisionSystem::CheckECSCollisions()
{
    if (!entityManager) return;
    // Get entities collider entities

    for (Entity e : entityManager->GetAllEntities()) {
        if (entityManager->HasComponent<Movement>(e)) {
            auto& mv = entityManager->GetComponent<Movement>(e);
            mv.blocked = false;
        }
    }

    std::vector<Entity> rects, triangles, circles;
    for (Entity e : entityManager->GetAllEntities())
    {
        if (entityManager->HasComponent<Transform>(e) &&
            entityManager->HasComponent<BoxCollider>(e))
        {
            rects.push_back(e);
        }

        if (entityManager->HasComponent<Transform>(e) &&
            entityManager->HasComponent<TriangleCollider>(e))
        {
            triangles.push_back(e);
        }

        if (entityManager->HasComponent<Transform>(e) &&
            entityManager->HasComponent<CircleCollider>(e))
        {
            circles.push_back(e);
        }
        
    }

    
    for (Entity rectEnt : rects)
    {
        // Check rect vs triangle collisions
        for (Entity triEnt : triangles)
        {
            auto& rectTransform = entityManager->GetComponent<Transform>(rectEnt);
            auto& rectColl = entityManager->GetComponent<BoxCollider>(rectEnt);


            auto& triTransform = entityManager->GetComponent<Transform>(triEnt);
            auto& triColl = entityManager->GetComponent<TriangleCollider>(triEnt);
            // Convert to collision system format
            Collider ecsRect = Collider::create_rect(
                rectColl.size.x /** rectTransform.scale.x*/,
                rectColl.size.y /** rectTransform.scale.y*/,
				rectTransform.position 
            );


            // Triangle vertices in world space
            Collider triCol = Collider::create_triangle(
                triTransform.position + triColl.v0,
                triTransform.position + triColl.v1,
                triTransform.position + triColl.v2
            );
            // Use your existing rect_to_triangle function
            if (rect_to_triangle(ecsRect, triCol))
            {
                std::cout << "Collision: Rect entity " << rectEnt.GetID()
                    << " hit Triangle entity " << triEnt.GetID() << "\n";

                if (entityManager->HasComponent<Movement>(rectEnt)) {
                    auto& mv = entityManager->GetComponent<Movement>(rectEnt);
                    mv.blocked = true;
                }
            }
        }

        for (Entity circEnt : circles)
        {
            auto& rectTransform = entityManager->GetComponent<Transform>(rectEnt);
            auto& rectColl = entityManager->GetComponent<BoxCollider>(rectEnt);
            auto& circTransform = entityManager->GetComponent<Transform>(circEnt);
            auto& circColl = entityManager->GetComponent<CircleCollider>(circEnt);

            // Convert to collision system format
            Collider ecsRect = Collider::create_rect(
                rectColl.size.x,
                rectColl.size.y,
                rectTransform.position
            );

            Collider ecsCircle = Collider::create_circle(
                circColl.radius /** circTransform.scale.x*/,  // Scale the radius
                circTransform.position + circColl.offset
            );

            // Use your existing check_collision function
            if (check_collision(ecsCircle, ecsRect))
            {
                std::cout << "Collision: Rect entity " << rectEnt.GetID()
                    << " hit Circle entity " << circEnt.GetID() << "\n";

                if (entityManager->HasComponent<Movement>(rectEnt)) {
                    auto& mv = entityManager->GetComponent<Movement>(rectEnt);
                    mv.blocked = true;
                }
            }
        }

        for (Entity circEnt : circles)
        {
            // Build circle collider (radius is truth; no /2 hack)
            auto& cT = entityManager->GetComponent<Transform>(circEnt);
            auto& cC = entityManager->GetComponent<CircleCollider>(circEnt);
            Collider ecsCircle = Collider::create_circle(
                cC.radius,
                cT.position + cC.offset
            );

            // Test this circle against every triangle
            for (Entity triEnt : triangles)
            {
                auto& tT = entityManager->GetComponent<Transform>(triEnt);
                auto& tC = entityManager->GetComponent<TriangleCollider>(triEnt);

                Collider triCol = Collider::create_triangle(
                    tT.position + tC.v0,
                    tT.position + tC.v1,
                    tT.position + tC.v2
                );

                if (circle_to_triangle(ecsCircle, triCol)) {
                    std::cout << "Collision: Circle entity " << circEnt.GetID()
                        << " hit Triangle entity " << triEnt.GetID() << "\n";

                    if (entityManager->HasComponent<Movement>(circEnt)) {
                        auto& mv = entityManager->GetComponent<Movement>(circEnt);
                        mv.blocked = true;
                    }
                }
            }
        }
    }
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
