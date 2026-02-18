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

#include "Precompiled.h"
#include "ECSEntityManager.h"
#include "Memory/MemoryManager.h"
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
            // Debug logging removed for performance (was causing 20-30ms lag per frame)
            // std::cout << "[EntityManager] Created entity ID " << id << " (recycled from free list, " << freeEntityIDs.size() << " free IDs remaining)\n";
        }
        else
        {
            id = nextEntityID++;
            // Debug logging removed for performance
            // std::cout << "[EntityManager] Created entity ID " << id << " (new, next ID will be " << nextEntityID << ")\n";
        }

        Entity entity(id);
        allEntities.push_back(entity);
        // Debug logging removed for performance
        // std::cout << "[EntityManager]   Total entities now: " << allEntities.size() << "\n";
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

        // Remove all components for this entity, deallocating each through
        // the MemoryManager (returns blocks to pool free list, not to OS)
        for (auto& [type, componentMap] : components)
        {
            auto it = componentMap.find(id);
            if (it != componentMap.end())
            {
                ComponentBase* comp = it->second;
                if (comp)
                {
                    size_t allocSize = comp->GetAllocatedSize();
                    comp->~ComponentBase();  // Virtual destructor
                    MemoryManager::GetInstance().DeallocateBySize(comp, allocSize);
                }
                componentMap.erase(it);
            }
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
        // Deallocate ALL components through the MemoryManager.
        // This calls each component's virtual destructor and returns the
        // memory block to the pool's free list. The actual page memory
        // is NOT returned to the OS until MemoryManager::Shutdown().
        for (auto& [type, componentMap] : components)
        {
            for (auto& [entityId, comp] : componentMap)
            {
                if (comp)
                {
                    size_t allocSize = comp->GetAllocatedSize();
                    comp->~ComponentBase();  // Virtual destructor
                    MemoryManager::GetInstance().DeallocateBySize(comp, allocSize);
                }
            }
        }

        components.clear();
        allEntities.clear();
        freeEntityIDs.clear();
        nextEntityID = 1;
	}
}