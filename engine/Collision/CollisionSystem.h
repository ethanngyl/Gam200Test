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
        PointToRect = 5
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
    Framework::Vector2D point; // a movable point for point tests

    float moveSpeed = 120.0f; //px per sec
    
    /* Simple velocities for the demo
    Vector2D velCircle{ +30.0f, 0.0f };  // px/sec to the right
    Vector2D velRect  { -30.0f, 0.0f };  // px/sec to the left*/

    bool collidedLastFrame{false};

    InputSystem* m_input{ nullptr };
    //void move(Collider& c, const Vec2& v, float dt);
    //test gate + current selection
    bool      testActive{ false };
    bool      sceneReady{ false };
    CollTest  mode{ CollTest::None };

    void printCollider(const char* name, const Collider& c);
    void setupScene(CollTest m);      // spawn & place shapes for selected test
    void clearScene();                // despawn (logically) and unlock mode
  };

} // namespace Framework
