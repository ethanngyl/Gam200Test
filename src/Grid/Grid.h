/**
===============================================================================
 File:           Grid.h
 Author:         Josh Ong
 Email:          josh.o@digipen.edu
 Date:           2025-10-22
 Contribution:	 100%
 ------------------------------------------------------------------------------

  Brief:
  - Defines the core data structure for the 2D tile grid. The Grid struct stores
	the dimensions, world-space properties, and a list of entities representing
	the individual tiles.

  Key features:
  - Stores grid dimensions (rows/cols) and physical properties (startPos, spacing).
  - Holds a vector of Entity IDs for all tiles in row-major order.
  - Provides utility methods for boundary checking (InBounds) and index calculation (Index).
  - Offers a quick lookup method for a tile entity at a specific coordinate (TileAt).
  - Provides a global access point (GetGrid) for the single Grid instance.
===============================================================================
 */

#pragma once

#include "ECSEntity.h"
#include "Vector2D.h"
#include <vector>

namespace Framework {

	class EntityManager;

	struct Grid {
		int rows = 0, cols = 0;
		Vector2D startPos{ 0.0f, 0.0f };
		Vector2D spacing{ 1.0f, 1.0f };
		Vector2D tileSize{ 1.0f, 1.0f };
		// ========================================================================
		// World-space minimum corner of the grid (bottom-left).
		// Used for spatial queries, bounds checking, and camera/grid interactions.
		// Author: Sim Kah Yan
		// ========================================================================
		Vector2D worldbound_min{};
		// ========================================================================
		// World-space maximum corner of the grid (top-right).
		// Defines the outer extent of the grid area in world coordinates.
		// Author: Sim Kah Yan
		// ========================================================================
		Vector2D worldbound_max{};

		std::vector<Entity> tiles;
		EntityManager* em = nullptr;

		inline bool InBounds(int x, int y) const {
			return x >= 0 && y >= 0 && x < cols && y < rows;
		}

		inline size_t Index(int x, int y) const {
			return static_cast<size_t>(y) * static_cast<size_t>(cols) + static_cast<size_t>(x);
		}

		inline Entity TileAt(int x, int y) const {
			if (!InBounds(x, y)) {
				return Entity{ INVALID_ENTITY };
			}
			return tiles[Index(x, y)];
		}
	};

	Grid& GetGrid();

}
