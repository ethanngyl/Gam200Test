#pragma once
#include "Math/Vector2D.h"

/*
===============================================================================
 Collision.h
------------------------------------------------------------------------------
 Collider shapes and test helpers for collision system.

 What¡¯s here
   - Data: ShapeType, Circle, Rect, Collider (centered AABB for rect)
   - Tests: circle_to_circle, rect_to_rect, circle_to_rect
            point_in_circle, point_in_rect, point_in_collider, check_collision
   - Bounds checks: circle_out_of_bounds, rect_out_of_bounds, point_out_of_bounds
   - Triangle support: point_in_triangle, circle_to_triangle, rect_to_triangle

 Notes
   - Coordinate system: y-up (top = y + h/2, bottom = y - h/2)
   - Rects are centered at position with width/height
   - Designed for use with CollisionSystem test harness
 Author: jiahao.zhou@digipen.edu
 Date:   2025-09-30
===============================================================================
*/

enum class ShapeType { Circle, Rect, Triangle };

struct Circle { float radius {0.0f};};
struct Rect   { float width{0.0f}, height{0.0f};};
//struct for triangle, but not implemented in shapetype first
struct Triangle {
    Framework::Vector2D v0;
    Framework::Vector2D v1;
    Framework::Vector2D v2;
};
// ---- Window Bounds------------------------------------
struct Bounds {
    float left{ -320.0f };
    float right{ 320.0f };
    float bottom{ -240.0f };
    float top{ 240.0f };
};


struct Collider {
    ShapeType shapeType{ShapeType::Circle};
    Framework::Vector2D position{};
    
    Circle circle{};
    Rect rect{};
    Triangle triangle{};

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
    static Collider create_triangle(Framework::Vector2D v0,
        Framework::Vector2D v1,
        Framework::Vector2D v2) {
        Collider c;
        c.shapeType = ShapeType::Triangle;
        c.triangle.v0 = v0;
        c.triangle.v1 = v1;
        c.triangle.v2 = v2;
        // store a simple center so logging looks same style as others
        c.position.x = (v0.x + v1.x + v2.x) / 3.0f;
        c.position.y = (v0.y + v1.y + v2.y) / 3.0f;
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

//check boundry for shapes
bool circle_out_of_bounds(const Collider& c, const Bounds& b);
bool rect_out_of_bounds(const Collider& r, const Bounds& b);
bool point_out_of_bounds(const Framework::Vector2D& p, const Bounds& b);

// Triangle collision function, not implement yet
bool point_in_triangle(Framework::Vector2D const& p, Triangle const& tri);
bool circle_to_triangle(const Collider& circle, const Collider& triCol);
bool rect_to_triangle(const Collider& rectAABB, const Collider& triCol);