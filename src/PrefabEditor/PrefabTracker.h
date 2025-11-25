/*
===============================================================================
File:        PrefabTracker.h
Author:      Sim Kah Yan
Email:       kahyan.sim@digipen.edu
Date:        2025-11-07
Contribution: 100%
-------------------------------------------------------------------------------
Brief:
Central registry for tracking all active prefab instances in the engine.
Whenever a prefab is loaded, this tracker records the entity → prefab
relationship so that editor tools and runtime systems can locate, update, or
synchronize all instances that originate from the same prefab asset.

Details:
- RegisterInstance(): Records that an entity was created from a specific prefab
  file path. Stored in both directions (entity → prefab, and prefab → list of
  entities).
- UnregisterInstance(): Removes an entity from the registry when destroyed,
  keeping lists clean and avoiding stale references.
- GetInstancesOf(): Returns all entities created from a given prefab file.
- GetPrefabOf(): Returns the prefab file path that an entity came from.
- Clear(): Resets the entire registry, typically used when changing scenes.

Notes:
- Implemented as a lightweight singleton (static Get()) for global access.
- Used by PrefabSerializer::LoadPrefab() to automatically track instances.
- Enables future features such as: automatic prefab reapply, batch editing,
  inspector syncing, and prefab-based scene updates.
===============================================================================
*/

#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include "ECSEntity.h"       // for Framework::Entity

namespace Framework
{
    /**
     * @brief Central registry that tracks all prefab instances in the game.
     *
     * Every time you spawn an entity from a prefab file, you should register it
     * here so that later you can find all instances of a given prefab and
     * update them.
     *
     * Usage:
     *   PrefabInstanceRegistry::Get().RegisterInstance(entity, "platform.prefab");
     *   auto instances = PrefabInstanceRegistry::Get().GetInstancesOf("platform.prefab");
     */
    class PrefabInstanceTracker
    {
    public:
        // Singleton-style access
        static PrefabInstanceTracker& Get()
        {
            static PrefabInstanceTracker instance;
            return instance;
        }

        // Called when an entity is created FROM a prefab
        void RegisterInstance(Entity e, const std::string& prefabPath)
        {
            if (!e.IsValid()) return;

            const unsigned id = e.GetID();

            entityToPrefab[id] = prefabPath;
            prefabToEntities[prefabPath].push_back(e);
        }

        // Called when an entity is destroyed (optional, but nice to have)
        void UnregisterInstance(Entity e)
        {
            if (!e.IsValid()) return;

            const unsigned id = e.GetID();
            auto it = entityToPrefab.find(id);
            if (it == entityToPrefab.end()) return;

            const std::string path = it->second;
            entityToPrefab.erase(it);

            auto vecIt = prefabToEntities.find(path);
            if (vecIt != prefabToEntities.end())
            {
                auto& vec = vecIt->second;
                vec.erase(
                    std::remove_if(vec.begin(), vec.end(),
                        [id](Entity ent) { return ent.GetID() == id; }),
                    vec.end()
                );
            }
        }

        // Get all entities that are instances of a specific prefab file
        std::vector<Entity> GetInstancesOf(const std::string& prefabPath) const
        {
            auto it = prefabToEntities.find(prefabPath);
            if (it == prefabToEntities.end())
                return {};
            return it->second;
        }

        // Query: which prefab file did this entity come from?
        std::string GetPrefabOf(Entity e) const
        {
            if (!e.IsValid()) return {};
            const unsigned id = e.GetID();
            auto it = entityToPrefab.find(id);
            if (it == entityToPrefab.end()) return {};
            return it->second;
        }

        // Clear everything (e.g., when loading a new level)
        void Clear()
        {
            entityToPrefab.clear();
            prefabToEntities.clear();
        }

    private:
        PrefabInstanceTracker() = default;
        ~PrefabInstanceTracker() = default;

        // entityID -> prefabPath
        std::unordered_map<unsigned, std::string> entityToPrefab;

        // prefabPath -> entities
        std::unordered_map<std::string, std::vector<Entity>> prefabToEntities;
    };
}
