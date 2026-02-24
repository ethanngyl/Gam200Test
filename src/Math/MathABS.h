/**
===============================================================================
 File:           MathABS.h
 Author:         Josh Ong
 Email:          josh.o@digipen.edu
 Date:           2025-10-27
 Contribution:   100%
 ------------------------------------------------------------------------------

  Brief:
  - Provides fast, inline, and constexpr implementations for calculating the
    absolute value (Abs) for various scalar types and a component-wise absolute
    value for Vector2D.

  Key features:
  - **Scalar Abs**: Overloaded templates/functions for all standard signed and
    unsigned integer types, as well as float, double, and long double.
  - **Vector Abs**: Provides component-wise absolute value calculation for Vector2D.
  - **Performance**: Use of 'inline constexpr' ensures these functions are typically
    evaluated at compile time or inlined for maximum runtime efficiency, avoiding
    potential overhead from <cmath> or <cstdlib> standard library calls.
  - **Magnitude Helper**: Includes LengthSq for squared magnitude, a common
    optimization technique that avoids a square root operation.


 Copyright (C) 2026 DigiPen Institute of Technology.
 Reproduction or disclosure of this file or its contents
 without the prior written consent of DigiPen Institute of
 Technology is prohibited.
===============================================================================
 */

#pragma once
#include "Vector2D.h"

namespace Framework {

    // ----- Scalar abs -----

    // floats
    inline constexpr float  Abs(float  x) noexcept { return x < 0.0f ? -x : x; }
    // doubles
    inline constexpr double Abs(double x) noexcept { return x < 0.0 ? -x : x; }
    // long doubles (if you ever use them)
    inline constexpr long double Abs(long double x) noexcept { return x < 0.0L ? -x : x; }

    // signed integers
    inline constexpr int    Abs(int    x) noexcept { return x < 0 ? -x : x; }
    inline constexpr long   Abs(long   x) noexcept { return x < 0 ? -x : x; }
    inline constexpr long long Abs(long long x) noexcept { return x < 0 ? -x : x; }

    // unsigned “abs” just returns the value (already non-negative)
    inline constexpr unsigned int       Abs(unsigned int       x) noexcept { return x; }
    inline constexpr unsigned long      Abs(unsigned long      x) noexcept { return x; }
    inline constexpr unsigned long long Abs(unsigned long long x) noexcept { return x; }

    // ----- Vector component-wise abs -----

    inline Vector2D Abs(const Vector2D& v) noexcept {
        return Vector2D{ v.x < 0.0f ? -v.x : v.x,
                         v.y < 0.0f ? -v.y : v.y };
    }

    // (Optional) magnitude helpers that don’t use std::
    inline constexpr float LengthSq(const Vector2D& v) noexcept { return v.x * v.x + v.y * v.y; }


}
