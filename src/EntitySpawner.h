/*
===============================================================================
 File:          EntitySpawner.h
 Author:        ETHAN NG
 Email:         n.ethanyongle@digipen.edu
 Date:          2025-10-31
 Contribution:  100%
 ------------------------------------------------------------------------------
  Entity Spawner System

  Design notes:
     - Provides centralized entity creation during gameplay
     - Eliminates hardcoded entity creation in main.cpp
     - Integrates with ECS (Entity-Component-System) architecture
     - Supports multiple spawn patterns (grid, circle, waves)
     - Handles component initialization for common entity types
===============================================================================
*/

#pragma once
#include "Precompiled.h"
#include "RenderComponents.h"   // for MeshRenderer
#include "ECSEntityManager.h"   // if not already pulled in through Precompiled.h
#include "Grid\GridTile.h"
#include "Graphics/RenderLayers.h"  // for standard layer constants

extern Framework::CoreEngine* engine;

namespace Framework {

    /**
     * @class EntitySpawner
     * @brief System for dynamically spawning entities during gameplay
     *
     * The EntitySpawner provides a high-level API for creating game entities
     * without manually adding components. It handles:
     * - Common entity archetypes (player, enemy, projectile, obstacle)
     * - Automatic component initialization with sensible defaults
     * - Pattern-based spawning (grids, circles, waves)
     * - Material and texture assignment
     * - Collision component setup
     *
     * Usage Pattern:
     * @code
     *   EntitySpawner spawner;
     *   spawner.SetEntityManager(&entityManager);
     *   spawner.Initialize();
     *
     *   // Spawn player
     *   Entity player = spawner.SpawnPlayer(Vector2D(0.0f, -0.5f));
     *
     *   // Spawn enemies
     *   spawner.SpawnEnemyWave(5, 0.8f);
     *
     *   // Spawn grid
     *   spawner.SpawnGrid("quad", 10, 10, Vector2D(-5.0f, -5.0f));
     * @endcode
     */
    class EntitySpawner : public EngineSystem {
    public:
        /**
         * @brief Default constructor
         *
         * Creates the EntitySpawner instance without initializing systems.
         * Call Initialize() after construction.
         */
        EntitySpawner() = default;

        /**
         * @brief Default destructor
         *
         * Cleans up the EntitySpawner. Does not destroy spawned entities
         * (entities are managed by EntityManager).
         */
        ~EntitySpawner() = default;

        /**
         * @brief Initializes the entity spawner system
         * @override EngineSystem::Initialize
         *
         * Performs initialization logging and prepares the spawner for use.
         * Must be called before spawning any entities.
         *
         * @note Requires EntityManager to be set via SetEntityManager() first
         */
        void Initialize() override {
            LOG_INFO("CORE", "EntitySpawner: Initialized");
        }

        /**
         * @brief Updates the entity spawner system each frame
         * @override EngineSystem::Update
         * @param dt Delta time since last frame (currently unused)
         *
         * The EntitySpawner is a passive system and does not require
         * per-frame updates. Spawning is triggered explicitly through
         * spawn method calls rather than automatic updates.
         *
         * @note This method is inherited from EngineSystem but not actively used
         */
        void Update(float dt) override {
            (void)dt;
        }

        /**
         * @brief Message handling interface (currently unused)
         * @override EngineSystem::SendEngineMessage
         * @param msg Message pointer to process
         *
         * Inherited from EngineSystem base class for future message-based
         * communication between engine systems. Currently not implemented.
         */
        void SendEngineMessage(Message* msg) override {
            (void)msg;
        }

        /**
         * @brief Sets the entity manager for ECS integration
         * @param em Pointer to the EntityManager instance
         *
         * The EntityManager is required for the spawner to:
         * - Create new entities
         * - Add components to entities
         * - Access and modify component data
         * - Integrate with the ECS architecture
         *
         * @note Must be called before any spawning operations
         * @note Does not take ownership (non-owning pointer)
         */
        void SetEntityManager(EntityManager* em) { entityManager = em; }

        // ========================================================================
        // SPAWNING METHODS
        // ========================================================================

        /**
         * @brief Spawn a basic sprite entity with transform and renderer
         * @param spriteName Name/path of the sprite texture to use
         * @param position World position for the entity
         * @param scale Scale of the sprite (default: 1.0, 1.0)
         * @return Entity handle to the newly created sprite entity
         *
         * Spawning Process:
         * 1. Creates a new entity via EntityManager
         * 2. Adds Transform component with position and scale
         * 3. Adds MeshRenderer component with sprite configuration
         * 4. Assigns mesh and material through GraphicsSystemV2
         * 5. Creates unique material instance to prevent shared state
         *
         * Components Added:
         * - Transform: Position, rotation (0), scale
         * - MeshRenderer: Sprite texture, visibility (true), tint (white)
         *
         * Material Handling:
         * - Each sprite gets its own material instance
         * - Prevents unintended state sharing between entities
         * - Material name format: "{spriteName}_inst_{entityID}"
         * - Shallow copies base material properties
         *
         * @note Sprite must be a valid resource name in GraphicsSystem
         * @note Returns Entity(0) if EntityManager not set
         *
         * Example:
         * @code
         *   Entity sprite = spawner.SpawnSprite("assets/coin.png",
         *                                        Vector2D(0.0f, 0.5f),
         *                                        Vector2D(0.2f, 0.2f));
         * @endcode
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
            mr.spriteName = spriteName;
            mr.visible = true;
            mr.tint = glm::vec4(1.0f);

            // ============================================================================
            // Author:        Tan Wei Leong
            // Email:         weileong.tan@digipen.edu
            // Date:          2025-11-06
            // Contribution:  100% (Dynamic material instancing and mesh assignment)
            // -----------------------------------------------------------------------------
            // Description:
            //   This section ensures that each spawned entity has its own unique
            //   material instance and properly assigned mesh. It integrates with the
            //   GraphicsSystemV2 to dynamically bind the correct mesh and base material
            //   based on the sprite name provided.
            //
            //   Implementation Details:
            //   • Uses GraphicsSystemV2::AssignMeshAndMaterial() to assign the correct
            //     mesh-material pair to the entity's MeshRenderer component.
            //   • Creates a shallow clone of the base material for each entity, ensuring
            //     that material properties (tint, texture, shader uniforms) are not shared
            //     between entities.
            //   • This prevents side effects where changing one material (e.g. tint or UVs)
            //     unintentionally affects all other entities using the same base material.
            //
            //   Key Features:
            //     - One material instance per entity (prevents global state sharing)
            //     - Material name pattern: "{spriteName}_inst_{entityID}"
            //     - Fully integrated with ECS and ResourceManager
            //     - Supports dynamic spawning and per-entity customization
            //
            //   Used by:
            //     - EntitySpawner::SpawnSprite()
            //     - EntitySpawner::SpawnPlayer()
            //     - EntitySpawner::SpawnEnemy()
            // ============================================================================
            // ensure each entity has its own material instance
            if (CORE && CORE->GetGraphicsSystem())
            {
                auto* gs = static_cast<GraphicsSystemV2*>(CORE->GetGraphicsSystem());
                gs->AssignMeshAndMaterial(mr, spriteName);

                // create a unique copy of the base material
                if (mr.material.IsValid())
                {
                    auto* base = gs->GetResourceManager().GetMaterial(mr.material);
                    MaterialHandle clone = gs->GetResourceManager().CreateMaterial(
                        spriteName + "_inst_" + std::to_string(entity.GetID()), base->shader);

                    *gs->GetResourceManager().GetMaterial(clone) = *base;  // shallow copy
                    mr.material = clone;
                }
            }

            return entity;
        }

        /**
         * @brief Spawn a player-controlled entity
         * @param position Starting world position for the player
         * @return Entity handle to the newly created player entity
         *
         * Spawning Process:
         * 1. Creates base sprite entity using "quad" mesh
         * 2. Assigns Material2 for shader rendering
         * 3. Adds Movement component for player control
         * 4. Adds CircleCollider for collision detection
         * 5. Sets camera to follow the player entity
         *
         * Components Added:
         * - Transform: Position and scale (0.1, 0.1)
         * - MeshRenderer: Quad mesh with Material2
         * - Movement: Move speed of 0.2 units/second
         * - CircleCollider: Radius of 0.15 units
         *
         * Default Configuration:
         * - Sprite: "quad" (basic quad mesh)
         * - Scale: 0.1 x 0.1 (small player size)
         * - Move Speed: 0.2 units per second
         * - Collider Radius: 0.15 units (slightly larger than visual)
         * - Material: Material2 (custom shader material)
         *
         * Camera Behavior:
         * - Automatically sets player as camera follow target
         * - Camera will track player movement
         * - Ensures player remains centered in view
         *


         * Example:
         * @code
         *   Entity player = spawner.SpawnPlayer(Vector2D(0.0f, -0.5f));
         *   // Player spawned at center-bottom of screen with camera following
         * @endcode
         */
        Entity SpawnPlayer(const Vector2D& position) {
            // Use the actual file path so the renderer will load a texture.
            const std::string spritePath = "quad";
            Entity player = SpawnSprite("assets/player.png", position, Vector2D(0.1f, 0.1f));

            auto& mr = entityManager->GetComponent<MeshRenderer>(player);

            // Create unique material instance for player (don't use shared Material2!)
            // This ensures UV bounds for animations won't affect other entities
            LOG_INFO("SPAWN_PLAYER", "=== Creating Material Instance for Player %u ===", player.GetID());
            if (CORE && CORE->GetGraphicsSystem()) {
                auto* gs = static_cast<GraphicsSystemV2*>(CORE->GetGraphicsSystem());
                auto* baseMat = gs->GetResourceManager().GetMaterial(GraphicsSystemV2::Material2);
                if (baseMat) {
                    LOG_INFO("SPAWN_PLAYER", "  Base Material2: name='%s' shader=%u",
                        baseMat->name.c_str(), baseMat->shader.GetID());

                    MaterialHandle playerMat = gs->GetResourceManager().CreateMaterial(
                        "player_material_" + std::to_string(player.GetID()),
                        baseMat->shader
                    );

                    auto* newMat = gs->GetResourceManager().GetMaterial(playerMat);
                    *newMat = *baseMat;  // Copy properties

                    LOG_INFO("SPAWN_PLAYER", "  Created unique material: name='%s' handle=%u",
                        newMat->name.c_str(), playerMat.GetID());
                    LOG_INFO("SPAWN_PLAYER", "  Initial UV bounds: (%.3f,%.3f,%.3f,%.3f)",
                        newMat->u0, newMat->v0, newMat->u1, newMat->v1);

                    mr.material = playerMat;  // Use unique instance
                } else {
                    LOG_ERROR("SPAWN_PLAYER", "  FAILED: Material2 is NULL!");
                }
            } else {
                LOG_ERROR("SPAWN_PLAYER", "  FAILED: CORE or GraphicsSystem is NULL!");
            }

            mr.layer = RenderLayers::Player;  // Player renders on top

            /*entityManager->AddComponent<Movement>(player);
            auto& movement = entityManager->GetComponent<Movement>(player);
            movement.moveSpeed = 0.2f;*/

            entityManager->AddComponent<CircleCollider>(player);
            auto& collider = entityManager->GetComponent<CircleCollider>(player);
            collider.radius = 0.15f;

			entityManager->AddComponent<Inventory>(player);

			entityManager->AddComponent<Health>(player, 5); // max 5 health points

			entityManager->AddComponent<AttackAP>(player, 3); // max 3 attack points

			entityManager->AddComponent<AP>(player, 5); // max 5 action points

			//entityManager->AddComponent<AttackRangeComponent>(player, 1); // 1 attack range

            // NOTE: SpriteAnimation component is now loaded via Lua (LoadPlayerAnimation)
            // This allows for more flexible animation management per level

            std::cout << "[EntitySpawner] Spawned player (testing.png) on layer " << RenderLayers::Player << "\n";

            //camera
            //FollowPlayer
            engine->GetGraphicsSystem()->SetFollowTarget(player);

            return player;
        }

        /**
         * @brief Spawn an enemy entity with collision
         * @param position Starting world position for the enemy
         * @param moveSpeed Movement speed in units/second (default: 0.05, currently unused)
         * @param size Collision box dimensions (default: 0.5 x 0.5)
         * @return Entity handle to the newly created enemy entity
         *
         * Spawning Process:
         * 1. Creates base sprite entity with testing texture
         * 2. Assigns Material2 for custom shader rendering
         * 3. Adds BoxCollider for rectangular collision detection
         *
         * Components Added:
         * - Transform: Position and scale (0.1, 0.1)
         * - MeshRenderer: Testing texture with Material2
         * - BoxCollider: Rectangular collision bounds
         *
         * Default Configuration:
         * - Sprite: "assets/testing.png"
         * - Visual Scale: 0.1 x 0.1 (small enemy)
         * - Collision Size: 0.5 x 0.5 (configurable via parameter)
         * - Material: Material2 (MULTIPLE SHADER SUPPORT - RUBRIC REQUIREMENT)
         *
         * Collision Behavior:
         * - Uses BoxCollider for axis-aligned collision
         * - Collision size independent of visual scale
         * - Can be configured per enemy type
         *
         *
         * Example:
         * @code
         *   // Standard enemy
         *   Entity enemy1 = spawner.SpawnEnemy(Vector2D(1.0f, 0.5f));
         *
         *   // Fast enemy with smaller hitbox
         *   Entity enemy2 = spawner.SpawnEnemy(Vector2D(-1.0f, 0.5f),
         *                                       0.1f,  // faster
         *                                       Vector2D(0.3f, 0.3f)); // smaller
         * @endcode
         */
        Entity SpawnEnemy(const Vector2D& position, float moveSpeed = 0.05f, const Vector2D& size = Vector2D(0.1f, 0.1f)) {
            (void)moveSpeed; // silence unused variable warning

            Entity enemy = SpawnSprite("assets/testing.png", position, Vector2D(0.1f, 0.1f));
            //entityManager->GetComponent<MeshRenderer>(enemy);
            auto& mr = entityManager->GetComponent<MeshRenderer>(enemy);
            mr.material = GraphicsSystemV2::Material2;
            mr.layer = RenderLayers::Enemies;  // Enemies render below player
            //entityManager->AddComponent<Movement>(enemy);
            //auto& movement = entityManager->GetComponent<Movement>(enemy);
            //movement.moveSpeed = moveSpeed;
            //movement.direction = Vector2D(0.0f, -1.0f);

            // *** NEW: Add Health component (50 HP by default) ***
            entityManager->AddComponent<Health>(enemy, 2);

            entityManager->AddComponent<BoxCollider>(enemy);
            auto& collider = entityManager->GetComponent<BoxCollider>(enemy);
            collider.size = size;

            std::cout << "[EntitySpawner] Spawned enemy on layer " << RenderLayers::Enemies << "\n";
            return enemy;
        }

        /**
         * @brief Spawn a projectile/bullet entity with directional movement
         * @param position Starting world position for the projectile
         * @param direction Normalized direction vector for projectile travel
         * @param speed Movement speed in units/second (default: 0.3)
         * @return Entity handle to the newly created projectile entity
         *
         * Spawning Process:
         * 1. Creates base sprite entity using "circle" mesh
         * 2. Adds ProjectileMovement component for autonomous movement
         * 3. Adds CircleCollider for collision detection
         * 4. Configures direction and speed
         *
         * Components Added:
         * - Transform: Position and scale (0.1, 0.1)
         * - MeshRenderer: Circle mesh rendering
         * - ProjectileMovement: Speed and direction for autonomous motion
         * - CircleCollider: Small circular collision (radius 0.05)
         *
         * Default Configuration:
         * - Visual Scale: 0.1 x 0.1 (small projectile)
         * - Speed: 0.3 units per second
         * - Collider Radius: 0.05 units (small hitbox)
         * - Mesh: "circle" (round projectile appearance)
         *
         * Movement Behavior:
         * - ProjectileMovement component handles autonomous motion
         * - Moves continuously in specified direction
         * - Speed determines travel velocity
         * - Direction should be normalized for consistent speed
         *
         * Collision Detection:
         * - Small circular collider for precise hit detection
         * - Radius smaller than visual size for gameplay feel
         * - Suitable for bullet/projectile interactions
         *
         * Example:
         * @code
         *   // Fire projectile upward
         *   Vector2D up(0.0f, 1.0f);
         *   Entity bullet = spawner.SpawnProjectile(playerPos, up, 0.5f);
         *
         *   // Fire toward mouse position
         *   Vector2D dir = (mousePos - playerPos).Normalize();
         *   Entity shot = spawner.SpawnProjectile(playerPos, dir);
         * @endcode
         */
        Entity SpawnProjectile(
            const Vector2D& position,
            const Vector2D& direction,
            float speed = 0.3f)
        {
            Entity projectile = SpawnSprite("assets/Bullet.png", position, Vector2D(0.1f, 0.1f));

            auto& mr = entityManager->GetComponent<MeshRenderer>(projectile);
            mr.layer = RenderLayers::Projectiles;  // Projectiles above player

            entityManager->AddComponent<ProjectileMovement>(projectile);
            auto& movement = entityManager->GetComponent<ProjectileMovement>(projectile);
            movement.moveSpeed = speed;
            movement.direction = direction;

            entityManager->AddComponent<CircleCollider>(projectile);
            auto& collider = entityManager->GetComponent<CircleCollider>(projectile);
            collider.radius = 0.05f;

            std::cout << "[EntitySpawner] Spawned projectile on layer " << RenderLayers::Projectiles << "\n";
            return projectile;
        }

        /**
         * @brief Spawn a horizontal wave of enemies
         * @param count Number of enemies to spawn in the wave
         * @param yPosition Y-axis position for the entire wave (default: 0.8)
         *
         * Spawning Process:
         * 1. Calculates even spacing across horizontal axis (-1.0 to 1.0)
         * 2. Spawns specified number of enemies at calculated positions
         * 3. All enemies placed at same Y height
         *
         * Positioning Logic:
         * - Enemies distributed evenly across screen width
         * - X positions range from -1.0 (left) to 1.0 (right)
         * - Spacing formula: x = -1.0 + (2.0 / (count - 1)) * i
         * - All enemies at same Y height for formation
         *
         * Default Configuration:
         * - Y Position: 0.8 (near top of screen)
         * - Even horizontal distribution
         * - Uses standard SpawnEnemy() for each entity
         *
         * Typical Use Cases:
         * - Space Invaders-style enemy formations
         * - Wave-based enemy spawning
         * - Boss battle minion waves
         * - Timed enemy deployments
         *
         * Formation Behavior:
         * - Creates uniform horizontal line
         * - Enemies spawn simultaneously
         * - No automatic movement (add via AI systems)
         * - Can be combined with multiple waves at different Y levels
         *
         * Example:
         * @code
         *   // Spawn 5 enemies near top
         *   spawner.SpawnEnemyWave(5, 0.8f);
         *
         *   // Spawn 10 enemies in middle
         *   spawner.SpawnEnemyWave(10, 0.0f);
         *
         *   // Multiple waves
         *   spawner.SpawnEnemyWave(5, 0.9f);  // Top row
         *   spawner.SpawnEnemyWave(7, 0.6f);  // Middle row
         *   spawner.SpawnEnemyWave(9, 0.3f);  // Bottom row
         * @endcode
         */
        void SpawnEnemyWave(int count, float yPosition = 0.8f) {
            for (int i = 0; i < count; ++i) {
                float x = -1.0f + (2.0f / (count - 1)) * i;
                SpawnEnemy(Vector2D(x, yPosition));
            }
            std::cout << "[EntitySpawner] Spawned enemy wave: " << count << " enemies\n";
        }

        /**
         * @brief Spawn entities in a grid pattern with tile metadata
         * @param spriteName Sprite/texture to use for all grid tiles
         * @param rows Number of rows in the grid
         * @param cols Number of columns in the grid
         * @param startPos World position of the grid origin (top-left corner)
         * @param spacing Distance between tile centers (default: 1.0 x 1.0)
         *
         * Spawning Process:
         * 1. Initializes grid record with dimensions and parameters
         * 2. Pre-allocates tile entity storage (rows * cols)
         * 3. Iterates through each grid position (row, col)
         * 4. Spawns sprite at calculated world position
         * 5. Adds GridTiles component with metadata
         * 6. Assigns Material2 for shader rendering
         * 7. Stores entity in grid record for lookup
         * 8. Performs debug validation and logs grid info
         *
         * Components Added (per tile):
         * - Transform: Calculated position and size (0.1 x 0.1)
         * - MeshRenderer: Specified sprite with Material2
         * - GridTiles: Comprehensive tile metadata
         *
         * GridTiles Metadata:
         * - tileId: Sequential unique identifier (0 to rows*cols-1)
         * - x, y: Grid coordinates (column, row)
         * - entity: Entity handle for this tile
         * - centerWorld: World-space position of tile center
         * - tileW, tileH: Tile dimensions (0.1 x 0.1)
         * - blocked: Pathfinding flag (default: false)
         * - occupant: Entity occupying tile (default: INVALID_ENTITY)
         *
         * Position Calculation:
         * - X Position: startPos.x + col * spacing.x
         * - Y Position: startPos.y + row * spacing.y
         * - Forms uniform grid aligned to world axes
         *
         * Grid Record:
         * - Stores all grid configuration and tile entities
         * - Enables grid-based queries (Index() method)
         * - Accessible via GetGrid() for pathfinding/logic
         *
         * Debug Validation:
         * - Verifies tileId uniqueness (no duplicates)
         * - Checks ID range (0 to total-1)
         * - Logs min/max ID, duplicate count, out-of-bounds count
         * - Prints corner tile samples for visual verification
         *
         * Example:
         * @code
         *   // 10x10 grid starting at top-left
         *   spawner.SpawnGrid("quad", 10, 10,
         *                     Vector2D(-5.0f, -5.0f),
         *                     Vector2D(1.0f, 1.0f));
         *
         *   // Dense 20x20 grid with tight spacing
         *   spawner.SpawnGrid("tile", 20, 20,
         *                     Vector2D(-10.0f, -10.0f),
         *                     Vector2D(0.5f, 0.5f));
         * @endcode
         */

        void SpawnGrid(
            const std::string& spriteName,
            int rows, int cols,
            const Vector2D& startPos,
            const Vector2D& spacing = Vector2D{ 1.0f, 1.0f })
        {
            (void)spriteName;
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
                    Entity e = SpawnSprite("assets/grid.png", pos, tileSize);

                    record.tiles[record.Index(col, row)] = e;

                    entityManager->AddComponent<GridTiles>(e);
                    auto& gridTile = entityManager->GetComponent<GridTiles>(e);
                    auto& mr = entityManager->GetComponent<MeshRenderer>(e);
                    mr.material = GraphicsSystemV2::Material2;
                    mr.layer = RenderLayers::Ground;  // Grid tiles on ground layer
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
         * @brief Spawn entities in a circular pattern around a center point
         * @param spriteName Sprite/texture to use for all circle entities
         * @param count Number of entities to spawn around the circle
         * @param center World position of the circle center
         * @param radius Distance from center to each spawned entity
         *
         * Spawning Process:
         * 1. Calculates angular spacing (2π / count radians)
         * 2. For each entity, computes angle (i * angular spacing)
         * 3. Converts polar coordinates to Cartesian (x, y)
         * 4. Spawns sprite at calculated position
         *
         * Position Calculation:
         * - Angle: (2π * i) / count (evenly distributed)
         * - X Position: center.x + radius * cos(angle)
         * - Y Position: center.y + radius * sin(angle)
         * - Forms perfect circle in world space
         *
         * Components Added (per entity):
         * - Transform: Calculated position, scale (0.1, 0.1)
         * - MeshRenderer: Specified sprite with default material
         *
         * Default Configuration:
         * - Entity Scale: 0.1 x 0.1 (small sprites)
         * - Even angular distribution
         * - No rotation alignment (entities face default direction)
         *
         * Circle Properties:
         * - Entities evenly spaced around circumference
         * - First entity at angle 0 (rightmost position)
         * - Subsequent entities progress counter-clockwise
         * - Perfect symmetry for any count value
         *
         * Typical Use Cases:
         * - Radial menu systems
         * - Power-up arrangements
         * - Boss battle attack patterns
         * - Orbital enemy formations
         * - Particle effect rings
         * - Defense tower placements
         *
         * Example:
         * @code
         *   // Spawn 8 coins in circle
         *   spawner.SpawnCircle("coin", 8, Vector2D(0.0f, 0.0f), 2.0f);
         *
         *   // Spawn ring of enemies
         *   spawner.SpawnCircle("enemy", 12, playerPos, 5.0f);
         *
         *   // Small orbital ring
         *   spawner.SpawnCircle("star", 16, bossPos, 1.5f);
         * @endcode
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
         * @brief Destroy an entity and remove it from the game
         * @param entity Entity handle to destroy
         *
         * Destruction Process:
         * 1. Validates EntityManager is set
         * 2. Calls EntityManager's DestroyEntity()
         * 3. Entity handle becomes invalid after destruction
         *
         * Cleanup Behavior:
         * - Removes all components from entity
         * - Frees entity ID for reuse
         * - Stops rendering and updating
         * - Removes from all system queries
         *
         * Example:
         * @code
         *   Entity projectile = spawner.SpawnProjectile(...);
         *   // ... later, on collision
         *   spawner.DestroyEntity(projectile);
         * @endcode
         */
        void DestroyEntity(Entity entity) {
            if (entityManager) {
                entityManager->DestroyEntity(entity);
            }
        }

    private:
        /// Pointer to entity manager for ECS operations (non-owning)
        EntityManager* entityManager = nullptr;
    };

} // namespace Framework