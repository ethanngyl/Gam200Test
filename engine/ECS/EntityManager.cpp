/**
 ===============================================================================
 File:           EntityManager.cpp
 Author:         ETHAN NG YONG LE
 Email:          n.ethanyongle@digipen.edu
 Date:           2025-09-30
 Contribution:   100%
 ------------------------------------------------------------------------------
 
  Design notes:
  Provides the EntityManager class which handles entity lifecycle,
 * component attachment/detachment, and component storage/retrieval.
 * This is the central coordination point for the ECS system.
===============================================================================
 */

#include "ECSEntityManager.h"

namespace Framework
{
    /**
     * @brief Creates a new entity with a unique ID
     * @return Newly created entity handle
     *
     * Implementation details:
     * - Reuses recycled IDs from destroyed entities if available
     * - Otherwise assigns the next sequential ID
     * - Adds the entity to the active entity list
     *
     * ID recycling prevents overflow in long-running applications
     * where entities are frequently created and destroyed.
     */
    Entity EntityManager::CreateEntity()
    {
        EntityID id;
        if (!freeEntityIDs.empty())
        {
            id = freeEntityIDs.back();
            freeEntityIDs.pop_back();
        }
        else
        {
            id = nextEntityID++;
        }

        Entity entity(id);
        allEntities.push_back(entity);
        return entity;
    }

    /**
     * @brief Destroys an entity and all its components
     * @param entity The entity to destroy
     *
     * Performs three cleanup operations:
     * 1. Removes all components associated with the entity
     * 2. Removes the entity from the active entity list
     * 3. Marks the entity ID for reuse
     *
     * Uses erase-remove idiom for efficient vector element removal.
     * After destruction, the entity ID becomes invalid and can be
     * reassigned to new entities.
     */

    void EntityManager::DestroyEntity(Entity entity)
    {
        EntityID id = entity.GetID();

        // Remove all components for this entity
        for (auto& [type, componentMap] : components)
        {
            componentMap.erase(id);
        }

        // Remove from active entities
        allEntities.erase(
            std::remove_if(allEntities.begin(), allEntities.end(),
                [id](const Entity& e) { return e.GetID() == id; }),
            allEntities.end()
        );

        // Recycle the ID
        freeEntityIDs.push_back(id);
    }
}