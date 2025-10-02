#pragma once
#include "ECSComponent.h"
#include "Vector2D.h"
#include <string>
/**
 * @file Component.h
 * @author ETHAN NG YONG LE (n.ethanyongle@digipen.edu)
 * @brief Core engine implementation providing game loop and system management
 * @date 2025-09-30
 *
 * @copyright Copyright (c) 2025
 *
 * Contains all component data structures used in the Entity Component System.
 * Components are pure data containers with no behavior - systems operate on them.
 */

namespace Framework
{
    /**
     * @struct Transform
     * @brief Spatial transformation component
     *
     * Defines an entity's position, rotation, and scale in 2D space.
     * Used by rendering and physics systems.
     */
    struct Transform : public Component<Transform>
    {
        Vector2D position;
        float rotation = 0.0f;
        Vector2D scale = Vector2D(1.0f, 1.0f);

        Transform(Vector2D pos = Vector2D()) : position(pos) {}
    };

    /**
     * @struct Movement
     * @brief Movement parameters for entities
     *
     * Defines movement speed and direction for physics/movement systems.
     */
    struct Movement : public Component<Movement>
    {
        float moveSpeed = 100.0f;
        Vector2D direction;
        bool blocked = false;
    };

    /**
     * @struct Sprite
     * @brief Visual rendering component
     *
     * Defines the visual representation of an entity for the graphics system.
     */
    struct Sprite : public Component<Sprite>
    {
        std::string texturePath;
        int layer = 0;
        bool flipX = false;
        bool flipY = false;
    };

    /**
     * @struct BoxCollider
     * @brief Axis-aligned bounding box collider
     *
     * Rectangular collision shape for collision detection.
     */
    struct BoxCollider : public Component<BoxCollider>
    {
        Vector2D size;
        Vector2D offset;
        bool isTrigger = false;
    };

    /**
     * @struct TriangleCollider
     * @brief Triangle-shaped collider
     *
     * Three-vertex polygon collision shape for more complex collision detection.
     */
    struct TriangleCollider : public Component<TriangleCollider>
    {
        Vector2D v0, v1, v2;  // Three vertices relative to transform position

        TriangleCollider(Vector2D vertex0 = Vector2D(-0.25f, -0.25f),
            Vector2D vertex1 = Vector2D(0.25f, -0.25f),
            Vector2D vertex2 = Vector2D(0.0f, 0.25f))
            : v0(vertex0), v1(vertex1), v2(vertex2) {
        }
    };

    /**
     * @struct CircleCollider
     * @brief Circular collision shape
     *
     * Radius-based collider for circular collision detection.
     */
    struct CircleCollider : public Component<CircleCollider>
    {
        float radius;       ///< Collision circle radius
        Vector2D offset;    ///< Offset from the transform position

        /**
         * @brief Constructs a circle collider
         * @param r Radius of the collision circle
         * @param off Offset from entity position
         */
        CircleCollider(float r = 0.5f, Vector2D off = Vector2D(0.0f, 0.0f))
            : radius(r), offset(off) {
        }
    };
}