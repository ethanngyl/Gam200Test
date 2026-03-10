#include "Precompiled.h"

namespace Framework {
	void ParticleSystem::CreateParticle() {
		if (!active) return;
		if (!CORE || !CORE->GetGraphicsSystem() || !CORE->GetEntityManager()) return; // safety check

		// Query active graphics system to convert pixel size into world-space scale
		// Size (pixel -> world conversion)
		auto* graphics = CORE->GetGraphicsSystem();
		float worldScale = settings.size * 0.1f;  // size is now in tile units (1.0 = one tile)

		Entity entity = CORE->GetEntityManager()->CreateEntity();
		
		CORE->GetEntityManager()->AddComponent<Transform>(entity, emitter);
		auto& transform = CORE->GetEntityManager()->GetComponent<Transform>(entity);
		transform.position = emitter;
		transform.scale = { worldScale, worldScale };	// Size

		CORE->GetEntityManager()->AddComponent<Particle>(entity);
		auto& particle = CORE->GetEntityManager()->GetComponent<Particle>(entity);

		// frand: gives any random number from 0.0f to 1.0f
		auto frand = []() { return float(std::rand()) / float(RAND_MAX); };

		// Store start values for interpolation
		particle.startTint = settings.tint;
		particle.endTint = settings.endTint;
		particle.startSize = worldScale;
		particle.endSize = worldScale * settings.endSize;
		particle.gravity = settings.gravity;
		particle.fadeOut = settings.fadeOut;
		particle.shrinkOverTime = settings.shrinkOverTime;
		particle.growOverTime = settings.growOverTime;

		// Mesh (Body for the particle)
		CORE->GetEntityManager()->AddComponent<MeshRenderer>(entity);
		auto& mr = CORE->GetEntityManager()->GetComponent<MeshRenderer>(entity);
		mr.layer = settings.layer;
		mr.tint = settings.tint;
		
		// Lifetime
		particle.lifetime = settings.minLifetime + frand() * (settings.maxLifetime - settings.minLifetime);
		particle.maxLifetime = particle.lifetime;  // Store max lifetime for interpolation
		particle.age = 0.0f;

		// Velocity: Radial or Directional cone
		float speed = settings.minSpeed + frand() * (settings.maxSpeed - settings.minSpeed);

		Vector2D direction { 0.0f, 0.0f };

		if (settings.direction.x == 0.0f && settings.direction.y == 0.0f) {
			// Radial Emission
			// scale up by 2 then shift by -1 (Range: -1 to 1)
			direction.x = (frand() * 2.0f) - 1.0f;
			direction.y = (frand() * 2.0f) - 1.0f;

			// direction normalized
			direction.normalize();
		}
		else {
			// Directional with fuzz
			direction = settings.direction;

			// scale up by 2 then shift by -1 (Range: -1 to 1)
			//direction.x += ((frand() * 2.0f) - 1.0f) * settings.directionFuzz;
			//direction.y += ((frand() * 2.0f) - 1.0f) * settings.directionFuzz;

			direction.y = std::abs(direction.y); // push mostly upward for smoke
			direction.x += ((frand() * 2.0f) - 1.0f) * 0.2f; // a bit more sideways

			// direction normalized
			direction.normalize();
		}

		particle.velocity = { direction.x * speed, direction.y * speed };

		particles.push_back(entity);
	}

	void ParticleSystem::Update(float dt) {
		if (dt <= 0.0f) return; // saftey check

		if (!active) { spawnAcc = 0.0f; return; }

		EntityManager* entityManager = CORE->GetEntityManager();
		if (!entityManager) return;

		// spawn new particles over time
		if (settings.spawnRate > 0.0f) {
			spawnAcc += dt * settings.spawnRate; // how many particles we "owe"
			while (spawnAcc >= 1.0f) {
				CreateParticle();
				spawnAcc -= 1.0f;
			}
		}

		// update existing particles
		for (auto count = particles.begin(); count != particles.end(); ) {
			Entity entity = *count;

			// Checks if components if is exist (prevents GetComponent throwing if something removed it)
			if (!entityManager->HasComponent<Transform>(entity) || !entityManager->HasComponent<Particle>(entity)) {
				count = particles.erase(count);
				continue;
			}

			auto& transform = entityManager->GetComponent<Transform>(entity);
			auto& particle = entityManager->GetComponent<Particle>(entity);
			auto& mr = entityManager->GetComponent<MeshRenderer>(entity);

			// Apply physics
			if (particle.gravity.x != 0.0f || particle.gravity.y != 0.0f) {
				particle.velocity.x += particle.gravity.x * dt;
				particle.velocity.y += particle.gravity.y * dt;
			}

			// Update position
			transform.position.x += particle.velocity.x * dt;
			transform.position.y += particle.velocity.y * dt;

			// Update age
			particle.age += dt;

			// Calculate life progress (0 to 1)
			float lifeProgress = particle.age / particle.maxLifetime;

			// Update visual properties based on life progress
			if (lifeProgress >= 0.0f && lifeProgress <= 1.0f) {
				if (particle.fadeOut) {
					// Fade out tint
					mr.tint.r = particle.startTint.r +
						(particle.endTint.r - particle.startTint.r) * lifeProgress;
					mr.tint.g = particle.startTint.g +
						(particle.endTint.g - particle.startTint.g) * lifeProgress;
					mr.tint.b = particle.startTint.b +
						(particle.endTint.b - particle.startTint.b) * lifeProgress;
					mr.tint.a = particle.startTint.a +
						(particle.endTint.a - particle.startTint.a) * lifeProgress;
				}

				if (particle.shrinkOverTime || particle.growOverTime)
				{
					float lifeProgress = particle.age / particle.maxLifetime;
					float currentSize = particle.startSize +
						(particle.endSize - particle.startSize) * lifeProgress;
					transform.scale = { currentSize, currentSize };
				}
			}

			// Check lifetime
			if (particle.age >= particle.maxLifetime) {
				entityManager->DestroyEntity(entity);
				count = particles.erase(count);
			}
			else {
				++count;
			}
		}
	}
} // namespace Framework