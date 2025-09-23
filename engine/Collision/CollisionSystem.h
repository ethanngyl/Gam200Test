#pragma once
#include "Interface.h"
#include "Collision.h"
#include <iostream>
#include "Input.h"


namespace Framework {

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
    Collider rectRight;
    Collider rectLeft;
    Collider rectTop;
    Collider rectBottom;

    float moveSpeed = 120.0f; //px per sec

    // Simple velocities for the demo
    Vec2 velCircle{ +30.0f, 0.0f };  // px/sec to the right
    Vec2 velRect  { -30.0f, 0.0f };  // px/sec to the left

    bool collidedLastFrame{false};
    InputSystem* m_input{ nullptr };
    //void move(Collider& c, const Vec2& v, float dt);
    void printCollider(const char* name, const Collider& c);
  };

} // namespace Framework
