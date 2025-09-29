#include "Collision.h"
//#include <cmath>
//#include <algorithm>

/*
===============================================================================
 Collision.cpp
------------------------------------------------------------------------------
 Implementations for primitive collision tests and boundary clamps.

 Key ideas
   - circle to circle: center distance vs (r1 + r2)
   - rect to rect (AABB): overlap on X and Y with y-up convention
   - circle to rect: clamp circle center to rect and compare distance to radius
   - point tests: reuse the same rect/circle rules
   - Bounds clamp: keep center shapes fully inside (respecting radius/half extents)

 Assumptions
   - All sizes/positions in same world units as the test harness
   - Touching edges count as collision (<= checks)

 Author: jiahao.zhou@digipen.edu
 Date:   2025-09-30
===============================================================================
*/

#include "Math/Vector2D.h"


bool circle_to_circle (const Collider& a, const Collider& b) {
    float dx = a.position.x - b.position.x;
    float dy = a.position.y - b.position.y;
    float distanceSquared = dx * dx + dy * dy;
    float radiusSum = a.circle.radius + b.circle.radius;
    return distanceSquared <= radiusSum * radiusSum;
}

bool rect_to_rect (const Collider& a, const Collider& b) { 
    float aLeft = a.position.x - a.rect.width / 2;
    float aRight = a.position.x + a.rect.width / 2;
    float aTop = a.position.y + a.rect.height / 2;
    float aBottom = a.position.y - a.rect.height / 2;

    float bLeft = b.position.x - b.rect.width / 2;
    float bRight = b.position.x + b.rect.width / 2;
    float bTop = b.position.y + b.rect.height / 2;
    float bBottom = b.position.y - b.rect.height / 2;

    return (aLeft <= bRight && aRight >= bLeft &&
            aTop >= bBottom && aBottom <= bTop);
}

bool circle_to_rect (const Collider& circle, const Collider& rect){
    float rectLeft = rect.position.x - rect.rect.width / 2;
	float rectRight = rect.position.x + rect.rect.width / 2;
	float rectTop = rect.position.y + rect.rect.height / 2;
	float rectBottom = rect.position.y - rect.rect.height / 2;

	// Find the most closest point on the rectangle to the circle
	float closestX = (circle.position.x < rectLeft) ? rectLeft :
		(circle.position.x > rectRight) ? rectRight :
		circle.position.x;

	float closestY = (circle.position.y < rectBottom) ? rectBottom :
		(circle.position.y > rectTop) ? rectTop :
		circle.position.y;

	// calculte distance between the circle's center and the closest point
	float distanceX = circle.position.x - closestX;
	float distanceY = circle.position.y - closestY;

	// calculte squared distance and compare with squared radius
	float distanceSquared = (distanceX * distanceX) + (distanceY * distanceY);
	return distanceSquared <= (circle.circle.radius * circle.circle.radius);
}

bool point_in_circle(const Framework::Vector2D point, const Collider& circle) {
    
    float dx = point.x - circle.position.x;
    float dy = point.y - circle.position.y;
    float distance = dx * dx + dy * dy;
    float radiusSqaured = circle.circle.radius * circle.circle.radius;
    return distance <= radiusSqaured; 
}

bool point_in_rect(const Framework::Vector2D point, const Collider& rect) {
    // centered AABB with y-up: top = y - h/2, bottom = y + h/2 (matches your rect_to_rect)
    float left   = rect.position.x - rect.rect.width  * 0.5f;
    float right  = rect.position.x + rect.rect.width  * 0.5f;
    float top    = rect.position.y + rect.rect.height * 0.5f;
    float bottom = rect.position.y - rect.rect.height * 0.5f;

    return (point.x >= left  && point.x <= right &&
            point.y <= top   && point.y >= bottom); // touch = hit
}


bool point_in_collider(const Framework::Vector2D point, const Collider& c) {
    if (c.shapeType == ShapeType::Circle) return point_in_circle(point, c);
    // else Rect
    return point_in_rect(point, c);
}

bool check_collision (const Collider& a, const Collider& b) {
    if (a.shapeType == ShapeType::Circle && b.shapeType == ShapeType::Circle) {
        return circle_to_circle(a, b);
    } else if (a.shapeType == ShapeType::Rect && b.shapeType == ShapeType::Rect) {
        return rect_to_rect(a, b);
    } else if (a.shapeType == ShapeType::Circle && b.shapeType == ShapeType::Rect) {
        return circle_to_rect(a, b);
    } else if (a.shapeType == ShapeType::Rect && b.shapeType == ShapeType::Circle) {
        return circle_to_rect(b, a); // Swap order for Circle-To-Rect
    }
    return false; // Fallback case
}

bool circle_out_of_bounds(const Collider& c, const Bounds& b) {
    float r = c.circle.radius;
    return (c.position.x - r < b.left) || (c.position.x + r > b.right) ||
        (c.position.y - r < b.bottom) || (c.position.y + r > b.top);
}

bool rect_out_of_bounds(const Collider& r, const Bounds& b) {
    float halfW = r.rect.width * 0.5f;
    float halfH = r.rect.height * 0.5f;
    float left = r.position.x - halfW;
    float right = r.position.x + halfW;
    float bottom = r.position.y - halfH;
    float top = r.position.y + halfH;
    return (left < b.left) || (right > b.right) || (bottom < b.bottom) || (top > b.top);
}

bool point_out_of_bounds(const Framework::Vector2D& p, const Bounds& b) {
    return (p.x < b.left) || (p.x > b.right) || (p.y < b.bottom) || (p.y > b.top);
}