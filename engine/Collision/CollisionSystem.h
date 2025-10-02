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
- Declare CollTest modes (circle–rect, rect–rect, circle–circle, point tests,
  triangle tests, bounds checks).
- (Before render system is done)Create a simple test scenes and routes input to test all collision functions.
- (After render system is done)Create ECS ...

Controls for(runtime, handled in .cpp):
- T: toggle test mode
- 1 to 0: pick a test mode (locked until reset)
- R: reset current test
- WASD: move the active subject/point

Notes:
- y-up coordinates; centered AABB rectangles.
- Console-only output; no HUD required.

Safety:
- No dynamic allocations here; data lives on the system object.
===============================================================================
*/
#pragma once
#include "Interface.h"
#include "Collision.h"
#include <iostream>
#include "Input.h"
#include "Math/Vector2D.h"


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
    CollisionSystem() : entityManager(nullptr) { };
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
    EntityManager* entityManager{ nullptr };
    CollTest  mode{ CollTest::None };

    
    void printCollider(const char* name, const Collider& c);
    //void setupScene(CollTest m);      // spawn & place shapes for selected test
    void clearScene();                // despawn (logically) and unlock mode
  };

} // namespace Framework
