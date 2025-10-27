//#pragma once
//#include "Precompiled.h"
//#include "Grid.h"
//#include "GridTile.h"
//
//namespace Framework {
//
//	inline std::vector<Entity>& TileEntitiesStorage() {
//		static std::vector<Entity> tileEntities;
//		return tileEntities;
//	}
//
//	inline void BuildGridTiles(EntityManager* em) {
//		auto bound = GridAPI::Bounds();
//
//		const auto& runTime = GridSystem::Runtime();
//
//		auto& tiles = TileEntitiesStorage();
//		tiles.assign(bound.cols * bound.rows, Entity{ INVALID_ENTITY });
//
//		for (int y = 0; y < bound.rows; ++y) {
//			for (int x = 0; x < bound.cols; ++x) {
//				GridCoord c{ x,y };
//				Vector2D center = GridAPI::TileToWorld(c);
//
//				//create ecs Entity
//				Entity e = em->CreateEntity();
//
//				//transform (center + scale to tile size)
//				em->AddComponent<Transform>(e);
//				auto& transform = em->GetComponent<Transform>(e);
//				transform.position = center;	//world center
//				transform.rotation = 0.0f;
//				transform.scale = { runTime.tileW * 0.9f, runTime.tileH * 0.9f };
//
//				//Render (use legacy spritename "quad") is mapped in GraphicsSystemV2
//				em->AddComponent<Renderable>(e);
//				auto& render = em->GetComponent<Renderable>(e);
//				render.visible = true;
//				render.layer = -500; //behind everything
//				render.orderInLayer = 0;
//				render.spriteName = "quad"; //map to quad mesh
//
//				 //Chessboard tints (semi-transparent so background shows through)
//				/*const bool dark = ((x + y) & 1) != 0;
//				render.tint = dark ? glm::vec4(0.12f, 0.12f, 0.13f, 0.6f)
//					: glm::vec4(0.18f, 0.18f, 0.20f, 0.6f);*/
//
//				//tag with gridTile
//				em->AddComponent<GridTile>(e);
//				auto& tile = em->GetComponent<GridTile>(e);
//				tile.x = x;
//				tile.y = y;
//
//				tiles[y * bound.cols + x] = e;
//			}
//		}
//	}
//
//	//helper to get the entity for any given tile
//	inline Entity TileEntityAt(int x, int y) {
//		auto bound = GridAPI::Bounds();
//
//		if (x < 0 || y < 0 || x >= bound.cols || y >= bound.rows) {
//			return Entity{ INVALID_ENTITY };
//		}
//
//		return TileEntitiesStorage()[y * bound.cols + x];
//
//	}
//
//}