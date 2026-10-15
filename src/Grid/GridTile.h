/**
===============================================================================
 File:           GridTile.h
 Author:         Josh Ong
 Email:          josh.o@digipen.edu
 Date:           2025-10-24
 Contribution:	 100%
 ------------------------------------------------------------------------------

  Brief:
  - Defines the GridTiles component, which is attached to every tile entity
	in the ECS to store tile-specific properties.

  Key features:
  - Inherits from Component, integrating seamlessly with the ECS.
  - Stores grid coordinates (x, y) and tile identification (tileId, entity).
  - Contains pathfinding data: 'blocked' status and the 'occupant' entity ID.
  - Stores world-space geometry data (centerWorld, tileW, tileH).
===============================================================================
 */

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

		float neighborLenX() const { return tileW; }
		float neighborLenY() const { return tileH; }
	};

}
