/*
===============================================================================
File:        PrefabSerializer.h
Author:      Sim Kah Yan
Email:       kahyan.sim@digipen.edu
Date:        2025-11-07
Contribution: 100%
-------------------------------------------------------------------------------
Brief:
Declaration of the PrefabSerializer module, responsible for saving and loading
entities and their components to/from JSON files. This enables prefab-based
workflows where reusable game objects can be authored, stored, and instantiated
at runtime.

Details:
- SavePrefab(): Serializes all components attached to a given entity
  (Transform, Sprite, MeshRenderer, Movement, Colliders, etc.) into a structured
  JSON file.
- LoadPrefab(): Reads a JSON file, creates a new entity, and reconstructs all
  component values exactly as they were saved.
- Integrates with PrefabInstanceTracker to register instantiated prefabs for
  editor updates and prefab-instance synchronization.

Notes:
- Uses nlohmann::json for serialization/deserialization.
- Produces human-readable JSON on disk for easier debugging and version control.
- Requires a valid EntityManager reference; this module does not create or own ECS systems.
===============================================================================
*/

#pragma once
#include <string>
#include "ECSEntityManager.h"

namespace PrefabSerializer
{
    // Writes a single entity (and the components it has) to a JSON file.
    // Returns true on success.
    bool SavePrefab(Framework::EntityManager& em,
        Framework::Entity entity,
        const std::string& outPath);
    
    Framework::Entity LoadPrefab(Framework::EntityManager& em, const std::string& path);
}

