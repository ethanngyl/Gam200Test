/**
===============================================================================
 File:        Quadtree.cpp
 Author:      Jiahao Zhou
 Email:       jiahao.zhou@digipen.edu
 Date:        2026-01-27
===============================================================================
 */

#include "Collision/Quadtree.h"

namespace Framework
{
    Quadtree::Quadtree(const AABB& worldBounds, int maxDepthIn, int maxItemsPerNodeIn)
    {
        maxDepth = maxDepthIn;
        maxItemsPerNode = maxItemsPerNodeIn;

        root = std::make_unique<Node>();
        root->bounds = worldBounds;
        root->depth = 0;
    }

    void Quadtree::Clear()
    {
        root->items.clear();
        for (auto& c : root->children)
        {
            c.reset();
        }
    }

    void Quadtree::Insert(Entity entity, const AABB& aabb)
    {
        Insert(root.get(), entity, aabb);
    }

    void Quadtree::Query(const AABB& region, std::vector<Entity>& out) const
    {
        Query(root.get(), region, out);
    }

    const AABB& Quadtree::GetWorldBounds() const
    {
        return root->bounds;
    }

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
