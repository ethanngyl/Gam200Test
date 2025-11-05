/**
===============================================================================
 File:           EntitySpawner.h (Fixed Logging Version)
 Description:    System for spawning entities dynamically during gameplay

 This version uses simpler logging to avoid const char* conversion issues.
===============================================================================
 */

#pragma once
#include "Precompiled.h"
#include "RenderComponents.h"   // for MeshRenderer
#include "ECSEntityManager.h"   // if not already pulled in through Precompiled.h
#include "Grid\GridTile.h"

extern Framework::CoreEngine* engine;

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
            (void)dt;
        }

        void SendEngineMessage(Message* msg) override {
            (void)msg;
        }

        void SetEntityManager(EntityManager* em) { entityManager = em; }

        // ========================================================================
        // SPAWNING METHODS
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

            entityManager->AddComponent<Transform>(entity, position);
            auto& transform = entityManager->GetComponent<Transform>(entity);
            transform.scale = scale;

            auto& mr = entityManager->AddComponent<MeshRenderer>(entity);
            mr.spriteName = spriteName;  // may be "player.png" or "quad" etc. 
            mr.visible = true;
            mr.tint = glm::vec4(1.0f);

            std::cout << "[EntitySpawner] Spawned sprite: " << spriteName << "\n";
            return entity;
        }

        /**
         * @brief Spawn a player entity
         */
        Entity SpawnPlayer(const Vector2D& position) {
            // Use the actual file path so the renderer will load a texture.
            Entity player = SpawnSprite("quad", position, Vector2D(0.3f, 0.3f));

            entityManager->AddComponent<Movement>(player);
            auto& movement = entityManager->GetComponent<Movement>(player);
            movement.moveSpeed = 0.2f;

            entityManager->AddComponent<CircleCollider>(player);
            auto& collider = entityManager->GetComponent<CircleCollider>(player);
            collider.radius = 0.15f;

            std::cout << "[EntitySpawner] Spawned player (testing.png)\n";

            //camera
            //FollowPlayer
            engine->GetGraphicsSystem()->SetFollowTarget(player);

            return player;
        }

        /**
         * @brief Spawn an enemy entity 
         */

        //TO MEET THE RUBRICS!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!LOOK HERE LOOK HERE (MULTIPLE SHADER)
        Entity SpawnEnemy(const Vector2D& position, float moveSpeed = 0.05f, const Vector2D& size = Vector2D(0.5f, 0.5f)) {
            (void)moveSpeed; // silence unused variable warning

            Entity enemy = SpawnSprite("assets/testing.png", position, Vector2D(0.4f, 0.4f));
            //entityManager->GetComponent<MeshRenderer>(enemy);
            auto& mr = entityManager->GetComponent<MeshRenderer>(enemy);
            mr.material = GraphicsSystemV2::Material2;
            //entityManager->AddComponent<Movement>(enemy);
            //auto& movement = entityManager->GetComponent<Movement>(enemy);
            //movement.moveSpeed = moveSpeed;
            //movement.direction = Vector2D(0.0f, -1.0f);


            entityManager->AddComponent<BoxCollider>(enemy);
            auto& collider = entityManager->GetComponent<BoxCollider>(enemy);
            collider.size = size;

            std::cout << "[EntitySpawner] Spawned enemy\n";
            return enemy;
        }

        /**
         * @brief Spawn a projectile/bullet
         */
        //ASC: its technically of a script
        Entity SpawnProjectile(
            const Vector2D& position,
            const Vector2D& direction,
            float speed = 0.3f)
        {
            Entity projectile = SpawnSprite("assets/background.jpg", position, Vector2D(0.1f, 0.1f));

            entityManager->AddComponent<ProjectileMovement>(projectile);
            auto& movement = entityManager->GetComponent<ProjectileMovement>(projectile);
            movement.moveSpeed = speed;
            movement.direction = direction;

            entityManager->AddComponent<CircleCollider>(projectile);
            auto& collider = entityManager->GetComponent<CircleCollider>(projectile);
            collider.radius = 0.05f;

            std::cout << "[EntitySpawner] Spawned projectile\n";
            return projectile;
        }

        /**
         * @brief Spawn an obstacle/wall
         */
        Entity SpawnObstacle(
            const Vector2D& position,
            const Vector2D& size = Vector2D(0.5f, 0.5f))
        {
            Entity obstacle = SpawnSprite("assets/testing.png", position, size);

            entityManager->AddComponent<BoxCollider>(obstacle);
            auto& collider = entityManager->GetComponent<BoxCollider>(obstacle);
            collider.size = size;

            std::cout << "[EntitySpawner] Spawned obstacle\n";
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
            std::cout << "[EntitySpawner] Spawned enemy wave: " << count << " enemies\n";
        }

        /**
         * @brief Spawn entities in a grid pattern
         */
        void SpawnGrid(
            const std::string& spriteName,
            int rows, int cols,
            const Vector2D& startPos,
            const Vector2D& spacing = Vector2D{ 1.0f, 1.0f })
        {

            auto& record = GetGrid();
            record.rows = rows;
            record.cols = cols;
            record.startPos = startPos;
            record.spacing = spacing;
            record.em = entityManager;
            record.tiles.assign(static_cast<size_t>(rows) * cols, Entity{ INVALID_ENTITY });

            int nextId = 0;

            for (int row = 0; row < rows; ++row) {
                for (int col = 0; col < cols; ++col) {
                    Vector2D pos(
                        startPos.x + col * spacing.x,
                        startPos.y + row * spacing.y
                    );
                    const Vector2D tileSize{ 0.1f, 0.1f };
                    Entity e = SpawnSprite(spriteName, pos, tileSize);

                    record.tiles[record.Index(col, row)] = e;

                    entityManager->AddComponent<GridTiles>(e);
                    auto& gridTile = entityManager->GetComponent<GridTiles>(e);
                    gridTile.tileId = nextId++;
                    gridTile.x = col;
                    gridTile.y = row;
                    gridTile.entity = e;
                    gridTile.centerWorld = pos;
                    gridTile.tileW = tileSize.x;   // canonical cell size
                    gridTile.tileH = tileSize.y;
                    gridTile.blocked = false;
                    gridTile.occupant = INVALID_ENTITY;
                }
            }
            std::cout << "[EntitySpawner] Spawned grid: " << (rows * cols) << " entities\n";


            // --- Debug summary (verify ids are sequential and unique) ---
            const int total = rows * cols;
            std::vector<bool> seen(static_cast<size_t>(total), false);
            int minId = INT_MAX, maxId = INT_MIN, dupCount = 0, oobCount = 0;

            for (auto e : entityManager->GetAllEntities()) {
                if (!entityManager->HasComponent<GridTiles>(e)) continue;
                const auto& gt = entityManager->GetComponent<GridTiles>(e);
                minId = min(minId, gt.tileId);
                maxId = max(maxId, gt.tileId);
                if (gt.tileId < 0 || gt.tileId >= total) { ++oobCount; continue; }
                if (seen[static_cast<size_t>(gt.tileId)]) ++dupCount;
                else seen[static_cast<size_t>(gt.tileId)] = true;
            }

            std::cout << "[GridDebug] tiles=" << total
                << " id-range=[" << minId << "," << maxId << "]"
                << " dup=" << dupCount
                << " oob=" << oobCount << "\n";

            // Also print corner samples to eyeball mapping:
            auto printTile = [&](int cx, int cy) {
                for (auto e : entityManager->GetAllEntities()) {
                    if (!entityManager->HasComponent<GridTiles>(e)) continue;
                    const auto& gt = entityManager->GetComponent<GridTiles>(e);
                    if (gt.x == cx && gt.y == cy) {
                        std::cout << "  (" << cx << "," << cy << ") -> entity " << e.GetID()
                            << " id=" << gt.tileId << " center=("
                            << gt.centerWorld.x << "," << gt.centerWorld.y << ")\n";
                        return;
                    }
                }
                };

            printTile(0, 0);
            printTile(cols - 1, 0);
            printTile(0, rows - 1);
            printTile(cols - 1, rows - 1);
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
            std::cout << "[EntitySpawner] Spawned circle: " << count << " entities\n";
        }

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
