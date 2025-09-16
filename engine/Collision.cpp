#include "Collision.h"
#include <cmath>
#include <algorithm>


bool Circle_To_Circle (const Collider& a, const Collider& b) {
    float dx = a.position.x - b.position.x;
    float dy = a.position.y - b.position.y;
    float distanceSquared = dx * dx + dy * dy;
    float radiusSum = a.circle.radius + b.circle.radius;
    return distanceSquared <= radiusSum * radiusSum;
}

bool Rect_To_Rect() {

}

bool Circle_To_Rect(const Collider& circle, const Collider& rect){
    float rectLeft = rect.position.x - rect.rect.width / 2;
	float rectRight = rect.position.x + rect.rect.width / 2;
	float rectTop = rect.position.y - rect.rect.height / 2;
	float rectBottom = rect.position.y + rect.rect.height / 2;

	// Find the closest point on the rectangle to the circle
	float closestX = (circle.position.x < rectLeft) ? rectLeft :
		(circle.position.x > rectRight) ? rectRight :
		circle.position.x;

	float closestY = (circle.position.y < rectTop) ? rectTop :
		(circle.position.y > rectBottom) ? rectBottom :
		circle.position.y;

	// Compute distance between the circle's center and the closest point
	float distanceX = circle.position.x - closestX;
	float distanceY = circle.position.y - closestY;

	// Compute squared distance and compare with squared radius
	float distanceSquared = (distanceX * distanceX) + (distanceY * distanceY);
	return distanceSquared < (circle.circle.radius * circle.circle.radius);
}

bool CheckCollision (const Collider& a, const Collider& b) {
    if (a.shapeType == ShapeType::Circle && b.shapeType == ShapeType::Circle) {
        return Circle_To_Circle(a, b);
    } else if (a.shapeType == ShapeType::Rect && b.shapeType == ShapeType::Rect) {
        return Rect_To_Rect(a, b);
    } else if (a.shapeType == ShapeType::Circle && b.shapeType == ShapeType::Rect) {
        return Circle_To_Rect(a, b);
    } else if (a.shapeType == ShapeType::Rect && b.shapeType == ShapeType::Circle) {
        return Circle_To_Rect(b, a); // Swap order for Circle-To-Rect
    }
    return false; // Fallback case
}