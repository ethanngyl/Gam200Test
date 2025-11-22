#include "RangeIndicatorSystem.h"
#include "Component.h"
#include "Graphics/RenderLayers.h"
#include <cmath>
#include <iostream>

namespace Framework {

    RangeIndicatorSystem::RangeIndicatorSystem()
        : entityManager(nullptr)
        , graphicsSystem(nullptr)
        , gridCellSize(1.0f)
        , gridOffset(0, 0)
    {
        std::cout << "RangeIndicatorSystem: Constructor\n";
    }

    RangeIndicatorSystem::~RangeIndicatorSystem() {
        ClearIndicators();
    }

    void RangeIndicatorSystem::Initialize() {
        std::cout << "RangeIndicatorSystem: Initialized\n";
    }

    void RangeIndicatorSystem::Update(float dt) {
        (void)dt;

        // DEBUGGING: Disable range indicator entities
        return;

        if (!entityManager) return;

        // Check all entities with AttackRangeComponent
        for (Entity entity : entityManager->GetAllEntities()) {
            if (!entityManager->HasComponent<AttackRangeComponent>(entity)) continue;

            auto& range = entityManager->GetComponent<AttackRangeComponent>(entity);

            // Show/hide range based on showRange flag
            if (range.showRange) {
                if (currentRangeEntity != entity) {
                    ShowRangeForEntity(entity);
                }
            }
            else {
                if (currentRangeEntity == entity) {
                    HideRange();
                }
            }
        }
    }

    void RangeIndicatorSystem::SendEngineMessage(Message* msg) {
        (void)msg;
    }

    void RangeIndicatorSystem::ShowRangeForEntity(Entity entity) {
        if (!entityManager) return;

        // Clear previous indicators
        ClearIndicators();

        if (!entityManager->HasComponent<Transform>(entity)) return;
        if (!entityManager->HasComponent<AttackRangeComponent>(entity)) return;

        auto& transform = entityManager->GetComponent<Transform>(entity);
        auto& range = entityManager->GetComponent<AttackRangeComponent>(entity);

        // Convert entity position to grid
        GridPosition centerGrid = WorldToGrid(transform.position);

        // Get all tiles in range
        std::vector<GridPosition> tiles = GetTilesInRange(
            centerGrid,
            range.minRange,
            range.maxRange
        );

        std::cout << "[RangeIndicator] Showing " << tiles.size()
            << " tiles for entity " << entity.GetID()
            << " (range: " << range.minRange << "-" << range.maxRange << ")\n";

        // Create indicator for each tile
        for (const auto& tile : tiles) {
            CreateTileIndicator(tile);
        }

        currentRangeEntity = entity;
    }

    void RangeIndicatorSystem::HideRange() {
        ClearIndicators();
        currentRangeEntity = Entity();
        std::cout << "[RangeIndicator] Range hidden\n";
    }

    void RangeIndicatorSystem::ToggleRange(Entity entity) {
        if (!entityManager->HasComponent<AttackRangeComponent>(entity)) return;

        auto& range = entityManager->GetComponent<AttackRangeComponent>(entity);
        range.showRange = !range.showRange;
    }

    bool RangeIndicatorSystem::IsPositionInRange(const GridPosition& pos,
        const GridPosition& center,
        int minRange, int maxRange) const {
        // Manhattan distance (grid-based)
        int distance = abs(pos.x - center.x) + abs(pos.y - center.y);
        return distance >= minRange && distance <= maxRange;
    }

    // ========================================================================
    // HELPER FUNCTIONS
    // ========================================================================

    GridPosition RangeIndicatorSystem::WorldToGrid(const Vector2D& worldPos) const {
        return {
            static_cast<int>(std::round((worldPos.x - gridOffset.x) / gridCellSize)),
            static_cast<int>(std::round((worldPos.y - gridOffset.y) / gridCellSize))
        };
    }

    Vector2D RangeIndicatorSystem::GridToWorld(const GridPosition& gridPos) const {
        return Vector2D(
            gridPos.x * gridCellSize + gridOffset.x,
            gridPos.y * gridCellSize + gridOffset.y
        );
    }

    std::vector<GridPosition> RangeIndicatorSystem::GetTilesInRange(
        const GridPosition& center,
        int minRange,
        int maxRange) const {

        std::vector<GridPosition> tiles;

        // Check all tiles in bounding box
        for (int x = center.x - maxRange; x <= center.x + maxRange; x++) {
            for (int y = center.y - maxRange; y <= center.y + maxRange; y++) {
                GridPosition pos = { x, y };

                if (IsPositionInRange(pos, center, minRange, maxRange)) {
                    tiles.push_back(pos);
                }
            }
        }

        return tiles;
    }

    void RangeIndicatorSystem::CreateTileIndicator(const GridPosition& gridPos) {
        if (!entityManager) return;

        Entity indicator = entityManager->CreateEntity();

        // Transform
        entityManager->AddComponent<Transform>(indicator);
        auto& transform = entityManager->GetComponent<Transform>(indicator);
        transform.position = GridToWorld(gridPos);
        transform.scale = Vector2D(gridCellSize * 0.9f, gridCellSize * 0.9f);  // 90% of cell size
        transform.rotation = 0.0f;

        // MeshRenderer
        entityManager->AddComponent<MeshRenderer>(indicator);
        auto& mr = entityManager->GetComponent<MeshRenderer>(indicator);

        // Use existing quad mesh
        if (graphicsSystem) {
            mr.mesh = graphicsSystem->GetResourceManager().GetMeshHandle("quad");
        }

        // Semi-transparent red color
        mr.tint = glm::vec4(1.0f, 0.0f, 0.0f, 0.4f);  // Red with 40% opacity
        mr.layer = RenderLayers::RangeIndicators;  // Above ground but below characters
        mr.orderInLayer = 0;

        indicatorEntities.push_back(indicator);
    }

    void RangeIndicatorSystem::ClearIndicators() {
        if (!entityManager) return;

        for (Entity indicator : indicatorEntities) {
            if (indicator.IsValid()) {
                entityManager->DestroyEntity(indicator);
            }
        }
        indicatorEntities.clear();
    }

} // namespace Framework
