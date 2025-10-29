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
