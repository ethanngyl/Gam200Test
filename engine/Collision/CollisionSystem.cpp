#include "Precompiled.h"
#include "CollisionSystem.h"
#include "Message.h"

namespace Framework {

void CollisionSystem::Initialize()
{
  // Start them apart horizontally; y = 0
  circle = Collider::create_circle(/*radius*/ 20.0f, /*pos*/ Vec2{-150.0f, 0.0f});
  rectRight   = Collider::create_rect(/*w*/ 60.0f, /*h*/ 40.0f, /*pos*/ Vec2{+150.0f, 0.0f});
  rectLeft = Collider::create_rect(60.0f, 40.0f, Vec2{ -300.0f, 0.0f });
  rectTop = Collider::create_rect(60.0f, 40.0f, Vec2{ -150.0f,  150.0f });
  rectBottom = Collider::create_rect(60.0f, 40.0f, Vec2{ -150.0f,  -150.0f});

  std::cout << "CollisionSystem: Initialized (WASD to move circle; 4 static rects)\n";
  printCollider("Circle", circle);
  printCollider("RectRight", rectRight);
  printCollider("RectLeft", rectLeft);
  printCollider("RectTop", rectTop);
  printCollider("RectBottom", rectBottom);
}

void CollisionSystem::Update(float dt)
{
    // If input system is not wired, do nothing
    if (!m_input) return;
  // Move both toward each other
  //move(circle, velCircle, dt);
  //move(rect,   velRect,   dt);

      // 1) Continuous movement while keys are held (InputSystem already prints one-shot ¡°pressed¡± logs)
    float dx = 0.0f, dy = 0.0f;
    if (m_input->IsKeyDown(KEY_D)) dx += moveSpeed * dt;
    if (m_input->IsKeyDown(KEY_A)) dx -= moveSpeed * dt;
    if (m_input->IsKeyDown(KEY_W)) dy += moveSpeed * dt;
    if (m_input->IsKeyDown(KEY_S)) dy -= moveSpeed * dt;

    // 2) Apply movement to the circle only (rect is static)
    if (dx != 0.0f || dy != 0.0f) {
        circle.position.x += dx;
        circle.position.y += dy;

        // Print a line whenever the collider actually moved
        std::cout << "Circle collider moved to ("
                  << circle.position.x << ", " << circle.position.y << ")\n";
    }

  bool hitAny = false;

  if (check_collision(circle, rectRight)) {
      std::cout << "Colliding with RectRight... circle=(" << circle.position.x << ", " << circle.position.y
          << ") rect=(" << rectRight.position.x << ", " << rectRight.position.y << ")\n";
      hitAny = true;
  }
  if (check_collision(circle, rectLeft)) {
      std::cout << "Colliding with RectLeft... circle=(" << circle.position.x << ", " << circle.position.y
          << ") rect=(" << rectLeft.position.x << ", " << rectLeft.position.y << ")\n";
      hitAny = true;
  }
  if (check_collision(circle, rectTop)) {
      std::cout << "Colliding with RectTop... circle=(" << circle.position.x << ", " << circle.position.y
          << ") rect=(" << rectTop.position.x << ", " << rectTop.position.y << ")\n";
      hitAny = true;
  }
  if (check_collision(circle, rectBottom)) {
      std::cout << "Colliding with RectBottom... circle=(" << circle.position.x << ", " << circle.position.y
          << ") rect=(" << rectBottom.position.x << ", " << rectBottom.position.y << ")\n";
      hitAny = true;
  }
  /*if (hit && !collidedLastFrame) {
    std::cout << "[COLLISION START] circle <-> rect at positions: ";
    printCollider("Circle", circle);
    printCollider("Rect  ", rect);
  }
  else if (!hit && collidedLastFrame) {
    std::cout << "[COLLISION END]\n";
  }*/

  collidedLastFrame = hitAny;

  // Optional: quick click test example (replace with actual mouse later)
  // Here we just probe the midpoint between the two every ~0.5s visually/log-wise.
  // Vec2 probe{ 0.5f*(circle.position.x + rect.position.x), 0.0f };
  // if (point_in_collider(probe, circle) || point_in_collider(probe, rect)) { ... }
}

void CollisionSystem::SendMessage(Message* message)
{
  if (message->MessageId == Status::Quit) {
    std::cout << "CollisionSystem: Received quit message\n";
  }
}

/*void CollisionSystem::move(Collider& c, const Vec2& v, float dt)
{
  c.position.x += v.x * dt;
  c.position.y += v.y * dt;
}*/

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

} // namespace Framework
