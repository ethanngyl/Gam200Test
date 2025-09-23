#include "Vector2D.h"
#include <cmath>
#include <cassert>

// ---- ctors ----
Vector2D::Vector2D() : x(0.0f), y(0.0f) {}
Vector2D::Vector2D(float _x, float _y) : x(_x), y(_y) {}

// ---- array-style access ----
float* Vector2D::data() { return &x; }
float const* Vector2D::data() const { return &x; }
float& Vector2D::operator[](int i) { return (i == 0) ? x : y; }
float        Vector2D::operator[](int i) const { return (i == 0) ? x : y; }

// ---- assignment ops ----
Vector2D& Vector2D::operator+=(Vector2D const& rhs) { x += rhs.x; y += rhs.y; return *this; }
Vector2D& Vector2D::operator-=(Vector2D const& rhs) { x -= rhs.x; y -= rhs.y; return *this; }
Vector2D& Vector2D::operator*=(float rhs) { x *= rhs;   y *= rhs;   return *this; }
Vector2D& Vector2D::operator/=(float rhs) {
	assert(rhs != 0.0f && "Vector2D /= 0");
	x /= rhs; y /= rhs; return *this;
}

// ---- unary ----
Vector2D Vector2D::operator-() const { return Vector2D{ -x, -y }; }

// ---- magnitudes/helpers ----
float Vector2D::lengthSq() const { return x * x + y * y; }
float Vector2D::length()   const { return std::sqrt(lengthSq()); }

Vector2D& Vector2D::normalize() {
	float len = length();
	if (len > 0.0f) { x /= len; y /= len; }
	return *this;
}
Vector2D Vector2D::normalized() const {
	float len = length();
	return (len > 0.0f) ? Vector2D{ x / len, y / len } : Vector2D{};
}

float Vector2D::dot(Vector2D const& a, Vector2D const& b) { return a.x * b.x + a.y * b.y; }
float Vector2D::crossMag(Vector2D const& a, Vector2D const& b) { return a.x * b.y - a.y * b.x; }
float Vector2D::distanceSq(Vector2D const& a, Vector2D const& b) { return (a - b).lengthSq(); }
float Vector2D::distance(Vector2D const& a, Vector2D const& b) { return std::sqrt(distanceSq(a, b)); }

// ---- free binary operators ----
Vector2D operator+(Vector2D lhs, Vector2D const& rhs) { lhs += rhs; return lhs; }
Vector2D operator-(Vector2D lhs, Vector2D const& rhs) { lhs -= rhs; return lhs; }
Vector2D operator*(Vector2D lhs, float rhs) { lhs *= rhs; return lhs; }
Vector2D operator*(float lhs, Vector2D rhs) { rhs *= lhs; return rhs; }
Vector2D operator/(Vector2D lhs, float rhs) { lhs /= rhs; return lhs; }

// ---- legacy wrappers ----
void  Vector2DNormalize(Vector2D& pResult, Vector2D const& pVec0) { pResult = pVec0; pResult.normalize(); }
float Vector2DLength(Vector2D const& pVec0) { return pVec0.length(); }
float Vector2DSquareLength(Vector2D const& pVec0) { return pVec0.lengthSq(); }
float Vector2DDistance(Vector2D const& pVec0, Vector2D const& pVec1) { return Vector2D::distance(pVec0, pVec1); }
float Vector2DSquareDistance(Vector2D const& pVec0, Vector2D const& pVec1) { return Vector2D::distanceSq(pVec0, pVec1); }
float Vector2DDotProduct(Vector2D const& pVec0, Vector2D const& pVec1) { return Vector2D::dot(pVec0, pVec1); }
float Vector2DCrossProductMag(Vector2D const& pVec0, Vector2D const& pVec1) { return Vector2D::crossMag(pVec0, pVec1); }
