#pragma once
/**
===============================================================================
 File:        Quadtree.h
 Author:      Jiahao Zhou
 Email:       jiahao.zhou@digipen.edu
 Date:        2026-01-27
-------------------------------------------------------------------------------
 Copyright (C) 2026 DigiPen Institute of Technology.
 Reproduction or disclosure of this file or its contents
 without the prior written consent of DigiPen Institute of
 Technology is prohibited.
-------------------------------------------------------------------------------
 Brief:
 Simple Quadtree broadphase for 2D AABB spatial partitioning.

 Responsibilities:
 - Insert (Entity, AABB) pairs.
 - Query region AABB to return candidate entities.
 - Optional debug draw by exposing node bounds.
===============================================================================
 */

#pragma once

#include "ECS/ECSEntity.h"
#include "Collision/BroadphaseAABB.h"
#include <array>
#include <memory>
#include <vector>

namespace Framework
{

    /**
     * @brief Simple Quadtree broadphase for 2D AABB spatial partitioning.
     *
     * Stores (Entity, AABB) items in a spatial hierarchy to reduce the number of
     * narrowphase collision checks. Broadphase is conservative: queries may return
     * false positives, but should not miss true overlaps within the world bounds.
     */
    class Quadtree
    {
    public:

        /**
         * @brief Stored payload for the quadtree.
         *
         * Each item couples an ECS Entity with its world-space AABB used for broadphase.
         */
        struct Item
        {
            Entity entity;
            AABB aabb;
        };

        /**
         * @brief Construct a quadtree covering a fixed world-space region.
         *
         * @param worldBounds World-space bounds for the root node.
         * @param maxDepth Maximum subdivision depth (root depth is 0).
         * @param maxItemsPerNode Maximum number of items a node can hold before subdividing.
         */

        Quadtree(const AABB& worldBounds, int maxDepth, int maxItemsPerNode);

        /**
         * @brief Remove all items and collapse the tree back to a single root node.
         *
         * Keeps the same world bounds and configuration values (maxDepth/maxItemsPerNode).
         */

        void Clear();

        /**
         * @brief Insert an entity with its world-space AABB into the quadtree.
         *
         * If the AABB fits fully within exactly one child region, it may be inserted
         * deeper; otherwise it remains stored in the current node. This is conservative
         * for broadphase candidate filtering.
         *
         * @param entity ECS entity to insert.
         * @param aabb World-space AABB for broadphase.
         */

        void Insert(Entity entity, const AABB& aabb);

        /**
         * @brief Query the quadtree for entities whose AABBs may overlap a region.
         *
         * Appends candidate entities into @p out. Results may include false positives;
         * caller should run narrowphase collision tests to confirm actual collisions.
         *
         * @param region World-space AABB region to query.
         * @param out Output list to append candidates into (not cleared by this function).
         */

        void Query(const AABB& region, std::vector<Entity>& out) const;


        /**
         * @brief Get the world-space bounds covered by the quadtree.
         *
         * @return Reference to the root node's AABB bounds.
         */

        const AABB& GetWorldBounds() const;

    private:

        /**
         * @brief Internal quadtree node.
         *
         * A node represents a world-space AABB region. Items that do not fit fully into
         * a single child node are stored in this node. Children are created on demand
         * via subdivision.
         */

        struct Node
        {
            AABB bounds;
            int depth = 0;
            std::vector<Item> items;
            std::array<std::unique_ptr<Node>, 4> children;


            /**
             * @brief Check whether this node has been subdivided.
             *
             * @return True if child nodes have been allocated; false otherwise.
             */

            bool HasChildren() const
            {
                return children[0] != nullptr;
            }
        };

        std::unique_ptr<Node> root;
        int maxDepth = 6;
        int maxItemsPerNode = 6;

        /**
         * @brief Recursive insertion helper.
         *
         * Inserts into a specific node. Subdivides when item capacity is exceeded and
         * depth allows. Items that cannot be fully placed into a single child remain
         * stored in the current node.
         *
         * @param node Current node being inserted into.
         * @param entity ECS entity to insert.
         * @param aabb World-space AABB for broadphase.
         */

        void Insert(Node* node, Entity entity, const AABB& aabb);

        /**
         * @brief Recursive query helper.
         *
         * Traverses nodes that intersect the query region and appends stored entities
         * whose AABBs may overlap the region.
         *
         * @param node Current node being queried.
         * @param region World-space query AABB.
         * @param out Output list to append candidates into.
         */

        void Query(const Node* node, const AABB& region, std::vector<Entity>& out) const;

        /**
         * @brief Subdivide a node into 4 children.
         *
         * Creates child nodes with bounds that partition the parent's bounds. Existing
         * items may remain in the parent unless reinserted by insertion logic.
         *
         * @param node Node to subdivide.
         */

        void Subdivide(Node* node);

        /**
         * @brief Determine which child quadrant can fully contain an AABB.
         *
         * Used during insertion to decide whether the item can be placed in exactly one
         * child node. If the AABB crosses the split lines, it must remain in the parent.
         *
         * @param nodeBounds Bounds of the node being considered (parent bounds).
         * @param aabb Item AABB to place.
         * @return Child index in [0,3] if fully contained by one child; otherwise -1.
         */

        int GetChildIndex(const AABB& nodeBounds, const AABB& aabb) const;
    };
}
