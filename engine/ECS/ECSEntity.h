/**
===============================================================================
 File:           ECSEntity.h
 Author:         ETHAN NG YONG LE
 Email:          n.ethanyongle@digipen.edu
 Date:           2025-09-30
 Contribution:   100%
 ------------------------------------------------------------------------------
 [A brief, one-sentence description of the file's primary purpose.]
 
  Design notes:
  Defines the Entity class which serves as a lightweight identifier
 * for game objects in the Entity Component System. Entities are just
 * IDs - the actual data lives in components.
===============================================================================
 */

#pragma once
#include <cstdint>

namespace Framework
{
    using EntityID = uint32_t;
    constexpr EntityID INVALID_ENTITY = 0;

    /**
     * @class Entity
     * @brief Lightweight identifier for game objects in ECS
     *
     * An Entity is simply a unique ID that serves as a handle to a
     * collection of components. Entities themselves contain no data
     * or behavior - they're just identifiers that systems use to
     * look up associated components.
     *
     * Valid entity IDs start at 1; 0 is reserved for INVALID_ENTITY.
     *
     * Example usage:
     * @code
     * Entity player = entityManager.CreateEntity();
     * entityManager.AddComponent<Transform>(player, Vector2D(0, 0));
     * @endcode
     */
    class Entity
    {
    public:
        /**
         * @brief Default constructor creates an invalid entity
         */
        Entity() : id(INVALID_ENTITY) {}

        /**
         * @brief Constructs an entity with the specified ID
         * @param entityId Unique identifier for this entity
         */
        Entity(EntityID entityId) : id(entityId) {}

        /**
         * @brief Gets the entity's unique identifier
         * @return The entity's ID
         */
        EntityID GetID() const { return id; }
        /**
         * @brief Checks if this entity is valid
         * @return True if the entity has a valid ID (non-zero)
         */
        bool IsValid() const { return id != INVALID_ENTITY; }

        /**
         * @brief Equality comparison operator
         * @param other Entity to compare with
         * @return True if both entities have the same ID
         */
        bool operator==(const Entity& other) const { return id == other.id; }
        /**
         * @brief Inequality comparison operator
         * @param other Entity to compare with
         * @return True if entities have different IDs
         */
        bool operator!=(const Entity& other) const { return id != other.id; }

    private:
        EntityID id; //Unique identifier for the entity
    };
}