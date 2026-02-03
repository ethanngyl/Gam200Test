#pragma once
/**
===============================================================================
 File:        BroadphaseAABB.h
 Author:      Jiahao Zhou
 Email:       jiahao.zhou@digipen.edu
 Date:        2026-01-27
-------------------------------------------------------------------------------
 Brief:
 Simple Axis-Aligned Bounding Box (AABB) used by broadphase algorithms such as
 Quadtree. Broadphase must be conservative: it should never miss a potential
 overlap candidate.

 Responsibilities:
 - Represent 2D AABB bounds (min/max).
 - Provide AABB intersection and containment helpers.
===============================================================================
 */

#pragma once

#include "Math/Vector2D.h"

namespace Framework
{
    struct AABB
    {
        Vector2D min;
        Vector2D max;
    };

    inline AABB MakeAABBFromCenterHalfExtents(const Vector2D& center, const Vector2D& half)
    {
        AABB aabb;
        aabb.min = Vector2D(center.x - half.x, center.y - half.y);
        aabb.max = Vector2D(center.x + half.x, center.y + half.y);
        return aabb;
    }

    inline AABB MakeAABBFromCenterSize(const Vector2D& center, const Vector2D& size)
    {
        Vector2D half(size.x * 0.5f, size.y * 0.5f);
        return MakeAABBFromCenterHalfExtents(center, half);
    }

    inline AABB MakeAABBFromCircle(const Vector2D& center, float radius)
    {
        Vector2D half(radius, radius);
        return MakeAABBFromCenterHalfExtents(center, half);
    }

    inline bool AABBIntersects(const AABB& a, const AABB& b)
    {
        if (a.max.x < b.min.x) return false;
        if (a.min.x > b.max.x) return false;
        if (a.max.y < b.min.y) return false;
        if (a.min.y > b.max.y) return false;
        return true;
    }

    inline bool AABBContains(const AABB& outer, const AABB& inner)
    {
        return inner.min.x >= outer.min.x &&
            inner.min.y >= outer.min.y &&
            inner.max.x <= outer.max.x &&
            inner.max.y <= outer.max.y;
    }
}
