#pragma once
#include "ECSEntity.h"
#include "ECSComponent.h"
#include "Precompiled.h"


namespace Framework
{
    class EntityManager
    {
    public:
        EntityManager() : nextEntityID(1) {}


        Entity CreateEntity();
        void DestroyEntity(Entity entity);

        template<typename T, typename... Args>
        T& AddComponent(Entity entity, Args&&... args);

        template<typename T>
        T& GetComponent(Entity entity);

        template<typename T>
        bool HasComponent(Entity entity);

        template<typename T>
        void RemoveComponent(Entity entity);

        // Query methods
        std::vector<Entity> GetAllEntities() const { return allEntities; }

    private:
        EntityID nextEntityID;
        std::vector<EntityID> freeEntityIDs;
        std::vector<Entity> allEntities;

        // Component storage: ComponentType -> EntityID -> Component
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