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
#include "ResourceHandle.h"   // <-- REQUIRED for TextureHandle
#include <glm/glm.hpp>         // <-- REQUIRED for glm::vec4
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
        int damage = 10;       // Configurable damage dealt on hit
        bool pierce = false;   // If true, passes through enemies instead of stopping
        bool isEnemyProjectile = false;  // If true, damages players instead of enemies
        uint32_t sourceEntityID = 0;     // Entity that spawned this projectile (skip self-hit)
    };

    enum class AnimGroup {
        Idle,
        Walk,
        Attack,
        Injured,
        Death,
        Count
    };

    enum class AnimDirection {
        Front,
        Back,
        Side,
        None
    };

    struct SpriteAnimation : public Component<SpriteAnimation>
    {
        AnimGroup group = AnimGroup::Idle;
        AnimDirection direction = AnimDirection::Front;

        /** The animation name that maps to JSON key */
        std::string animName;     // final JSON animation key

        /** Handle to the full sprite sheet */
        TextureHandle spriteSheet;

        /** Frame stepping */
        int currentFrame = 0;
        float elapsedTime = 0.0f;

        /** Animation data (ALL data-driven from JSON) */
        int startFrame = 0;   // First frame in the animation range
        int frameCount = 1;   // Number of frames in the animation
        int rows = 1;
        int columns = 1;
        float frameTime = 0.1f;

        /** Frame layout */
        int frameWidth = 0;
        int frameHeight = 0;

        /** UV correction + flipping */
        float uvShrinkPx = 0.0f;
        bool loop = true;
        bool playing = true;
        bool flipX = false;
        
        /** If false, skip JSON-based animation selection (for UI elements, etc.) */
        bool useJsonConfig = true;
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
        glm::vec4 tint = glm::vec4(1.0f);  // Color tint multiplier (RGBA)
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

    struct TagComponent : public Component<TagComponent> {
        std::string tag;           // e.g., "Player", "Enemy", "Collectible"

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
        int actionPoints = 3;
        int maxActionPoints = 3;

        AP() = default;
		AP(int maxAP) : actionPoints(maxAP), maxActionPoints(maxAP) {}
    };

    struct AttackRangeComponent : public Component<AttackRangeComponent> {
        int minRange = 1;        // Minimum attack distance (in grid cells)
        int maxRange = 3;        // Maximum attack distance (in grid cells)
        bool showRange = false;  // Toggle visibility

        AttackRangeComponent() = default;
        AttackRangeComponent(int min, int max) : minRange(min), maxRange(max) {}
    };

    /**
 * @brief Chest component - collectible items that block enemies
 */
    struct Chest : public Component<Chest> {
        bool collected = false;     // Has player collected this chest?
        int chestID = 0;            // Unique ID for this chest

        Chest() = default;
        Chest(int id) : chestID(id) {}
    };

    /**
     * @brief Goal component - level exit that checks for chest completion
     */
    struct Goal : public Component<Goal> {
        int chestsRequired = 0;     // How many chests needed to complete level
        bool canExit = false;       // Can player exit now?

        Goal() = default;
        Goal(int required) : chestsRequired(required) {}
    };

    /**
     * @brief Player inventory - tracks collected chests
     */
    struct Inventory : public Component<Inventory> {
        std::vector<int> collectedChests;  // IDs of collected chests

        int GetChestCount() const { return static_cast<int>(collectedChests.size()); }

        bool HasChest(int chestID) const {
            return std::find(collectedChests.begin(), collectedChests.end(), chestID)
                != collectedChests.end();
        }

        void AddChest(int chestID) {
            if (!HasChest(chestID)) {
                collectedChests.push_back(chestID);
            }
        }
    };

    struct AttackAP : public Component<AttackAP> {
        int points = 1; //current attack points
        int maxPoints = 3; //max attack points per player turn

        AttackAP(int start = 1) : points(start), maxPoints(start) {}
    };

    /**
     * @brief Status effects component - tracks active buffs/debuffs on an entity
     *
     * Effect types:
     *   "guard"       - blocks next damage instance (duration -1 = until consumed)
     *   "parry"       - reflects next damage to attacker (duration -1 = until consumed)
     *   "stun"        - skip next turn(s)
     *   "vulnerable"  - take extra damage from all sources
     *   "knightsOath" - damage redirected to sourceEntity
     */
    struct StatusEffects : public Component<StatusEffects> {
        struct Effect {
            std::string type;
            int turnsRemaining;       // -1 = permanent / until consumed
            uint32_t sourceEntity;    // who applied this effect
            uint32_t targetEntity;    // for knightsOath: which ally is protected
            int extraData;            // for vulnerable: +N extra damage
        };

        std::vector<Effect> effects;

        bool HasEffect(const std::string& type) const {
            for (const auto& e : effects) {
                if (e.type == type) return true;
            }
            return false;
        }

        const Effect* GetEffect(const std::string& type) const {
            for (const auto& e : effects) {
                if (e.type == type) return &e;
            }
            return nullptr;
        }

        void AddEffect(const std::string& type, int turns, uint32_t source = 0,
                        uint32_t target = 0, int extra = 0) {
            // Remove existing effect of same type before adding
            RemoveEffect(type);
            effects.push_back({type, turns, source, target, extra});
        }

        void RemoveEffect(const std::string& type) {
            effects.erase(
                std::remove_if(effects.begin(), effects.end(),
                    [&](const Effect& e) { return e.type == type; }),
                effects.end());
        }

        void DecrementTurns() {
            for (auto& e : effects) {
                if (e.turnsRemaining > 0) {
                    e.turnsRemaining--;
                }
            }
            // Remove expired (reached 0)
            effects.erase(
                std::remove_if(effects.begin(), effects.end(),
                    [](const Effect& e) { return e.turnsRemaining == 0; }),
                effects.end());
        }
    };

} // namespace Framework