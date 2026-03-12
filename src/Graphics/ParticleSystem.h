#pragma once

#include "Vector2D.h"
#include "ECSEntity.h"

#include <string>
#include <vector>
#include <glm/glm.hpp>

namespace Framework {
	class ParticleSystem {
	public:
		struct Settings {
			glm::vec4	tint		{ 0.0f, 0.0f, 0.0f, 1.0f };
			int			layer		{ 0 };
			float		spawnRate	{ 0.0f };
			float		maxLifetime { 0.0f };
			float		minLifetime { 0.0f };
			float		maxSpeed	{ 0.0f };
			float		minSpeed	{ 0.0f };
			float		size		{ 0.0f };	// visual size (pixels)
			int			burstCnt	{ 0 };		// 0 = continuous; >0 = spawn this many once at start

			// directional emission (for smoke up / explosion radial)
			Vector2D	direction		{ 0.0f, 0.0f }; // (0,0) = radial/random
			float		directionFuzz	{ 0.0f };		// 0 = straight, 1 = wide cone
			float		spawnRadius		{ 0.0f };		// random offset from emitter position

			glm::vec4	endTint{ 1.0f, 1.0f, 1.0f, 0.0f }; // For fade out
			float		endSize{ 0.0f }; // For size change
			Vector2D	gravity{ 0.0f, 0.0f };
			bool		fadeOut{ true };
			bool		shrinkOverTime{ false };
			bool		growOverTime{ false };

			int         ownerPlayerID{ -1 };  // -1 = always active, 0-2 = specific player
		};

		void CreateParticle();
		void Update(float dt);

		void SetEmitter(float x, float y) { emitter = { x, y }; }
		void SetFollowEntity(EntityID id) { followEntity = id; }
		void SetSettings(const Settings& s) { settings = s; }

		const Settings& GetSettings() const { return settings; }
		Vector2D GetEmitter() const { return emitter; }

		bool IsActive() const { return active; }
		void SetActive(bool a) { active = a;  }

		size_t GetParticle() const { return particles.size(); } // Get particle count

		void SpawnBurst(int count) { for (int i{ 0 }; i < count; ++i) CreateParticle(); } // Burst spawning

		void SetOwnerPlayer(int playerID) { settings.ownerPlayerID = playerID; }
		int GetOwnerPlayer() const { return settings.ownerPlayerID; }

	private:
		std::vector<Entity> particles;
		Vector2D emitter { 0.0f, 0.0f };
		EntityID followEntity = INVALID_ENTITY;  // if set, emitter tracks this entity every frame
		float spawnAcc = 0.0f;   // accumulator
		bool active = true;	     // toggle controlled emitter

		Settings settings;
	};
} // namespace Framework