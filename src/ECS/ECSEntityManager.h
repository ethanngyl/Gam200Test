/**
===============================================================================
 File:           ECSEntityManager.h
 Author:         ETHAN NG YONG LE
 Email:          n.ethanyongle@digipen.edu
 Date:           2025-09-30
 Contribution:   100%
 ------------------------------------------------------------------------------
 [A brief, one-sentence description of the file's primary purpose.]
 
  Design notes:
  Provides the EntityManager class which handles entity lifecycle,
 * component attachment/detachment, and component storage/retrieval.
 * This is the central coordination point for the ECS system.
===============================================================================
 */

#pragma once
#include "ECSEntity.h"
#include "ECSComponent.h"
#include "Memory/MemoryManager.h"
#include <vector>
#include <unordered_map>
#include <typeindex>
#include <stdexcept>

namespace Framework
{
    /**
     * @class EntityManager
     * @brief Manages entities and their associated components
     *
     * The EntityManager is responsible for:
     * - Creating and destroying entities
     * - Attaching and removing components from entities
     * - Storing and retrieving component data
     * - Querying entities by component types
     *
     * Components are stored in a sparse map structure for efficient
     * access and iteration. Entity IDs are recycled when entities
     * are destroyed to prevent ID exhaustion.
     */
    class EntityManager
    {
    public:
        /**
         * @brief Constructs an EntityManager with initial ID of 1
         */
        EntityManager() : nextEntityID(1) {}

        /**
         * @brief Creates a new entity with a unique ID
         * @return Newly created entity handle
         *
         * Reuses freed entity IDs if available, otherwise assigns
         * the next sequential ID.
         */
        Entity CreateEntity();

        /**
         * @brief Destroys an entity and all its components
         * @param entity The entity to destroy
         *
         * Removes all components associated with the entity and
         * marks its ID for reuse.
         */
        void DestroyEntity(Entity entity);

        /**
         * @brief Adds a component to an entity
         * @tparam T Component type to add
         * @tparam Args Constructor argument types
         * @param entity Entity to add component to
         * @param args Arguments forwarded to component constructor
         * @return Reference to the newly created component
         *
         * Constructs a component of type T in-place with perfect
         * forwarding of constructor arguments.
         */
        template<typename T, typename... Args>
        T& AddComponent(Entity entity, Args&&... args);

        /**
         * @brief Retrieves a component from an entity
         * @tparam T Component type to retrieve
         * @param entity Entity to get component from
         * @return Reference to the component
         *
         * @warning Undefined behavior if component doesn't exist.
         *          Use HasComponent() to check first.
         */
        template<typename T>
        T& GetComponent(Entity entity);

        /**
         * @brief Checks if an entity has a specific component type
         * @tparam T Component type to check for
         * @param entity Entity to check
         * @return True if the entity has the component
         */
        template<typename T>
        bool HasComponent(Entity entity);

        /**
         * @brief Removes a component from an entity
         * @tparam T Component type to remove
         * @param entity Entity to remove component from
         *
         * Does nothing if the component doesn't exist.
         */
        template<typename T>
        void RemoveComponent(Entity entity);

        /**
         * @brief Gets all currently active entities
         * @return Vector of all entity handles
         *
         * Used by systems to iterate over entities for processing.
         */
        std::vector<Entity> GetAllEntities() const { return allEntities; }

        /**
		 * @brief Delete all entities and components
         * @author jiahao.zhou@digipen.edu
         * add this function to clear all entities and components in the entity manager
         * require this function for loading scenes in ImGui system
         */
        void ClearAllEntities();
        void ResetEntityIDCounter() {
            nextEntityID = 1; // Or whatever your starting ID is
        }

    private:
        EntityID nextEntityID; //Next entitiy id to assign
        std::vector<EntityID> freeEntityIDs; //Recycled entity ids
        std::vector<Entity> allEntities; //All active entities

        /**
         * @brief Component storage (managed by custom MemoryManager)
         *
         * Nested map structure:
         * - Outer map: Component type -> Component instances for that type
         * - Inner map: Entity ID -> Component pointer (pool-allocated)
         *
         * Components are allocated from the MemoryManager's pool allocators
         * using placement new, NOT from the default C++ heap. Raw pointers
         * are used instead of std::unique_ptr because lifetime is managed
         * explicitly through the MemoryManager.
         */
        std::unordered_map<std::type_index,
            std::unordered_map<EntityID, ComponentBase*>> components;
    };

    // Template implementations must be in header
    template<typename T, typename... Args>
    T& EntityManager::AddComponent(Entity entity, Args&&... args)
    {
        std::type_index typeIndex(typeid(T));

        // Check if this entity already has this component type - if so,
        // deallocate the old one through the MemoryManager first
        auto& entityMap = components[typeIndex];
        auto it = entityMap.find(entity.GetID());
        if (it != entityMap.end() && it->second != nullptr)
        {
            ComponentBase* old = it->second;
            size_t allocSize = old->GetAllocatedSize();
            old->~ComponentBase();  // Call virtual destructor
            MemoryManager::GetInstance().DeallocateBySize(old, allocSize);
        }

        // Allocate from the MemoryManager using placement new
        // This gets a block from the pre-allocated pool (O(1), no OS call)
        // and constructs the component in-place.
        T* ptr = MemoryManager::GetInstance().Allocate<T>(
            std::forward<Args>(args)...
        );
        entityMap[entity.GetID()] = ptr;
        return *ptr;
    }

    template<typename T>
    T& EntityManager::GetComponent(Entity entity)
    {
        std::type_index typeIndex(typeid(T));
        auto typeIt = components.find(typeIndex);

        // Check if component type exists
        if (typeIt == components.end()) {
            throw std::runtime_error("GetComponent: Component type does not exist");
        }

        auto entityIt = typeIt->second.find(entity.GetID());

        // Check if entity has this component
        if (entityIt == typeIt->second.end()) {
            throw std::runtime_error("GetComponent: Entity does not have this component");
        }

        return *static_cast<T*>(entityIt->second);
    }

    template<typename T>
    bool EntityManager::HasComponent(Entity entity)
    {
        std::type_index typeIndex(typeid(T));
        auto it = components.find(typeIndex);
        if (it == components.end()) return false;
        return it->second.find(entity.GetID()) != it->second.end();
    }

    template<typename T>
    void EntityManager::RemoveComponent(Entity entity)
    {
        std::type_index typeIndex(typeid(T));
        auto it = components.find(typeIndex);
        if (it != components.end()) {
            auto entityIt = it->second.find(entity.GetID());
            if (entityIt != it->second.end()) {
                // Deallocate through the MemoryManager:
                // 1. Call virtual destructor (cleans up object state)
                // 2. Return block to pool free list (O(1), no OS call)
                ComponentBase* comp = entityIt->second;
                if (comp) {
                    size_t allocSize = comp->GetAllocatedSize();
                    comp->~ComponentBase();
                    MemoryManager::GetInstance().DeallocateBySize(comp, allocSize);
                }
                it->second.erase(entityIt);
            }
        }
    }
}