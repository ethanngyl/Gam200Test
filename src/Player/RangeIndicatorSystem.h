#pragma once

#include "Interface.h"
#include "ECSEntityManager.h"
#include "GraphicsSystemV2.h"
#include <vector>

namespace Framework {

    // Grid position helper
    struct GridPosition {
        int x;
        int y;

        bool operator==(const GridPosition& other) const {
            return x == other.x && y == other.y;
        }
    };

    /**
     * @brief System that displays attack range indicators on grid tiles
     */
    class RangeIndicatorSystem : public EngineSystem {
    public:
        RangeIndicatorSystem();
        ~RangeIndicatorSystem();

        void Initialize() override;
        void Update(float dt) override;
        void SendEngineMessage(Message* msg) override;

        // Dependencies
        void SetEntityManager(EntityManager* em) { entityManager = em; }
        void SetGraphicsSystem(GraphicsSystemV2* gfx) { graphicsSystem = gfx; }

        // Grid configuration
        void SetGridCellSize(float size) { gridCellSize = size; }
        void SetGridOffset(const Vector2D& offset) { gridOffset = offset; }

        // Range indicator control
        void ShowRangeForEntity(Entity entity);
        void HideRange();
        void ToggleRange(Entity entity);

        // Check if a grid position is in range
        bool IsPositionInRange(const GridPosition& pos, const GridPosition& center,
            int minRange, int maxRange) const;

    private:
        EntityManager* entityManager;
        GraphicsSystemV2* graphicsSystem;

        std::vector<Entity> indicatorEntities;  // Active indicator tiles
        Entity currentRangeEntity;              // Entity whose range is shown

        float gridCellSize = 1.0f;              // Size of one grid cell in world units
        Vector2D gridOffset = Vector2D(0, 0);   // Offset for grid alignment

        // Helper functions
        GridPosition WorldToGrid(const Vector2D& worldPos) const;
        Vector2D GridToWorld(const GridPosition& gridPos) const;
        std::vector<GridPosition> GetTilesInRange(const GridPosition& center,
            int minRange, int maxRange) const;
        void CreateTileIndicator(const GridPosition& gridPos);
        void ClearIndicators();
    };

} // namespace Framework