/**
===============================================================================
 File:        BroadphaseAABB.h
 Author:      Jiahao Zhou
 Email:       jiahao.zhou@digipen.edu
 Date:        2026-01-27
-------------------------------------------------------------------------------
 Copyright (C) 2026 DigiPen Institute of Technology.
 Reproduction or disclosure of this file or its contents
 without the prior written consent of DigiPen Institute of
 Technology is prohibited.
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
    /**
     * @brief Axis-Aligned Bounding Box (AABB) represented by min/max corners.
     *
     * Coordinates are in world space. Used by broadphase systems (e.g., quadtree)
     * to conservatively filter candidate collision pairs.
     */

    struct AABB
    {
        Vector2D min;
        Vector2D max;
    };

    /**
     * @brief Build an AABB from a center position and half-extents.
     *
     * This is commonly used when the caller already has half-size data
     * (e.g., collider half width/height in world space).
     *
     * @param center Center of the box in world space.
     * @param halfExtents Half width/height in world space.
     * @return AABB with min/max computed as center +/- halfExtents.
     */

    inline AABB MakeAABBFromCenterHalfExtents(const Vector2D& center, const Vector2D& half)
    {
        AABB aabb;
        aabb.min = Vector2D(center.x - half.x, center.y - half.y);
        aabb.max = Vector2D(center.x + half.x, center.y + half.y);
        return aabb;
    }

    /**
     * @brief Build an AABB from a center position and full size.
     *
     * Internally converts full size to half-extents and computes bounds
     * as center +/- (size * 0.5f).
     *
     * @param center Center of the box in world space.
     * @param size Full width/height in world space.
     * @return AABB with min/max computed from center and size.
     */

    inline AABB MakeAABBFromCenterSize(const Vector2D& center, const Vector2D& size)
    {
        Vector2D half(size.x * 0.5f, size.y * 0.5f);
        return MakeAABBFromCenterHalfExtents(center, half);
    }

    /**
     * @brief Build an AABB that conservatively encloses a circle.
     *
     * Used for broadphase candidate filtering for circular colliders.
     *
     * @param center Circle center in world space.
     * @param radius Circle radius in world space.
     * @return AABB spanning [center - (r,r), center + (r,r)].
     */


    inline AABB MakeAABBFromCircle(const Vector2D& center, float radius)
    {
        Vector2D half(radius, radius);
        return MakeAABBFromCenterHalfExtents(center, half);
    }

    /**
     * @brief Test whether two AABBs overlap (inclusive).
     *
     * Broadphase intersection tests should be conservative; if this returns true,
     * narrowphase collision tests may be performed to confirm actual collision.
     *
     * @param a First AABB.
     * @param b Second AABB.
     * @return True if the AABBs overlap on both X and Y axes.
     */

    inline bool AABBIntersects(const AABB& a, const AABB& b)
    {
        if (a.max.x < b.min.x) return false;
        if (a.min.x > b.max.x) return false;
        if (a.max.y < b.min.y) return false;
        if (a.min.y > b.max.y) return false;
        return true;
    }

    /**
     * @brief Test whether one AABB fully contains another.
     *
     * This is useful for quadtree insertion decisions (e.g., deciding whether an
     * object fits fully within a child node's bounds).
     *
     * @param outer The container AABB.
     * @param inner The candidate AABB to be contained.
     * @return True if inner is fully inside outer (inclusive).
     */


    inline bool AABBContains(const AABB& outer, const AABB& inner)
    {
        return inner.min.x >= outer.min.x &&
            inner.min.y >= outer.min.y &&
            inner.max.x <= outer.max.x &&
            inner.max.y <= outer.max.y;
    }
}
