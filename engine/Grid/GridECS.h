/**
===============================================================================
 File:           GridECS.h
 Author:         Josh Ong
 Email:          josh.o@digipen.edu
 Date:           2025-10-24
 Contribution: 	 90%
 ------------------------------------------------------------------------------

  Brief:
  - Provides the main API for interacting with the grid system using ECS components.
	This includes boundary checks, walkability queries, state modification, and
	coordinate conversions.

  Key features:
  - Defines GridCoord structure for simple integer tile coordinates.
  - **Tile State API**: Includes IsWalkable, SetBlocked, and SetOccupant, which
	abstract the process of accessing the GridTiles component via the EntityManager.
  - **Coordinate Conversion**: Provides WorldToTile and TileToWorld functions
	for converting between world-space Vector2D and grid-space GridCoord,
	based on the center of the tile.
  - All functions use the global GetGrid() accessor and rely on the EntityManager
	and GridTiles component for their operations.
===============================================================================
 */

#pragma once
#include <cmath>
#include <optional>
#include "Vector2D.h"
#include "ECSEntity.h"
#include "ECSEntityManager.h"
#include "GridTile.h"
#include "Grid.h"


namespace Framework {

		struct GridCoord {
			int x{ 0 }, y{ 0 };
		};

		//Bounds, Walkability, Editing

		inline bool InBounds(GridCoord c) {
			const auto& grid = GetGrid();
			return grid.InBounds(c.x, c.y);

		}

		inline bool IsWalkable(GridCoord c) {
			const auto& grid = GetGrid();
			if (!grid.InBounds(c.x, c.y) || !grid.em) {
				return false;
			}
			Entity e = grid.TileAt(c.x, c.y);
			if (e.GetID() == INVALID_ENTITY) {
				return false;
			}
			if (!grid.em->HasComponent<GridTiles>(e)) {
				return false;
			}
			const auto& gridTile = grid.em->GetComponent<GridTiles>(e);

			return !gridTile.blocked && gridTile.occupant.GetID() == INVALID_ENTITY;
		}

		inline bool SetBlocked(GridCoord c, bool blocked) {
			auto& grid = GetGrid();
			if (!grid.InBounds(c.x, c.y) || !grid.em) {
				return false;
			}
			Entity e = grid.TileAt(c.x, c.y);
			if (e.GetID() == INVALID_ENTITY || !grid.em->HasComponent<GridTiles>(e)) {
				return false;
			}
			auto& gridTile = grid.em->GetComponent<GridTiles>(e);
			gridTile.blocked = blocked;
			return true;
		}

		inline bool SetOccupant(GridCoord c, Entity who) {
			auto& grid = GetGrid();
			if (!grid.InBounds(c.x, c.y) || !grid.em) {
				return false;
			}
			Entity e = grid.TileAt(c.x, c.y);
			if (e.GetID() == INVALID_ENTITY || !grid.em->HasComponent<GridTiles>(e)) {
				return false;
			}
			auto& gridTile = grid.em->GetComponent<GridTiles>(e);
			gridTile.occupant = who;
			return true;
		}

		// --- World <-> Tile conversion (center-based) ---

		inline std::optional<GridCoord> WorldToTile(const Vector2D& world) {
			const auto& grid = GetGrid();
			// guard zero spacing
			

			const float fx = (world.x - grid.startPos.x) / grid.spacing.x;
			const float fy = (world.y - grid.startPos.y) / grid.spacing.y;

			// nearest integer tile center
			const int tx = static_cast<int>(std::floor(fx + 0.5f));
			const int ty = static_cast<int>(std::floor(fy + 0.5f));

			if (!grid.InBounds(tx, ty)) {
				return std::nullopt;
			}
			return GridCoord{ tx, ty };
		}

		inline Vector2D TileToWorld(GridCoord c) {
			const auto& grid = GetGrid();
			return Vector2D{
			  grid.startPos.x + static_cast<float>(c.x) * grid.spacing.x,
			  grid.startPos.y + static_cast<float>(c.y) * grid.spacing.y
			};
		}
		// ============================================================================
		// Declaration of function: RebuildSpatialPartitioning
		// author: jiahao.zhou@digipen
		// ============================================================================
		
		void RebuildSpatialPartition();
		// ============================================================================
		// Declaration of function: SpatialPartitioningInsert
		// author: jiahao.zhou@digipen
		// ============================================================================
		void SpatialPartitioningInsert(Framework::Entity entity);

		// ============================================================================
		// Declaration of function: SpatialPartitioningRemove
		// author: jiahao.zhou@digipen
		// ============================================================================
		void SpatialPartitioningRemove(Framework::Entity entity);
}
