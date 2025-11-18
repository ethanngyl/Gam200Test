/*
===============================================================================
File:        PrefabInstanceRegistry.cpp
Author:      Sim Kah Yan
Email:       kahyan.sim@digipen.edu
Date:        2025-11-17
Contribution: 100%
-------------------------------------------------------------------------------
Brief:
Implementation of PrefabInstanceRegistry, the central tracking system for
entities spawned from prefab files.
===============================================================================
*/

#include "PrefabInstanceRegistry.h"
#include <algorithm>    // std::remove_if

namespace Framework
{
    PrefabInstanceRegistry& PrefabInstanceRegistry::Get()
    {
        static PrefabInstanceRegistry instance;
        return instance;
    }

    void PrefabInstanceRegistry::RegisterInstance(Entity entity, const std::string& prefabPath)
    {
        if (!entity.IsValid())
            return;

        const unsigned id = entity.GetID();

        // If this entity was already registered to some prefab, remove it first
        auto oldIt = entityToPrefab.find(id);
        if (oldIt != entityToPrefab.end())
        {
            const std::string& oldPath = oldIt->second;
            auto vecIt = prefabToEntities.find(oldPath);
            if (vecIt != prefabToEntities.end())
            {
                auto& vec = vecIt->second;
                vec.erase(
                    std::remove_if(vec.begin(), vec.end(),
                        [id](const Entity& e) { return e.GetID() == id; }),
                    vec.end()
                );
            }
        }

        // Store new mapping
        entityToPrefab[id] = prefabPath;
        prefabToEntities[prefabPath].push_back(entity);
    }

    void PrefabInstanceRegistry::UnregisterInstance(Entity entity)
    {
        if (!entity.IsValid())
            return;

        const unsigned id = entity.GetID();
        auto it = entityToPrefab.find(id);
        if (it == entityToPrefab.end())
            return;

        const std::string& path = it->second;
        entityToPrefab.erase(it);

        auto vecIt = prefabToEntities.find(path);
        if (vecIt != prefabToEntities.end())
        {
            auto& vec = vecIt->second;
            vec.erase(
                std::remove_if(vec.begin(), vec.end(),
                    [id](const Entity& e) { return e.GetID() == id; }),
                vec.end()
            );
        }
    }

    std::vector<Entity> PrefabInstanceRegistry::GetInstancesOf(const std::string& prefabPath) const
    {
        auto it = prefabToEntities.find(prefabPath);
        if (it == prefabToEntities.end())
            return {};

        return it->second; // return a copy
    }

    std::string PrefabInstanceRegistry::GetPrefabOf(Entity entity) const
    {
        if (!entity.IsValid())
            return {};

        const unsigned id = entity.GetID();
        auto it = entityToPrefab.find(id);
        if (it == entityToPrefab.end())
            return {};

        return it->second;
    }

    void PrefabInstanceRegistry::Clear()
    {
        entityToPrefab.clear();
        prefabToEntities.clear();
    }

} // namespace Framework
