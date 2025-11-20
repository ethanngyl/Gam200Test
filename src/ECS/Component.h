/**
===============================================================================
 File:           Component.h
 Author:         ETHAN NG YONG LE
 Email:          n.ethanyongle@digipen.edu
 Date:           2025-09-30
 Contribution:   100%
 ------------------------------------------------------------------------------
 
  Design notes:
  Contains all component data structures used in the Entity Component System.
 * Components are pure data containers with no behavior - systems operate on them.
===============================================================================
 */

#pragma once
#include "ECSComponent.h"
#include "Vector2D.h"
#include <string>

extern "C" {
    struct lua_State;
    void lua_close(struct lua_State* L);  
}

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

        float upperLimit = 0.0f;
        float lowerLimit = 0.0f;

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

    struct ProjectileMovement : public Component<Movement>
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

    struct AudioSource : public Component<AudioSource> {
        std::string soundName;
        bool isPlaying = false;
        bool loop = false;
        float volume = 1.0f;
        float pitch = 1.0f;
        bool playOnStart = false;

        void* fmodChannel = nullptr;
    };

    struct TagComponent {
        std::string tag;           // e.g., "Player", "Enemy", "Collectible"
        std::vector<std::string> groups;  // Multiple groups: {"Damageable", "Physics"}

        TagComponent() = default;
        TagComponent(const std::string& t) : tag(t) {}
    };

    struct ScriptComponent : public Component<ScriptComponent>
    {
        std::string scriptPath;
        lua_State* L = nullptr; 
        bool initialized = false;
        float updateTimer = 0.0f;

        bool hasOnInit = false;
        bool hasOnUpdate = false;
        bool hasOnDestroy = false;

        ScriptComponent() = default;
        ScriptComponent(const std::string& path) : scriptPath(path) {}
        ~ScriptComponent() {
            if (L) {
                lua_close(L);
                L = nullptr;
            }
        }

        // Prevent copying
        ScriptComponent(const ScriptComponent&) = delete;
        ScriptComponent& operator=(const ScriptComponent&) = delete;

        // Move constructor
        ScriptComponent(ScriptComponent&& other) noexcept
            : scriptPath(std::move(other.scriptPath))
            , L(other.L)
            , initialized(other.initialized)
            , updateTimer(other.updateTimer)
            , hasOnInit(other.hasOnInit)
            , hasOnUpdate(other.hasOnUpdate)
            , hasOnDestroy(other.hasOnDestroy)
        {
            other.L = nullptr;
        }

        // Move assignment
        ScriptComponent& operator=(ScriptComponent&& other) noexcept {
            if (this != &other) {
                if (L) lua_close(L);
                scriptPath = std::move(other.scriptPath);
                L = other.L;
                initialized = other.initialized;
                updateTimer = other.updateTimer;
                hasOnInit = other.hasOnInit;
                hasOnUpdate = other.hasOnUpdate;
                hasOnDestroy = other.hasOnDestroy;
                other.L = nullptr;
            }
            return *this;
        }
    };

    /**
     * @brief Health component for entities that can take damage
     */
    struct Health : public Component<Health> {
        int maxHealth = 50;
        int currentHealth = 50;
        bool isDead = false;

        Health() = default;
        Health(int max) : maxHealth(max), currentHealth(max) {}

        void TakeDamage(int damage) {
            currentHealth -= damage;
            if (currentHealth <= 0) {
                currentHealth = 0;
                isDead = true;
            }
        }

        void Heal(int amount) {
            currentHealth += amount;
            if (currentHealth > maxHealth) {
                currentHealth = maxHealth;
            }
        }

        float GetHealthPercent() const {
            return static_cast<float>(currentHealth) / static_cast<float>(maxHealth);
        }
    };

    struct AP : public Component<AP> {
        int hp = 3;
        int maxHp = 3;
        int actionPoints = 3;
        int maxActionPoints = 3;

        AP(int startHp = 3, int startAp = 3) : hp(startHp), maxHp(startHp), actionPoints(startAp), maxActionPoints(startAp) {

        }
    };

    struct AttackRangeComponent : public Component<AttackRangeComponent> {
        int minRange = 1;        // Minimum attack distance (in grid cells)
        int maxRange = 3;        // Maximum attack distance (in grid cells)
        bool showRange = false;  // Toggle visibility

        AttackRangeComponent() = default;
        AttackRangeComponent(int min, int max) : minRange(min), maxRange(max) {}
    };

}