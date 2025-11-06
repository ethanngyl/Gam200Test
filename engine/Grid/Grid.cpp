/**
===============================================================================
 File:           Grid.cpp
 Author:         Josh Ong
 Email:          josh.o@digipen.edu
 Date:           2025-10-22
 Contribution:	 10%
 ------------------------------------------------------------------------------

  Brief:
  - Provides the implementation for the global Grid access function.

  Design notes:
  - The single Grid instance is declared as a static global variable within
	the Framework namespace to ensure a single, application-wide grid manager.
  - GetGrid() returns a reference to this static instance, allowing any system
	to interact with the grid properties defined in Grid.h.
===============================================================================
 */
#include "Precompiled.h"
#include "Grid.h"


namespace Framework {

	static Grid grid;
	Grid& GetGrid() {
		return grid;
	}

	// ============================================================================
	// Definition of function: RebuildSpatialPartitioning
	// author: jiahao.zhou@digipen
	// this function got 2 part, first use a loop to clear all tiles' occupant to INVALID_ENTITY
	// second use another loop to go through all entities in the entity manager, get their transform component
	// 
	// after these 2 steps, each tile is either empty or occupied by an entity
	// ============================================================================


	void RebuildSpatialPartition() {

		// get the grid
		Grid& grid = GetGrid();

		// check if entity manager is valid
		if (grid.em == nullptr) {
			return;
		}

		// clear all tiles' occupant
		for (size_t i = 0; i < grid.tiles.size(); ++i) {
			// get the tile entity
			Entity tileEntity = grid.tiles[i];
			
			// skip invalid tile entities
			if (tileEntity.GetID() == INVALID_ENTITY) {
				continue;
			}

			// check if the tile entity has GridTiles component, if yes, proceed
			if (!grid.em->HasComponent<GridTiles>(tileEntity)) {
				continue;
			}
				// get the GridTiles component
				GridTiles& gt = grid.em->GetComponent<GridTiles>(tileEntity);

				// reset occupant to INVALID_ENTITY
				gt.occupant = Entity{ INVALID_ENTITY };
			}
			// now go through all entities and insert them into the grid
            // get all entities from entity manager
			const std::vector<Entity>& allEntities = grid.em->GetAllEntities();

			// iterate through all entities
			for (size_t i = 0; i < allEntities.size(); ++i) {

				//get the entity
				Entity e = allEntities[i];

				// check if the entity has Transform component
				if (!grid.em->HasComponent<Transform>(e)) {
					// skip this entity if no transform component
					continue;
				}
				// get the Transform component
				const Transform& transform = grid.em->GetComponent<Transform>(e);

				// convert world position to grid coordinate
				auto gc = WorldToTile(transform.position);

				// skip if the grid coordinate is invalid
				if (!gc.has_value()) {
					continue;
				}

				// get the tile entity at the grid coordinate
				Entity tileEntity = grid.TileAt(gc->x, gc->y);

				// skip invalid tile entities
				if (tileEntity.GetID() == INVALID_ENTITY) {
					continue;
				}
				
				// check if the tile entity has GridTiles component, skip if no
				if (!grid.em->HasComponent<GridTiles>(tileEntity)) {
					continue;
				}
				
				// get the GridTiles component
				GridTiles& gt = grid.em->GetComponent<GridTiles>(tileEntity);
				gt.occupant = e;
				


			}
		


	}

	// ============================================================================
	// Declaration of function: SpatialPartitioningInsert
	// author: jiahao.zhou@digipen
	// ============================================================================

	void SpatialPartitioningInsert(Framework::Entity entity) {
		// get the grid
		Grid& grid = GetGrid();
		// check if entity manager is valid
		if (grid.em == nullptr) {
			return;
		}
		// check if the entity has Transform component
		if (!grid.em->HasComponent<Transform>(entity)) {
			return;
		}
		// get the Transform component
		const Transform& transform = grid.em->GetComponent<Transform>(entity);
		// convert world position to grid coordinate
		auto gc = WorldToTile(transform.position);
		// skip if the grid coordinate is invalid
		if (!gc.has_value()) {
			return;
		}
		// get the tile entity at the grid coordinate
		Entity tileEntity = grid.TileAt(gc->x, gc->y);
		// skip invalid tile entities
		if (tileEntity.GetID() == INVALID_ENTITY) {
			return;
		}
		// check if the tile entity has GridTiles component, skip if no
		if (!grid.em->HasComponent<GridTiles>(tileEntity)) {
			return;
		}
		// get the GridTiles component
		GridTiles& gt = grid.em->GetComponent<GridTiles>(tileEntity);
		// set the occupant to the entity
		gt.occupant = entity;
	}

	// ============================================================================
	// Declaration of function: SpatialPartitioningRemove
	// author: jiahao.zhou@digipen
	// ============================================================================
	void SpatialPartitioningRemove(Framework::Entity entity) {
		// get the grid
		Grid& grid = GetGrid();
		// check if entity manager is valid
		if (grid.em == nullptr) {
			return;
		}
		// check if the entity has Transform component
		if (grid.em->HasComponent<Transform>(entity)) {
			// get the Transform component
			const Transform& transform = grid.em->GetComponent<Transform>(entity);
			// convert world position to grid coordinate
			auto gc = WorldToTile(transform.position);
			// proceed if the grid coordinate is valid
			if (gc.has_value()) {
				// get the tile entity at the grid coordinate
				Entity tileEntity = grid.TileAt(gc->x, gc->y);
				// skip invalid tile entities
				if (tileEntity.GetID() != INVALID_ENTITY &&
					// check if the tile entity has GridTiles component
					grid.em->HasComponent<GridTiles>(tileEntity)) {
					// get the GridTiles component
					GridTiles& gt = grid.em->GetComponent<GridTiles>(tileEntity);
					// check if the occupant is the entity to be removed
					if (gt.occupant.GetID() == entity.GetID()) {
						// set occupant to INVALID_ENTITY
						gt.occupant = Entity{ INVALID_ENTITY };
						return;
					}
				}
			}
		}
		// if not found  Transform, do a full scan to remove the entity from any tile it occupies
		for (size_t i = 0; i < grid.tiles.size(); ++i) {
			// get the tile entity
			Entity tile = grid.tiles[i];
			// skip invalid tile entities
			if (tile.GetID() == INVALID_ENTITY) {
				continue;
			}
			// check if the tile entity has GridTiles component, if yes, proceed
			if (!grid.em->HasComponent<GridTiles>(tile)) {
				continue;
			}
			// get the GridTiles component
			GridTiles& gt = grid.em->GetComponent<GridTiles>(tile);
			
			if (gt.occupant.GetID() == entity.GetID()) {
				gt.occupant = Entity{ INVALID_ENTITY };
				break;
			}
		}
	}
} // namespace Framework

