#pragma once
#include "Math/Vector2D.h"

/*struct Vec2 {
    float x{0.0f};
    float y{0.0f};
};*/

enum class ShapeType { Circle, Rect };

struct Circle { float radius {0.0f};};
struct Rect   { float width{0.0f}, height{0.0f};};

struct Collider {
    ShapeType shapeType{ShapeType::Circle};
    Framework::Vector2D position{};
    
    Circle circle{};
    Rect rect{};

    static Collider create_circle (float radius, Framework::Vector2D position){
        Collider c;
        c.shapeType = ShapeType::Circle;
        c.circle.radius = radius;
        c.position = position;
        return c;
    }
    static Collider create_rect (float width, float height, Framework::Vector2D position){
        Collider c;
        c.shapeType = ShapeType::Rect;
        c.rect.width = width;
        c.rect.height = height;
        c.position = position;
        return c;
    }
};

bool circle_to_circle (const Collider& a, const Collider& b);
bool rect_to_rect     (const Collider& a, const Collider& b);
bool circle_to_rect   (const Collider& circle, const Collider& rect);
//  click tests
bool point_in_circle  (const Framework::Vector2D point, const Collider& circle);
bool point_in_rect    (const Framework::Vector2D point, const Collider& rect);
bool point_in_collider(const Framework::Vector2D point, const Collider& c);

bool check_collision (const Collider& a, const Collider& b);
