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
            LOG_INFO("EntityManager", "Created entity ID %u (recycled from free list, %zu free IDs remaining)", id, freeEntityIDs.size());
        }
        else
        {
            id = nextEntityID++;
            LOG_INFO("EntityManager", "Created entity ID %u (new, next ID will be %u)", id, nextEntityID);
        }

        Entity entity(id);
        allEntities.push_back(entity);
        LOG_INFO("EntityManager", "  Total entities now: %zu", allEntities.size());
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


    /**
     * @brief Delete all entities and components
     * @author jiahao.zhou@digipen.edu
	 * add this function to clear all entities and components in the entity manager
     * require this function for loading scenes in ImGui system
     */
    void EntityManager::ClearAllEntities()
    {
        size_t entityCount = allEntities.size();
        size_t componentTypeCount = components.size();
        size_t freeIDCount = freeEntityIDs.size();

        LOG_INFO("EntityManager", "===== CLEARING ALL ENTITIES =====");
        LOG_INFO("EntityManager", "Before clear:");
        LOG_INFO("EntityManager", "  - Entities: %zu", entityCount);
        LOG_INFO("EntityManager", "  - Component types: %zu", componentTypeCount);
        LOG_INFO("EntityManager", "  - Free IDs: %zu", freeIDCount);
        LOG_INFO("EntityManager", "  - Next entity ID: %u", nextEntityID);

        components.clear();
        allEntities.clear();
        freeEntityIDs.clear();
        nextEntityID = 1;

        LOG_INFO("EntityManager", "After clear:");
        LOG_INFO("EntityManager", "  - Entities: %zu", allEntities.size());
        LOG_INFO("EntityManager", "  - Component types: %zu", components.size());
        LOG_INFO("EntityManager", "  - Free IDs: %zu", freeEntityIDs.size());
        LOG_INFO("EntityManager", "  - Next entity ID: %u", nextEntityID);
        LOG_INFO("EntityManager", "===== CLEAR COMPLETE =====");
	}
}