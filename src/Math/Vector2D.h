/*
===============================================================================
 File:           Vector2D.h
 Author:         Josh Ong
 Email:          josh.o@digipen.edu
 Date:           2025-09-22
 Contribution:   100%
 ------------------------------------------------------------------------------
 Header file for the Vector2D class.

  Design notes:
  This class represents a 2D vector or point with float components (x, y).
  It provides a modern C++ interface with overloaded operators for intuitive
  mathematical expressions (e.g., v1 + v2, v * scalar).

  Key features include:
  -   Constructors for creating zero and initialized vectors.
  -   Member functions for vector properties like length() and normalization.
	  Both mutating (normalize()) and non-mutating (normalized()) versions
	  are provided for flexibility.
  -   Static functions for operations involving two vectors, like dot().
  -   Type aliases (Vec2, Point2D) for convenience.

 Copyright (C) 2026 DigiPen Institute of Technology.
 Reproduction or disclosure of this file or its contents
 without the prior written consent of DigiPen Institute of
 Technology is prohibited.
===============================================================================
*/
#pragma once

namespace Framework {

	class Vector2D {
	public:
		float x, y;

		// Constructors
		Vector2D();                 // (0,0)
		Vector2D(float _x, float _y);

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

	// Binary operators (non-member functions)
	Vector2D operator+(Vector2D lhs, Vector2D const& rhs);
	Vector2D operator-(Vector2D lhs, Vector2D const& rhs);
	Vector2D operator*(Vector2D lhs, float rhs);
	Vector2D operator*(float lhs, Vector2D rhs);
	Vector2D operator/(Vector2D lhs, float rhs);

	// non-member functions
	void  Vector2DNormalize(Vector2D& pResult, Vector2D const& pVec0);
	float Vector2DLength(Vector2D const& pVec0);
	float Vector2DSquareLength(Vector2D const& pVec0);
	float Vector2DDistance(Vector2D const& pVec0, Vector2D const& pVec1);
	float Vector2DSquareDistance(Vector2D const& pVec0, Vector2D const& pVec1);
	float Vector2DDotProduct(Vector2D const& pVec0, Vector2D const& pVec1);
	float Vector2DCrossProductMag(Vector2D const& pVec0, Vector2D const& pVec1);

	using Vec2 = Vector2D;
	using Point2D = Vector2D;
	using Pt2 = Vector2D;

}
