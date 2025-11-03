#pragma once

#include "ECSEntity.h"
#include "ECSComponent.h"
#include "Component.h"
#include "Vector2D.h"

namespace Framework {

	struct GridTiles : public Component<GridTiles> {
		int     tileId = -1;               // row-major running id
		int     x = 0;                // column index
		int     y = 0;                // row index
		Entity  entity = INVALID_ENTITY;   // this tile's entity id

		bool    blocked = false;            // for AI/pathfinding
		Entity  occupant = INVALID_ENTITY;   // who stands here (optional)

		Vector2D centerWorld{ 0.f, 0.f };
		float    tileW = 1.f;
		float    tileH = 1.f;
	};

}
