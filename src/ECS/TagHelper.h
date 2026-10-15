/*
===============================================================================
 File:          TagHelper.h
 Author:        ETHAN NG
 Email:         n.ethanyongle@digipen.edu
 Date:          2026-04-05
 Contribution:  100%
 ------------------------------------------------------------------------------
 TagHelper Header

 Overview:
    The TagHelper module provides runtime functionality for the game engine.

===============================================================================
*/
#pragma once
#include "Component.h"
#include "ECSEntityManager.h"

namespace Framework {

    /// Find the first entity with the given tag, or INVALID_ENTITY if none.
    /**
        * @brief Finds the first entity that owns the requested tag.
        * @param em Entity manager used for entity/component queries.
        * @param tag Tag string to search for.
        * @return First matching entity, or INVALID_ENTITY when not found.
     */
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
    /**
        * @brief Collects all entities that own the requested tag.
        * @param em Entity manager used for entity/component queries.
        * @param tag Tag string to search for.
        * @return List of matching entities.
     */
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
