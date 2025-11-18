/*
===============================================================================
File:        PrefabInstanceRegistry.h
Author:      Sim Kah Yan
Email:       kahyan.sim@digipen.edu
Date:        2025-11-17
Contribution: 100%
-------------------------------------------------------------------------------
Brief:
Central authority for tracking all entities that were spawned from prefab files.

Design notes:
- Singleton-style registry: PrefabInstanceRegistry::Get()
- Tracks both directions:
    • entityID           -> prefabPath
    • prefabPath (file)  -> vector<Entity> instances
- Used by:
    • PrefabSerializer::LoadPrefab() to RegisterInstance(...)
    • (Optional) EntityManager::DestroyEntity() to UnregisterInstance(...)
    • Editor/tools to query all instances when a prefab changes.
===============================================================================
*/

#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include "ECSEntity.h"   // Framework::Entity

namespace Framework
{
    /**
     * @brief Central registry for prefab instances.
     *
     * Whenever you spawn an entity from a prefab file (e.g. "platform.prefab"),
     * you should register it here. This allows tools to later query:
     *  - "Give me all entities that came from platform.prefab"
     *  - "Which prefab did this entity come from?"
     */
    class PrefabInstanceRegistry
    {
    public:
        /**
         * @brief Global access to the singleton instance.
         */
        static PrefabInstanceRegistry& Get();

        /**
         * @brief Register an entity as an instance of a prefab.
         *
         * If the entity was already registered with a different prefab, the
         * old mapping is removed and replaced with the new one.
         *
         * @param entity     Entity spawned from a prefab.
         * @param prefabPath Path to the prefab file (e.g. "platform.prefab").
         */
        void RegisterInstance(Entity entity, const std::string& prefabPath);

        /**
         * @brief Remove an entity from the registry.
         *
         * Call this when an entity is destroyed so the registry doesn't keep
         * stale references.
         */
        void UnregisterInstance(Entity entity);

        /**
         * @brief Get all entities that were spawned from a specific prefab.
         *
         * Returns a copy of the internal vector so callers can safely mutate
         * their own copy without affecting the registry.
         */
        std::vector<Entity> GetInstancesOf(const std::string& prefabPath) const;

        /**
         * @brief Query which prefab file this entity came from.
         *
         * @return prefab path if known, or empty string if not registered.
         */
        std::string GetPrefabOf(Entity entity) const;

        /**
         * @brief Clear all tracking data.
         *
         * Useful when changing levels/scenes.
         */
        void Clear();

    private:
        PrefabInstanceRegistry() = default;
        ~PrefabInstanceRegistry() = default;

        // Non-copyable
        PrefabInstanceRegistry(const PrefabInstanceRegistry&) = delete;
        PrefabInstanceRegistry& operator=(const PrefabInstanceRegistry&) = delete;

        // entityID -> prefabPath
        std::unordered_map<unsigned, std::string> entityToPrefab;

        // prefabPath -> entities
        std::unordered_map<std::string, std::vector<Entity>> prefabToEntities;
    };

} // namespace Framework
