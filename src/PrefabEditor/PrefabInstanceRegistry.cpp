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

 Copyright (C) 2025 DigiPen Institute of Technology.
 Reproduction or disclosure of this file or its contents
 without the prior written consent of DigiPen Institute of
 Technology is prohibited.
===============================================================================
*/

#include "PrefabInstanceRegistry.h"
#include <algorithm>    // std::remove_if

namespace Framework
{
    /**
     * @brief Access the global singleton instance.
     *
     * This uses the "Meyers singleton" pattern. The static local variable
     * is created the first time this function is called and reused on
     * subsequent calls.
     */
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

        // ---------------------------------------------------------------------
        // STEP 1: If this entity was already registered, remove its old mapping
        // ---------------------------------------------------------------------
        //
        // entityToPrefab keeps a reverse lookup from entityID -> prefabPath.
        // If we find an entry here, it means this entity was previously tracked
        // as belonging to some prefab (oldPath).
        auto oldIt = entityToPrefab.find(id);
        if (oldIt != entityToPrefab.end())
        {
            const std::string& oldPath = oldIt->second;
            auto vecIt = prefabToEntities.find(oldPath);
            if (vecIt != prefabToEntities.end())
            {
                auto& vec = vecIt->second;
                // Remove this entity from the old prefab's instance list.
                // We use remove_if + erase to get rid of any Entity whose
                // ID matches 'id'.
                vec.erase(
                    std::remove_if(vec.begin(), vec.end(),
                        [id](const Entity& e) { return e.GetID() == id; }),
                    vec.end()
                );
            }
        }

        // ---------------------------------------------------------------------
        // STEP 2: Add the new mapping
        // ---------------------------------------------------------------------

        // 1) Update reverse lookup: entityID -> prefabPath
        entityToPrefab[id] = prefabPath;
        // 2) Append to forward lookup: prefabPath -> list of entities
        //    If prefabPath does not exist yet, operator[] creates a new entry.
        prefabToEntities[prefabPath].push_back(entity);
    }

    void PrefabInstanceRegistry::UnregisterInstance(Entity entity)
    {
        // Ignore invalid entities.
        if (!entity.IsValid())
            return;

        const unsigned id = entity.GetID();

        // First, check if this entity is even tracked. If not, we are done.
        auto it = entityToPrefab.find(id);
        if (it == entityToPrefab.end())
            return;

        // Remember which prefab this entity was associated with
        const std::string& path = it->second;

        // Remove the reverse mapping: entityID -> prefabPath
        entityToPrefab.erase(it);

        // Next, remove this entity from the prefab's instance list
        auto vecIt = prefabToEntities.find(path);
        if (vecIt != prefabToEntities.end())
        {
            auto& vec = vecIt->second;
            // Remove any Entity whose ID matches 'id' from the vector
            vec.erase(
                std::remove_if(vec.begin(), vec.end(),
                    [id](const Entity& e) { return e.GetID() == id; }),
                vec.end()
            );
        }
    }

    std::vector<Entity> PrefabInstanceRegistry::GetInstancesOf(const std::string& prefabPath) const
    {
        // Look up the vector of entities for this prefab
        auto it = prefabToEntities.find(prefabPath);
        if (it == prefabToEntities.end())
            return {};// No entry: return empty vector

        return it->second; // Return a copy so callers can modify it without affecting the registry.
    }

    std::string PrefabInstanceRegistry::GetPrefabOf(Entity entity) const
    {
        // Invalid entity => no associated prefab
        if (!entity.IsValid())
            return {};

        // Check if we have a mapping for this entity ID
        const unsigned id = entity.GetID();
        auto it = entityToPrefab.find(id);
        if (it == entityToPrefab.end())
            return {};// Unknown entity

        // Return the prefab path string
        return it->second;
    }

    void PrefabInstanceRegistry::Clear()
    {
        // Drop all mappings in both directions. Typically used when
        // switching levels/scenes so we don't keep stale data.
        // Force deallocation using swap trick - don't call clear() first
        std::unordered_map<unsigned, std::string>().swap(entityToPrefab);
        std::unordered_map<std::string, std::vector<Entity>>().swap(prefabToEntities);
    }

} // namespace Framework
