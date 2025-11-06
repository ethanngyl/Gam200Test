/**
===============================================================================
 File:           EntitySpawner.h
 Description:    System for spawning entities dynamically during gameplay

 Purpose:
 Instead of hardcoding entities in main.cpp, this system allows you to:
 - Spawn entities from anywhere in your code
 - Create enemies, projectiles, pickups at runtime
 - Respond to game events (player input, collisions, timers)
 - No ImGui needed, no hardcoded main.cpp
===============================================================================
 */

#pragma once
#include "Precompiled.h"

namespace Framework {

    /**
     * @brief Entity Spawner System - Creates entities dynamically
     *
     * Use this system to spawn entities during gameplay instead of
     * hardcoding them in main.cpp
     */
    class EntitySpawner : public EngineSystem {
    public:
        EntitySpawner() = default;
        ~EntitySpawner() = default;

        void Initialize() override {
            LOG_INFO("CORE", "EntitySpawner: Initialized");
        }

        void Update(float dt) override {
            // Could spawn entities based on timers, waves, etc.
            // For now, this is just a utility system
            (void)dt;
        }

        void SendEngineMessage(Message* msg) override {
            (void)msg;
        }

        // Set entity manager
        void SetEntityManager(EntityManager* em) { entityManager = em; }

        // ========================================================================
        // SPAWNING METHODS - Call these from anywhere!
        // ========================================================================

        /**
         * @brief Spawn a basic sprite entity
         */
        Entity SpawnSprite(
            const std::string& spriteName,
            const Vector2D& position,
            const Vector2D& scale = Vector2D(1.0f, 1.0f))
        {
            if (!entityManager) {
                LOG_ERROR("CORE", "EntitySpawner: EntityManager not set!");
                return Entity(0);
            }

            Entity entity = entityManager->CreateEntity();

            // Add transform
            entityManager->AddComponent<Transform>(entity, position);
            auto& transform = entityManager->GetComponent<Transform>(entity);
            transform.scale = scale;

            // Add sprite
            entityManager->AddComponent<Sprite>(entity);
            entityManager->GetComponent<Sprite>(entity).texturePath = spriteName;

            LOG_INFO("CORE", "Spawned sprite: " + spriteName);
            return entity;
        }

        /**
         * @brief Spawn a player entity
         */
        Entity SpawnPlayer(const Vector2D& position) {
            Entity player = SpawnSprite("circle", position, Vector2D(0.3f, 0.3f));

            // Add movement
            entityManager->AddComponent<Movement>(player);
            auto& movement = entityManager->GetComponent<Movement>(player);
            movement.moveSpeed = 0.2f;

            // Add collider
            entityManager->AddComponent<CircleCollider>(player);
            auto& collider = entityManager->GetComponent<CircleCollider>(player);
            collider.radius = 0.15f;

            LOG_INFO("CORE", "Spawned player");
            return player;
        }

        /**
         * @brief Spawn an enemy entity
         */
        Entity SpawnEnemy(const Vector2D& position, float moveSpeed = 0.05f) {
            Entity enemy = SpawnSprite("triangle", position, Vector2D(0.4f, 0.4f));

            // Add movement
            entityManager->AddComponent<Movement>(enemy);
            auto& movement = entityManager->GetComponent<Movement>(enemy);
            movement.moveSpeed = moveSpeed;
            movement.direction = Vector2D(0.0f, -1.0f); // Move down

            // Add collider
            entityManager->AddComponent<TriangleCollider>(enemy);
            auto& triCol = entityManager->GetComponent<TriangleCollider>(enemy);
            float halfW = 0.04f;
            float halfH = 0.04f;
            triCol.v0 = Vector2D(0.0f, halfH);
            triCol.v1 = Vector2D(-halfW, -halfH);
            triCol.v2 = Vector2D(halfW, -halfH);

            LOG_INFO("CORE", "Spawned enemy");
            return enemy;
        }

        /**
         * @brief Spawn a projectile/bullet
         */
        Entity SpawnProjectile(
            const Vector2D& position,
            const Vector2D& direction,
            float speed = 0.3f)
        {
            Entity projectile = SpawnSprite("circle", position, Vector2D(0.1f, 0.1f));

            // Add movement
            entityManager->AddComponent<Movement>(projectile);
            auto& movement = entityManager->GetComponent<Movement>(projectile);
            movement.moveSpeed = speed;
            movement.direction = direction;

            // Add small collider
            entityManager->AddComponent<CircleCollider>(projectile);
            auto& collider = entityManager->GetComponent<CircleCollider>(projectile);
            collider.radius = 0.05f;

            LOG_INFO("CORE", "Spawned projectile");
            return projectile;
        }

        /**
         * @brief Spawn an obstacle/wall
         */
        Entity SpawnObstacle(
            const Vector2D& position,
            const Vector2D& size = Vector2D(0.5f, 0.5f))
        {
            Entity obstacle = SpawnSprite("quad", position, size);

            // Add collider
            entityManager->AddComponent<BoxCollider>(obstacle);
            auto& collider = entityManager->GetComponent<BoxCollider>(obstacle);
            collider.size = size;

            LOG_INFO("CORE", "Spawned obstacle");
            return obstacle;
        }

        /**
         * @brief Spawn a wave of enemies
         */
        void SpawnEnemyWave(int count, float yPosition = 0.8f) {
            for (int i = 0; i < count; ++i) {
                float x = -1.0f + (2.0f / (count - 1)) * i;
                SpawnEnemy(Vector2D(x, yPosition));
            }
            LOG_INFO("CORE", "Spawned enemy wave: " + std::to_string(count));
        }

        /**
         * @brief Spawn entities in a grid pattern
         */
        void SpawnGrid(
            const std::string& spriteName,
            int rows, int cols,
            const Vector2D& startPos,
            const Vector2D& spacing)
        {
            for (int row = 0; row < rows; ++row) {
                for (int col = 0; col < cols; ++col) {
                    Vector2D pos(
                        startPos.x + col * spacing.x,
                        startPos.y + row * spacing.y
                    );
                    SpawnSprite(spriteName, pos, Vector2D(0.1f, 0.1f));
                }
            }
            LOG_INFO("CORE", "Spawned grid: " + std::to_string(rows * cols) + " entities");
        }

        /**
         * @brief Spawn entities in a circle pattern
         */
        void SpawnCircle(
            const std::string& spriteName,
            int count,
            const Vector2D& center,
            float radius)
        {
            for (int i = 0; i < count; ++i) {
                float angle = (2.0f * 3.14159f * i) / count;
                Vector2D pos(
                    center.x + radius * cosf(angle),
                    center.y + radius * sinf(angle)
                );
                SpawnSprite(spriteName, pos, Vector2D(0.1f, 0.1f));
            }
            LOG_INFO("CORE", "Spawned circle: " + std::to_string(count) + " entities");
        }

        // ========================================================================
        // UTILITY METHODS
        // ========================================================================

        /**
         * @brief Destroy an entity
         */
        void DestroyEntity(Entity entity) {
            if (entityManager) {
                entityManager->DestroyEntity(entity);
            }
        }

    private:
        EntityManager* entityManager = nullptr;
    };

} // namespace Framework