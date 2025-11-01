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
#include <vector>
#include <unordered_map>
#include <memory>
#include <typeindex>

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

    private:
        EntityID nextEntityID; //Next entitiy id to assign
        std::vector<EntityID> freeEntityIDs; //Recycled entity ids
        std::vector<Entity> allEntities; //All active entities

        /**
         * @brief Component storage
         *
         * Nested map structure:
         * - Outer map: Component type -> Component instances for that type
         * - Inner map: Entity ID -> Component instance
         *
         * This sparse storage allows efficient component access and
         * iteration over entities with specific component types.
         */
        std::unordered_map<std::type_index,
            std::unordered_map<EntityID, std::unique_ptr<ComponentBase>>> components;
    };

    // Template implementations must be in header
    template<typename T, typename... Args>
    T& EntityManager::AddComponent(Entity entity, Args&&... args)
    {
        std::type_index typeIndex(typeid(T));
        auto component = std::make_unique<T>(std::forward<Args>(args)...);
        T* ptr = component.get();
        components[typeIndex][entity.GetID()] = std::move(component);
        return *ptr;
    }

    template<typename T>
    T& EntityManager::GetComponent(Entity entity)
    {
        std::type_index typeIndex(typeid(T));
        return *static_cast<T*>(components[typeIndex][entity.GetID()].get());
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
        components[typeIndex].erase(entity.GetID());
    }
}