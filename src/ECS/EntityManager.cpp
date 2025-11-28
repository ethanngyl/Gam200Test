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
#include <iostream>
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
            std::cout << "[EntityManager] Created entity ID " << id << " (recycled from free list, " << freeEntityIDs.size() << " free IDs remaining)\n";
        }
        else
        {
            id = nextEntityID++;
            std::cout << "[EntityManager] Created entity ID " << id << " (new, next ID will be " << nextEntityID << ")\n";
        }

        Entity entity(id);
        allEntities.push_back(entity);
        std::cout << "[EntityManager]   Total entities now: " << allEntities.size() << "\n";
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

        std::cout << "[EntityManager] ===== CLEARING ALL ENTITIES =====\n";
        std::cout << "[EntityManager] Before clear:\n";
        std::cout << "[EntityManager]   - Entities: " << entityCount << "\n";
        std::cout << "[EntityManager]   - Component types: " << componentTypeCount << "\n";
        std::cout << "[EntityManager]   - Free IDs: " << freeIDCount << "\n";
        std::cout << "[EntityManager]   - Next entity ID: " << nextEntityID << "\n";

        components.clear();
        allEntities.clear();
        freeEntityIDs.clear();
        nextEntityID = 1;

        std::cout << "[EntityManager] After clear:\n";
        std::cout << "[EntityManager]   - Entities: " << allEntities.size() << "\n";
        std::cout << "[EntityManager]   - Component types: " << components.size() << "\n";
        std::cout << "[EntityManager]   - Free IDs: " << freeEntityIDs.size() << "\n";
        std::cout << "[EntityManager]   - Next entity ID: " << nextEntityID << "\n";
        std::cout << "[EntityManager] ===== CLEAR COMPLETE =====\n";
	}
}