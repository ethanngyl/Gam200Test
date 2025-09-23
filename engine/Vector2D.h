#pragma once

class Vector2D {
public:
	float x, y;

	// Constructors
	Vector2D();                 // (0,0)
	Vector2D(float _x, float _y);

	// Do not change (kept as in your original)
	Vector2D& operator=(Vector2D const&) = default;
	Vector2D(Vector2D const&) = default;

	// Array-style access
	float* data();
	float const* data() const;
	float& operator[](int i);
	float        operator[](int i) const;

	// Assignment operators
	Vector2D& operator+=(Vector2D const& rhs);
	Vector2D& operator-=(Vector2D const& rhs);
	Vector2D& operator*=(float rhs);
	Vector2D& operator/=(float rhs);

	// Unary
	Vector2D operator-() const;

	// Magnitudes / helpers
	float     lengthSq()  const;
	float     length()    const;
	Vector2D& normalize();
	Vector2D  normalized() const;

	// Static math
	static float dot(Vector2D const& a, Vector2D const& b);
	static float crossMag(Vector2D const& a, Vector2D const& b);
	static float distanceSq(Vector2D const& a, Vector2D const& b);
	static float distance(Vector2D const& a, Vector2D const& b);
};

// Binary operators (free functions)
Vector2D operator+(Vector2D lhs, Vector2D const& rhs);
Vector2D operator-(Vector2D lhs, Vector2D const& rhs);
Vector2D operator*(Vector2D lhs, float rhs);
Vector2D operator*(float lhs, Vector2D rhs);
Vector2D operator/(Vector2D lhs, float rhs);

// Legacy-style free functions (backward compatible names)
void  Vector2DNormalize(Vector2D& pResult, Vector2D const& pVec0);
float Vector2DLength(Vector2D const& pVec0);
float Vector2DSquareLength(Vector2D const& pVec0);
float Vector2DDistance(Vector2D const& pVec0, Vector2D const& pVec1);
float Vector2DSquareDistance(Vector2D const& pVec0, Vector2D const& pVec1);
float Vector2DDotProduct(Vector2D const& pVec0, Vector2D const& pVec1);
float Vector2DCrossProductMag(Vector2D const& pVec0, Vector2D const& pVec1);

// Optional aliases to match your typedefs
using Vec2 = Vector2D;
using Point2D = Vector2D;
using Pt2 = Vector2D;

