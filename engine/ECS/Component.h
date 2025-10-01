#pragma once
#include "Precompiled.h"
//#include "ECSComponent.h"
//#include "../Math/Vector2D.h"
//#include <string>

namespace Framework
{
    struct Transform : public Component<Transform>
    {
        Vector2D position;
        float rotation = 0.0f;
        Vector2D scale = Vector2D(1.0f, 1.0f);

        Transform(Vector2D pos = Vector2D()) : position(pos) {}
    };

    struct Movement : public Component<Movement>
    {
        float moveSpeed = 100.0f;
        Vector2D direction;
    };

    struct Sprite : public Component<Sprite>
    {
        std::string texturePath;
        int layer = 0;
        bool flipX = false;
        bool flipY = false;
    };

    struct BoxCollider : public Component<BoxCollider>
    {
        Vector2D size;
        Vector2D offset;
        bool isTrigger = false;
    };

    struct TriangleCollider : public Component<TriangleCollider>
    {
        Vector2D v0, v1, v2;  // Three vertices relative to transform position

        TriangleCollider(Vector2D vertex0 = Vector2D(-0.25f, -0.25f),
            Vector2D vertex1 = Vector2D(0.25f, -0.25f),
            Vector2D vertex2 = Vector2D(0.0f, 0.25f))
            : v0(vertex0), v1(vertex1), v2(vertex2) {
        }
    };

    struct Player : public Component<Player>
    {
        int health = 100;
    };
}