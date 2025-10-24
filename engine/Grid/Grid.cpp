/*!************************************************************************
\file      Grid.cpp
\author
\par DP email:
\par Course:
\par Assignment:
\date     2025-10-22
\brief
  Implementation of GridSystem and GridAPI for a 2D tile grid.
*************************************************************************/
#include "Precompiled.h"
#include "Grid.h"
#include <cmath>      // std::floor
#include <algorithm>  // std::clamp

namespace Framework {

    // -------- Static storage ------------------------------------------------
    GridRuntime GridSystem::s_rt{};

    // -------- GridSystem ----------------------------------------------------
    void GridSystem::Initialize() {
        // Intentionally empty — actual bake is done via GridAPI::Initialize or Bake().
    }

    void GridSystem::Update(float /*dt*/) {
        // Optional: add debug draw or ImGui here later. No-op for now.
    }

    void GridSystem::SendEngineMessage(Message* /*message*/) {
        // Optional: react to GridReady/TileClicked if you add those messages later.
    }

    bool GridSystem::Bake(const GridConfig& cfg) {
        s_rt.bounds = { cfg.cols, cfg.rows };
        s_rt.tileW = cfg.tileW;
        s_rt.tileH = cfg.tileH;
        s_rt.originWorld = cfg.originWorld;
        s_rt.diag = cfg.diag;

        const size_t N = static_cast<size_t>((std::max)(0, cfg.cols))
            * static_cast<size_t>((std::max)(0, cfg.rows));
        s_rt.cells.assign(N, GridCell{});

        return true;
    }

    const GridRuntime& GridSystem::Runtime() { return s_rt; }

    // -------- GridAPI -------------------------------------------------------
    namespace GridAPI {

        bool Initialize(const GridConfig& cfg) { return GridSystem::Bake(cfg); }

        GridBounds Bounds() { return GridSystem::Runtime().bounds; }

        bool InBounds(GridCoord c) {
            const auto& rt = GridSystem::Runtime();
            return (c.x >= 0 && c.y >= 0 && c.x < rt.bounds.cols && c.y < rt.bounds.rows);
        }

        bool IsWalkable(GridCoord c) {
            if (!InBounds(c)) return false;
            const auto& rt = GridSystem::Runtime();
            const auto  idx = static_cast<size_t>(c.y) * rt.bounds.cols + static_cast<size_t>(c.x);
            const auto& cell = rt.cells[idx];
            return !cell.blocked && (cell.occupant.GetID() == INVALID_ENTITY);
        }

        bool SetBlocked(GridCoord c, bool blocked) {
            if (!InBounds(c)) return false;
            auto& rt = const_cast<GridRuntime&>(GridSystem::Runtime());
            const auto idx = static_cast<size_t>(c.y) * rt.bounds.cols + static_cast<size_t>(c.x);
            rt.cells[idx].blocked = blocked;
            return true;
        }

        bool SetOccupant(GridCoord c, Entity who) {
            if (!InBounds(c)) return false;
            auto& rt = const_cast<GridRuntime&>(GridSystem::Runtime());
            const auto idx = static_cast<size_t>(c.y) * rt.bounds.cols + static_cast<size_t>(c.x);
            rt.cells[idx].occupant = who; // who.id==0 means empty
            return true;
        }

        std::optional<GridCoord> WorldToTile(const Vector2D& world) {
            const auto& rt = GridSystem::Runtime();

            // Assume originWorld is the CENTER of (0,0).
            const float lx = world.x - rt.originWorld.x;
            const float ly = world.y - rt.originWorld.y;

            const int tx = static_cast<int>(std::floor(lx / rt.tileW + 0.5f));
            const int ty = static_cast<int>(std::floor(ly / rt.tileH + 0.5f));

            GridCoord c{ tx, ty };
            return InBounds(c) ? std::optional<GridCoord>(c) : std::nullopt;
        }

        Vector2D TileToWorld(GridCoord c) {
            const auto& rt = GridSystem::Runtime();
            return Vector2D{ rt.originWorld.x + c.x * rt.tileW,
                             rt.originWorld.y + c.y * rt.tileH };
        }

    } // namespace GridAPI

} // namespace Framework

