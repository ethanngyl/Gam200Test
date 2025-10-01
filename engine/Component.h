#pragma once
#include "../ECSComponent.h"
#include "../Math/Vector2D.h"
#include <string>

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

    struct Player : public Component<Player>
    {
        int health = 100;
    };
}