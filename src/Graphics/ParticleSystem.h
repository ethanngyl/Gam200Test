#pragma once
#include "Precompiled.h"
#include "Component.h"		// for Particle

namespace Framework {
	class ParticleSystem {
	public:
		ParticleSystem();
		//~Particle	System();

		void CreateParticle();
		void Update(float dt);

		void SetSpawnRate(float perSecond) { spawnRate = perSecond; }

	private:
		std::vector<Entity> particles;
		Vector2D emitter { 0.0f, 0.0f };
		Vector2D dir{ 0.0f, 0.0f };
				
		float spawnRate = 20.0f; // particles per second
		float spawnAcc = 0.0f;   // accumulator
	};
} // namespace Framework