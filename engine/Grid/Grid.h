#pragma once

#include "Precompiled.h"

namespace Framework {

	struct GridCoord { int x{ 0 }, y{ 0 }; };
	struct GridBounds { int cols{ 0 }, rows{ 0 }; };

	enum class DiagonalRule : uint8_t { None4Way, Allow8Way, NoCutCorners };


	struct GridConfig {
		int cols{ 0 };
		int rows{ 0 };
		float tileW{ 1.0f };
		float tileH{ 1.0f };
		Vector2D originWorld{ 0.0f, 0.0f }; //world pos of tile (0,0) center
		DiagonalRule diag{ DiagonalRule::None4Way };
	};

	struct GridCell {
		bool blocked{ false };
		Entity occupant{ INVALID_ENTITY };
	
	};

	struct GridRuntime {
		GridBounds bounds{};
		float tileW{ 1.0f };
		float tileH{ 1.0f };
		Vector2D originWorld{};
		DiagonalRule diag{ DiagonalRule::None4Way };
		std::vector<GridCell> cells; //size = rows * cols
	};

	namespace GridAPI {
		//bake/initialize the grid with a configuration. returns true on success
		bool Initialize(const GridConfig& cfg);

		GridBounds Bounds();

		//Bounds / Walkability / editing
		bool InBounds(GridCoord c);
		bool IsWalkable(GridCoord c);
		bool SetBlocked(GridCoord c, bool blocked);
		bool SetOccupant(GridCoord c, Entity who); //who == INVALID_ENTITY clears

		//World <-> tile conversion (center-based)
		std::optional <GridCoord> WorldToTile(const Vector2D& world);
		Vector2D TileToWorld(GridCoord c);
	}
		

		class GridSystem :public EngineSystem {
		public:
			GridSystem() = default;
			virtual ~GridSystem() = default;

			// Init hook (kept minimal, bake via GridAPI::Initialize).
			void Initialize() override;

			// Optional per-frame hooks (e.g., debug draw).
			void Update(float dt) override;

			// Message plumbing if you later broadcast GridReady/TileClicked.
			void SendEngineMessage(Message* message) override;

			// Programmatic bake for tools/ImGui (same as GridAPI::Initialize).
			static bool Bake(const GridConfig& cfg);

			//Access baked runtime for advanced tools
			static const GridRuntime& Runtime();

		private: 
			static GridRuntime s_rt;

			static inline size_t Index(GridCoord c) {
				return static_cast<size_t>(c.y) * static_cast<size_t>(s_rt.bounds.cols) + static_cast<size_t>(c.x);
			}
		};
	
}
