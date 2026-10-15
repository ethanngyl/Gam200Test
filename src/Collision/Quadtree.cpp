/**
===============================================================================
 File:        Quadtree.cpp
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
 Implements the Quadtree broadphase structure used to reduce collision candidate
 checks by spatially partitioning world-space AABBs.

 Responsibilities:
 - Maintain a hierarchical partition (nodes) over fixed world bounds.
 - Insert (Entity, AABB) items with depth/capacity constraints.
 - Query candidate entities that may overlap a given AABB region.

 Notes:
 This system is a broadphase optimization. Query results are conservative and
 may include false positives; narrowphase collision tests must confirm actual
 collisions. Correctness requires never missing a potential overlap within the
 world bounds.
===============================================================================
 */


#include "Collision/Quadtree.h"

namespace Framework
{

    /**
     * @brief Construct a quadtree with fixed world bounds and limits.
     *
     * Initializes the root node using @p worldBounds and stores configuration
     * constraints for maximum depth and per-node capacity.
     *
     * @param worldBounds World-space bounds for the root node.
     * @param maxDepth Maximum subdivision depth (root depth is 0).
     * @param maxItemsPerNode Maximum number of items per node before subdividing.
     */

    Quadtree::Quadtree(const AABB& worldBounds, int maxDepthIn, int maxItemsPerNodeIn)
    {
        maxDepth = maxDepthIn;
        maxItemsPerNode = maxItemsPerNodeIn;

        root = std::make_unique<Node>();
        root->bounds = worldBounds;
        root->depth = 0;
    }

    /**
     * @brief Clear all stored items and reset the tree to a single root node.
     *
     * Removes all node children and item lists. World bounds and configuration
     * values remain unchanged.
     */

    void Quadtree::Clear()
    {
        root->items.clear();
        for (auto& c : root->children)
        {
            c.reset();
        }
    }

    /**
     * @brief Insert an entity with its AABB into the quadtree.
     *
     * This is the public insertion entry point. If the AABB lies outside the
     * world bounds, insertion may be ignored depending on your implementation.
     * Uses recursive insertion to place the item as deep as possible while
     * remaining conservative.
     *
     * @param entity ECS entity to insert.
     * @param aabb World-space AABB used for broadphase.
     */

    void Quadtree::Insert(Entity entity, const AABB& aabb)
    {
        Insert(root.get(), entity, aabb);
    }

    /**
     * @brief Query candidate entities whose AABBs may overlap a region.
     *
     * This is the public query entry point. Appends candidate entities into @p out.
     * Results are conservative and may include false positives.
     *
     * @param region World-space query AABB.
     * @param out Output list to append candidates into (not cleared).
     */

    void Quadtree::Query(const AABB& region, std::vector<Entity>& out) const
    {
        Query(root.get(), region, out);
    }


    /**
     * @brief Get the root world bounds covered by the quadtree.
     *
     * @return Reference to the root node's bounds.
     */
    const AABB& Quadtree::GetWorldBounds() const
    {
        return root->bounds;
    }

    /**
     * @brief Recursive insertion into a specific node.
     *
     * If the node has children and the AABB fits fully within exactly one child,
     * the item is inserted into that child. Otherwise the item remains stored in
     * the current node. When capacity is exceeded and depth allows, the node is
     * subdivided.
     *
     * @param node Node to insert into.
     * @param entity ECS entity to insert.
     * @param aabb World-space AABB used for broadphase.
     */


    void Quadtree::Insert(Node* node, Entity entity, const AABB& aabb)
    {
        // If it doesn't intersect this node at all, ignore.
        if (!AABBIntersects(node->bounds, aabb))
        {
            return;
        }

        // If node has children, try to put into a child that fully contains it.
        if (node->HasChildren())
        {
            int idx = GetChildIndex(node->bounds, aabb);
            if (idx != -1)
            {
                Insert(node->children[idx].get(), entity, aabb);
                return;
            }
        }

        // Otherwise store here.
        node->items.push_back(Item{ entity, aabb });

        // Split if too crowded and allowed.
        if (!node->HasChildren() &&
            node->depth < maxDepth &&
            static_cast<int>(node->items.size()) > maxItemsPerNode)
        {
            Subdivide(node);

            // Re-insert items into children where possible.
            std::vector<Item> remaining;
            remaining.reserve(node->items.size());

            for (const Item& it : node->items)
            {
                int idx = GetChildIndex(node->bounds, it.aabb);
                if (idx != -1)
                {
                    Insert(node->children[idx].get(), it.entity, it.aabb);
                }
                else
                {
                    remaining.push_back(it);
                }
            }

            node->items.swap(remaining);
        }
    }

    /**
     * @brief Recursive query helper.
     *
     * Traverses nodes whose bounds intersect @p region. For each visited node,
     * appends entities stored in that node whose AABBs may overlap the region.
     *
     * @param node Current node being queried.
     * @param region World-space query AABB.
     * @param out Output list to append candidates into.
     */

    void Quadtree::Query(const Node* node, const AABB& region, std::vector<Entity>& out) const
    {
        if (!AABBIntersects(node->bounds, region))
        {
            return;
        }

        for (const Item& it : node->items)
        {
            if (AABBIntersects(it.aabb, region))
            {
                out.push_back(it.entity);
            }
        }

        if (!node->HasChildren())
        {
            return;
        }

        for (const auto& child : node->children)
        {
            if (child)
            {
                Query(child.get(), region, out);
            }
        }
    }

    /**
     * @brief Subdivide a node into four child nodes.
     *
     * Partitions the node's bounds into quadrants and allocates children. The
     * insertion logic determines whether existing items should remain in the
     * parent or be pushed down.
     *
     * @param node Node to subdivide.
     */

    void Quadtree::Subdivide(Node* node)
    {
        const Vector2D min = node->bounds.min;
        const Vector2D max = node->bounds.max;
        const Vector2D mid((min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f);

        // Children order:
        // 0: bottom-left, 1: bottom-right, 2: top-left, 3: top-right
        AABB bl{ Vector2D(min.x, min.y), Vector2D(mid.x, mid.y) };
        AABB br{ Vector2D(mid.x, min.y), Vector2D(max.x, mid.y) };
        AABB tl{ Vector2D(min.x, mid.y), Vector2D(mid.x, max.y) };
        AABB tr{ Vector2D(mid.x, mid.y), Vector2D(max.x, max.y) };

        node->children[0] = std::make_unique<Node>();
        node->children[1] = std::make_unique<Node>();
        node->children[2] = std::make_unique<Node>();
        node->children[3] = std::make_unique<Node>();

        node->children[0]->bounds = bl;
        node->children[1]->bounds = br;
        node->children[2]->bounds = tl;
        node->children[3]->bounds = tr;

        node->children[0]->depth = node->depth + 1;
        node->children[1]->depth = node->depth + 1;
        node->children[2]->depth = node->depth + 1;
        node->children[3]->depth = node->depth + 1;
    }

    /**
     * @brief Compute which quadrant can fully contain an AABB.
     *
     * Used to decide whether an item can be inserted into exactly one child node.
     * If the AABB crosses split lines (does not fit fully in a single quadrant),
     * returns -1 so it stays in the current node.
     *
     * @param nodeBounds Bounds of the parent node.
     * @param aabb Item AABB to place.
     * @return Child index in [0,3] if fully contained by one child; otherwise -1.
     */

    int Quadtree::GetChildIndex(const AABB& nodeBounds, const AABB& aabb) const
    {
        const Vector2D min = nodeBounds.min;
        const Vector2D max = nodeBounds.max;
        const Vector2D mid((min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f);

        // Determine which quadrant fully contains the AABB.
        const bool inLeft = aabb.max.x <= mid.x;
        const bool inRight = aabb.min.x >= mid.x;
        const bool inBottom = aabb.max.y <= mid.y;
        const bool inTop = aabb.min.y >= mid.y;

        if (inLeft && inBottom) return 0;  // bottom-left
        if (inRight && inBottom) return 1; // bottom-right
        if (inLeft && inTop) return 2;     // top-left
        if (inRight && inTop) return 3;    // top-right

        return -1; // doesn't fit wholly in one child
    }
}
