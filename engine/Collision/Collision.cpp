#include "Collision.h"
//#include <cmath>
//#include <algorithm>


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
    float aTop = a.position.y - a.rect.height / 2;
    float aBottom = a.position.y + a.rect.height / 2;

    float bLeft = b.position.x - b.rect.width / 2;
    float bRight = b.position.x + b.rect.width / 2;
    float bTop = b.position.y - b.rect.height / 2;
    float bBottom = b.position.y + b.rect.height / 2;

    return (aLeft <= bRight && aRight >= bLeft &&
            aTop <= bBottom && aBottom >= bTop);
}

bool circle_to_rect (const Collider& circle, const Collider& rect){
    float rectLeft = rect.position.x - rect.rect.width / 2;
	float rectRight = rect.position.x + rect.rect.width / 2;
	float rectTop = rect.position.y - rect.rect.height / 2;
	float rectBottom = rect.position.y + rect.rect.height / 2;

	// Find the most closest point on the rectangle to the circle
	float closestX = (circle.position.x < rectLeft) ? rectLeft :
		(circle.position.x > rectRight) ? rectRight :
		circle.position.x;

	float closestY = (circle.position.y < rectTop) ? rectTop :
		(circle.position.y > rectBottom) ? rectBottom :
		circle.position.y;

	// calculte distance between the circle's center and the closest point
	float distanceX = circle.position.x - closestX;
	float distanceY = circle.position.y - closestY;

	// calculte squared distance and compare with squared radius
	float distanceSquared = (distanceX * distanceX) + (distanceY * distanceY);
	return distanceSquared <= (circle.circle.radius * circle.circle.radius);
}

bool point_in_circle(const Vec2& point, const Collider& circle) {
    
    float dx = point.x - circle.position.x;
    float dy = point.y - circle.position.y;
    float distance = dx * dx + dy * dy;
    float radiusSqaured = circle.circle.radius * circle.circle.radius;
    return distance <= radiusSqaured; 
}

bool point_in_rect(const Vec2& point, const Collider& rect) {
    // centered AABB with y-up: top = y - h/2, bottom = y + h/2 (matches your rect_to_rect)
    float left   = rect.position.x - rect.rect.width  * 0.5f;
    float right  = rect.position.x + rect.rect.width  * 0.5f;
    float top    = rect.position.y - rect.rect.height * 0.5f;
    float bottom = rect.position.y + rect.rect.height * 0.5f;

    return (point.x >= left  && point.x <= right &&
            point.y >= top   && point.y <= bottom); // touch = hit
}


bool point_in_collider(const Vec2& point, const Collider& c) {
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