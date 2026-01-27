#pragma once
/**
===============================================================================
 File:        Quadtree.h
 Author:      Jiahao Zhou
 Email:       jiahao.zhou@digipen.edu
 Date:        2026-01-27
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
    class Quadtree
    {
    public:
        struct Item
        {
            Entity entity;
            AABB aabb;
        };

        Quadtree(const AABB& worldBounds, int maxDepth, int maxItemsPerNode);

        void Clear();
        void Insert(Entity entity, const AABB& aabb);
        void Query(const AABB& region, std::vector<Entity>& out) const;

        const AABB& GetWorldBounds() const;

    private:
        struct Node
        {
            AABB bounds;
            int depth = 0;
            std::vector<Item> items;
            std::array<std::unique_ptr<Node>, 4> children;

            bool HasChildren() const
            {
                return children[0] != nullptr;
            }
        };

        std::unique_ptr<Node> root;
        int maxDepth = 6;
        int maxItemsPerNode = 6;

        void Insert(Node* node, Entity entity, const AABB& aabb);
        void Query(const Node* node, const AABB& region, std::vector<Entity>& out) const;

        void Subdivide(Node* node);
        int GetChildIndex(const AABB& nodeBounds, const AABB& aabb) const;
    };
}
