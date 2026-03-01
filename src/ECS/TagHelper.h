#pragma once
#include "Component.h"
#include "ECSEntityManager.h"

namespace Framework {

    /// Find the first entity with the given tag, or INVALID_ENTITY if none.
    inline Entity FindFirstByTag(EntityManager* em, const char* tag) {
        for (Entity e : em->GetAllEntities()) {
            if (em->HasComponent<TagComponent>(e) &&
                em->GetComponent<TagComponent>(e).tag == tag) {
                return e;
            }
        }
        return Entity{ INVALID_ENTITY };
    }

    /// Collect all entities that carry the given tag.
    inline std::vector<Entity> FindAllByTag(EntityManager* em, const char* tag) {
        std::vector<Entity> result;
        for (Entity e : em->GetAllEntities()) {
            if (em->HasComponent<TagComponent>(e) &&
                em->GetComponent<TagComponent>(e).tag == tag) {
                result.push_back(e);
            }
        }
        return result;
    }

} // namespace Framework
