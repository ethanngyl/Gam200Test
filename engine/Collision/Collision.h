#pragma once
#include "Math/Vector2D.h"

/*
===============================================================================
 Collision.h
------------------------------------------------------------------------------
 Collider shapes, test helpers, and boundary utilities for collision system.

 What¡¯s here
   - Data: ShapeType, Circle, Rect, Collider (centered AABB for rect)
   - Tests: circle_to_circle, rect_to_rect, circle_to_rect
            point_in_circle, point_in_rect, point_in_collider, check_collision
   - Bounds: Bounds struct + out_of_bounds and clamp helpers (y-up)

 Notes
   - Coordinate system: y-up (top = y + h/2, bottom = y - h/2)
   - Rects are centered at position with width/height

 Author: jiahao.zhou@digipen.edu
 Date:   2025-09-30
===============================================================================
*/

enum class ShapeType { Circle, Rect };

struct Circle { float radius {0.0f};};
struct Rect   { float width{0.0f}, height{0.0f};};

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

//check boundry for shapes
bool circle_out_of_bounds(const Collider& c, const Bounds& b);
bool rect_out_of_bounds(const Collider& r, const Bounds& b);
bool point_out_of_bounds(const Framework::Vector2D& p, const Bounds& b);