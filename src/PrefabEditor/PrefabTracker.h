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
