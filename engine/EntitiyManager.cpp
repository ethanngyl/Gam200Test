#include "Precompiled.h"
#include "ECSEntityManager.h"

namespace Framework
{
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