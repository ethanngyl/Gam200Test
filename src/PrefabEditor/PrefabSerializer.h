//kahyan
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

