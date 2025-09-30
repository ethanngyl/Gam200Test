#pragma once
#include "Interface.h"
#include "Collision.h"
#include <iostream>
#include "Input.h"
#include "Math/Vector2D.h"

/*
===============================================================================
 CollisionSystem.h
------------------------------------------------------------------------------
 Test cases for collision functions with simple interactive modes.

 Controls
   - T: toggle test mode
   - 1,2,3,4,5,6,7,8,9,0: select mode (Circle-Rect, Rect-Rect, Circle-Circle, Point-Circle, Point-Rect)
   - R: reset current test
   - WASD: move active object/point

 Responsibilities
   - Spawn simple scenes for each test
   - Step movement from input and run collision checks
   - Track last-frame collision flag
   

 Notes
   - y-up math, centered AABBs
   - No dependency on debug HUD (console prints only)
   - For debugging / verification only


 Author: jiahao.zhou@digipen.edu
 Date:   2025-09-30
===============================================================================
*/

namespace Framework {
  

    enum class CollTest {
        None = 0,
        CircleToRect = 1,
        RectToRect = 2,
        CircleToCircle = 3,
        PointToCircle = 4,
        PointToRect = 5,
        TriCircle = 6,
        TriRect = 7,
        BoundsCircle = 8,
	    BoundsRect = 9,
        BoundsPoint = 10
    };
  // A tiny demo/test harness around your Collision utilities.
  class CollisionSystem : public InterfaceSystem
  {
  public:
    CollisionSystem() = default;
    ~CollisionSystem() override = default;

    void Initialize() override;
    void Update(float dt) override;
    void SendEngineMessage(Message* message) override;
    void CheckECSCollisions();
    void SetEntityManager(EntityManager* em) { entityManager = em; }
    void SetInput(InputSystem* input) { m_input = input; }
  private:
    // Demo scene: one circle and one rect move toward each other until they collide.
    Collider circle;
    Collider rect;
    Collider rectRight;
    Collider rectLeft;
    Collider rectTop;
    Collider rectBottom;
    Collider circleTop;
    Collider circleRight;
    Collider circleBottom;
    Collider circleLeft;
    Collider sTriangle;
    Framework::Vector2D point; // a movable point for point tests

    float moveSpeed = 120.0f; //px per sec
    bool autoMove{ false };                          // toggle with [M]
    Framework::Vector2D autoVel{ 120.0f, 90.0f };    // px/s
    Bounds world;  // {-320..320, -240..240} by default                                  // uses default {-320..320, -240..240}
    /* Simple velocities for the demo
    Vector2D velCircle{ +30.0f, 0.0f };  // px/sec to the right
    Vector2D velRect  { -30.0f, 0.0f };  // px/sec to the left*/

    bool collidedLastFrame{false};

    InputSystem* m_input{ nullptr };
    //void move(Collider& c, const Vec2& v, float dt);
    //test gate + current selection
    bool      testActive{ false };
    bool      sceneReady{ false };
    EntityManager* entityManager;
    CollTest  mode{ CollTest::None };

    
    void printCollider(const char* name, const Collider& c);
    void setupScene(CollTest m);      // spawn & place shapes for selected test
    void clearScene();                // despawn (logically) and unlock mode
  };

} // namespace Framework
