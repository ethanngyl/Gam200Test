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
- Circle–circle: center distance vs (r1 + r2).
- Rect–rect (AABB): overlap test on X and Y (y-up).
- Circle–rect: clamp circle center to rect; compare to radius.
- Point tests: follow circle/rect rules.
- Bounds checks: compare shape extents to world bounds.
- Triangle: barycentric point-in-tri + segment–segment edge checks.

Notes:
- World units are consistent with the harness.
- Edge-touch counts as collision (<= comparisons).

Safety:
- Float math only; short-circuit on degenerate triangles.
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

// -------------------------------------------------------------------------
// Returns true if two circles intersect or touch.
// Touching edges count as collision.
// -------------------------------------------------------------------------
bool circle_to_circle(const Collider& a, const Collider& b) {
    float dx = a.position.x - b.position.x;
    float dy = a.position.y - b.position.y;
    float distanceSquared = dx * dx + dy * dy;
    float radiusSum = a.circle.radius + b.circle.radius;
    return distanceSquared <= radiusSum * radiusSum;
}


// -------------------------------------------------------------------------
// return true if overlap exists on both X and Y axes
// -------------------------------------------------------------------------
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


///below are all functions for triangle collision
// ---------- required functions ----------

// Barycentric test
bool point_in_triangle(Framework::Vector2D const& p, Triangle const& tri)
{
    using Framework::Vector2D;
    const Vector2D& a = tri.v0;
    const Vector2D& b = tri.v1;
    const Vector2D& c = tri.v2;

    Vector2D v0 = c - a;
    Vector2D v1 = b - a;
    Vector2D v2 = p - a;

    float dot00 = Vector2D::dot(v0, v0);
    float dot01 = Vector2D::dot(v0, v1);
    float dot02 = Vector2D::dot(v0, v2);
    float dot11 = Vector2D::dot(v1, v1);
    float dot12 = Vector2D::dot(v1, v2);

    float denom = dot00 * dot11 - dot01 * dot01;
    if (std::fabs(denom) < 1e-12f) return false; // degenerate triangle

    float invDenom = 1.0f / denom;
    float u = (dot11 * dot02 - dot01 * dot12) * invDenom;
    float v = (dot00 * dot12 - dot01 * dot02) * invDenom;

    return (u >= 0.0f) && (v >= 0.0f) && (u + v <= 1.0f);
}


/// Check if circle intersects triangle (point-in-tri OR edge distance).
/// return true/false
bool circle_to_triangle(const Collider& A, const Collider& B)
{
    using Framework::Vector2D;

    const Collider* circle = nullptr;
    const Collider* triCol = nullptr;

    if (A.shapeType == ShapeType::Circle && B.shapeType == ShapeType::Triangle) { circle = &A; triCol = &B; }
    else if (A.shapeType == ShapeType::Triangle && B.shapeType == ShapeType::Circle) { circle = &B; triCol = &A; }
    else { return false; }

    const Triangle& tri = triCol->triangle;
    const Vector2D C = circle->position;
    const float r = circle->circle.radius;
    const float r2 = r * r;



    // 1) Any vertex of triangle inside circle?
    {
        Vector2D d = tri.v0 - C; if (Vector2D::dot(d, d) <= r2) return true;
        d = tri.v1 - C;          if (Vector2D::dot(d, d) <= r2) return true;
        d = tri.v2 - C;          if (Vector2D::dot(d, d) <= r2) return true;
    }

    // 2) Circle center inside triangle?
    if (point_in_triangle(C, tri)) return true;

    // 3) Any triangle edge within radius of circle center?
    {
        // Helper: closest point on segment (triangle edge) to circle center (scoped lambda).
        auto closest_point_on_segment = [](const Vector2D& a, const Vector2D& b, const Vector2D& p) {
            Vector2D ab = b - a; Vector2D ap = p - a;
            float abLen2 = Vector2D::dot(ab, ab); if (abLen2 <= 1e-12f) return a;
            float t = Vector2D::dot(ap, ab) / abLen2; if (t < 0) t = 0; else if (t > 1) t = 1;
            return a + ab * t;
            };

        Vector2D q, dq;
        q = closest_point_on_segment(tri.v0, tri.v1, C); dq = q - C; if (Vector2D::dot(dq, dq) <= r2) return true;
        q = closest_point_on_segment(tri.v1, tri.v2, C); dq = q - C; if (Vector2D::dot(dq, dq) <= r2) return true;
        q = closest_point_on_segment(tri.v2, tri.v0, C); dq = q - C; if (Vector2D::dot(dq, dq) <= r2) return true;
    }

    return false;
}


/// Check if rect intersects triangle (by edges and point tests).
/// return true/false
bool rect_to_triangle(const Collider& rectAABB, const Collider& triCol)
{
    using Framework::Vector2D;
    const Triangle& tri = triCol.triangle;
    const float hw = rectAABB.rect.width * 0.5f;
    const float hh = rectAABB.rect.height * 0.5f;

    // Rectangle corners (centered at rectAABB.position)
    const Vector2D bl{ rectAABB.position.x - hw, rectAABB.position.y - hh }; // bottom-left
    const Vector2D br{ rectAABB.position.x + hw, rectAABB.position.y - hh }; // bottom-right
    const Vector2D tr{ rectAABB.position.x + hw, rectAABB.position.y + hh }; // top-right
    const Vector2D tl{ rectAABB.position.x - hw, rectAABB.position.y + hh }; // top-left

    // 1) Any triangle vertex inside rect?
    auto point_in_rect_local = [&](const Vector2D& p)->bool {
        return (p.x >= bl.x && p.x <= br.x && p.y >= bl.y && p.y <= tl.y);
        };
    if (point_in_rect_local(tri.v0) || point_in_rect_local(tri.v1) || point_in_rect_local(tri.v2))
        return true;

    // 2) Any rect corner inside triangle?
    if (point_in_triangle(bl, tri) || point_in_triangle(br, tri) ||
        point_in_triangle(tr, tri) || point_in_triangle(tl, tri)) {
        std::cout << "Collision Detected\n";
        return true;
    }

    //if (point_in_triangle(rectAABB.position, tri))
       // return true;


    //to check any edge intersection between triangle and rect?
    // Helper: segment–segment intersection using 2D cross products.
    auto segments_intersect = [](const Vector2D& p1, const Vector2D& p2,
        const Vector2D& q1, const Vector2D& q2)->bool
        {
            auto cross2 = [](const Vector2D& a, const Vector2D& b) {
                return a.x * b.y - a.y * b.x;
                };
            Vector2D r = p2 - p1, s = q2 - q1;
            float rxs = cross2(r, s);
            if (std::fabs(rxs) < 1e-6f) return false; // parallel/collinear treated as no-hit here
            float t = cross2(q1 - p1, s) / rxs;
            float u = cross2(q1 - p1, r) / rxs;
            return (t >= 0.f && t <= 1.f && u >= 0.f && u <= 1.f);
        };

    const Vector2D triPts[3] = { tri.v0, tri.v1, tri.v2 };
    const Vector2D rectPts[4] = { bl, br, tr, tl };

    for (int i = 0; i < 3; ++i) {
        Vector2D a = triPts[i];
        Vector2D b = triPts[(i + 1) % 3];
        for (int j = 0; j < 4; ++j) {
            Vector2D c = rectPts[j];
            Vector2D d = rectPts[(j + 1) % 4];
            if (segments_intersect(a, b, c, d)) return true;
        }
    }

    return false;
}