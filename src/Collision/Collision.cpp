/*
===============================================================================
File:        Collision.cpp
Author:      Jiahao Zhou
Email:       jiahao.zhou@digipen.edu
Date:        2025-09-30
Contribution: 100%
-------------------------------------------------------------------------------
Implementations for primitive collision/bounds/triangle tests.

Key ideas:
- Circle-circle: center distance vs (r1 + r2).
- Rect-rect (AABB): overlap test on X and Y (y-up).
- Circle-rect: clamp circle center to rect; compare to radius.
- Point tests: follow circle/rect rules.
- Bounds checks: compare shape extents to world bounds.

Notes:
- World units are consistent with the harness.
- Edge-touch counts as collision (<= comparisons).

Safety:
- No allocations; all inputs are by const ref/value.
===============================================================================
*/

#include "Precompiled.h"
#include "Collision.h"
//#include <cmath>
//#include <algorithm>
#include "Math/Matrix3x3.h"
#include <cmath>
#include <iostream>


/// Returns true if two circles intersect or touch.
/// Touching edges count as collision.
bool circle_to_circle(const Collider& a, const Collider& b) {
    float dx = a.position.x - b.position.x;
    float dy = a.position.y - b.position.y;
    float distanceSquared = dx * dx + dy * dy;
    float radiusSum = a.circle.radius + b.circle.radius;
    return distanceSquared <= radiusSum * radiusSum;
}

/// return true if overlap exists on both X and Y axes
bool rect_to_rect(const Collider& a, const Collider& b) {
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

/// Clamp-and-check circle vs AABB (y-up). True if intersect or touch.
bool circle_to_rect (const Collider& A, const Collider& B){

    const Collider* circle = nullptr;
    const Collider* rect = nullptr;

    if (A.shapeType == ShapeType::Circle && B.shapeType == ShapeType::Rect) { circle = &A; rect = &B; }
    else if (A.shapeType == ShapeType::Rect && B.shapeType == ShapeType::Circle) { circle = &B; rect = &A; }
    else { return false; } // not a circle-rect pair

    float rectLeft = rect->position.x - rect->rect.width / 2;
	float rectRight = rect->position.x + rect->rect.width / 2;
	float rectTop = rect->position.y + rect->rect.height / 2;
	float rectBottom = rect->position.y - rect->rect.height / 2;

	// Find the most closest point on the rectangle to the circle
	float closestX = (circle->position.x < rectLeft) ? rectLeft :
		(circle->position.x > rectRight) ? rectRight :
		circle->position.x;

	float closestY = (circle->position.y < rectBottom) ? rectBottom :
		(circle->position.y > rectTop) ? rectTop :
		circle->position.y;

	// calculte distance between the circle's center and the closest point
	float distanceX = circle->position.x - closestX;
	float distanceY = circle->position.y - closestY;

	// calculte squared distance and compare with squared radius
	float distanceSquared = (distanceX * distanceX) + (distanceY * distanceY);
    /*std::cout
        << "rect w=" << rect->rect.width
        << " h=" << rect->rect.height
        << " r=" << circle->circle.radius << "\n"
        << "L=" << rectLeft << " R=" << rectRight
        << " B=" << rectBottom << " T=" << rectTop << "\n"
        << "closest=(" << closestX << "," << closestY << ") "
        << "dx=" << distanceX << " dy=" << distanceY
        << " d2=" << distanceSquared
        << " r2=" << (circle->circle.radius * circle->circle.radius) << "\n";*/
	return distanceSquared <= (circle->circle.radius * circle->circle.radius);
}

/// True if point lies inside centered AABB (y-up). Edges are included.
bool point_in_circle(const Framework::Vector2D point, const Collider& circle) {

    float dx = point.x - circle.position.x;
    float dy = point.y - circle.position.y;
    float distance = dx * dx + dy * dy;
    float radiusSqaured = circle.circle.radius * circle.circle.radius;
    return distance <= radiusSqaured;
}

bool point_in_rect(const Framework::Vector2D point, const Collider& rect) {
    // centered AABB with y-up: top = y - h/2, bottom = y + h/2 (matches your rect_to_rect)
    float left = rect.position.x - rect.rect.width * 0.5f;
    float right = rect.position.x + rect.rect.width * 0.5f;
    float top = rect.position.y + rect.rect.height * 0.5f;
    float bottom = rect.position.y - rect.rect.height * 0.5f;

    return (point.x >= left && point.x <= right &&
        point.y <= top && point.y >= bottom); // touch = hit
}


/// decide which point collision fucntion to use based on collider type

bool point_in_collider(const Framework::Vector2D point, const Collider& c) {
    if (c.shapeType == ShapeType::Circle) return point_in_circle(point, c);
    // else Rect
    return point_in_rect(point, c);
}


/// decide which collision function to use based on collider types

bool check_collision(const Collider& a, const Collider& b) {
    if (a.shapeType == ShapeType::Circle && b.shapeType == ShapeType::Circle) {
        return circle_to_circle(a, b);
    }
    else if (a.shapeType == ShapeType::Rect && b.shapeType == ShapeType::Rect) {
        return rect_to_rect(a, b);
    }
    else if (a.shapeType == ShapeType::Circle && b.shapeType == ShapeType::Rect) {
        return circle_to_rect(a, b);
    }
    else if (a.shapeType == ShapeType::Rect && b.shapeType == ShapeType::Circle) {
        return circle_to_rect(b, a); // Swap order for Circle-To-Rect
    }
    return false; // Fallback case
}

/// return true if circle is out of bounds
bool circle_out_of_bounds(const Collider& c, const Bounds& b) {
    //std::cout << "circle out of bounds check\n";
    //std::cout << "circle position: (" << c.position.x << ", " << c.position.y << ")\n";
    //std::cout << "bounds left: " << b.left << ", right: " << b.right
        //<< ", bottom: " << b.bottom << ", top: " << b.top << "\n";
    float r = c.circle.radius;
    return (c.position.x - r < b.left) || (c.position.x + r > b.right) ||
        (c.position.y - r < b.bottom) || (c.position.y + r > b.top);
}

/// return true if rect is out of bounds
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