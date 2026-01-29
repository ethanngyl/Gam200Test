#include "Precompiled.h"

namespace Framework {
	void ParticleSystem::CreateParticle() {
		if (!active) return; // Inactive not create
		if (!CORE || !CORE->GetGraphicsSystem() || !CORE->GetEntityManager()) return; // safety check

		// Query active graphics system to convert pixel size into world-space scale
		// Size (pixel -> world conversion)
		auto* graphics = CORE->GetGraphicsSystem();
		//float pixelSize = 15.0f; // desired size in pixels
		float worldScale = (2.0f * settings.size) / float(graphics->GetRenderHeight());

		Entity entity = CORE->GetEntityManager()->CreateEntity();
		
		CORE->GetEntityManager()->AddComponent<Transform>(entity, emitter);
		auto& transform = CORE->GetEntityManager()->GetComponent<Transform>(entity);
		transform.position = emitter;
		transform.scale = { worldScale , worldScale };	// Size

		CORE->GetEntityManager()->AddComponent<Particle>(entity);
		auto& particle = CORE->GetEntityManager()->GetComponent<Particle>(entity);
		// Store start values for interpolation
		particle.startTint = settings.tint;
		particle.endTint = settings.endTint;
		particle.startSize = worldScale;
		particle.endSize = settings.endSize;
		particle.gravity = settings.gravity;
		particle.fadeOut = settings.fadeOut;
		particle.shrinkOverTime = settings.shrinkOverTime;

		// Sprite
		CORE->GetEntityManager()->AddComponent<Sprite>(entity);
		auto& sprite = CORE->GetEntityManager()->GetComponent<Sprite>(entity);
		sprite.texturePath = settings.texturePath;
		sprite.layer = settings.layer;
		sprite.tint = settings.tint;

		// frand: gives any random number from 0.0f to 1.0f
		auto frand = []() { return float(std::rand()) / float(RAND_MAX); };
		
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
			direction.x += ((frand() * 2.0f) - 1.0f) * settings.directionFuzz;
			direction.y += ((frand() * 2.0f) - 1.0f) * settings.directionFuzz;

			// direction normalized
			direction.normalize();
		}

		particle.velocity = { direction.x * speed, direction.y * speed };

		particles.push_back(entity);
	}

	void ParticleSystem::Update(float dt) {
		if (dt <= 0.0f) return; // saftey check

		EntityManager* entityManager = CORE->GetEntityManager();
		if (!entityManager) return;

		// spawn new particles over time
		if (spawnRate > 0.0f) {
			spawnAcc += dt * spawnRate; // how many particles we "owe"
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
			auto& sprite = entityManager->GetComponent<Sprite>(entity);

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
					sprite.tint.r = particle.startTint.r +
						(particle.endTint.r - particle.startTint.r) * lifeProgress;
					sprite.tint.g = particle.startTint.g +
						(particle.endTint.g - particle.startTint.g) * lifeProgress;
					sprite.tint.b = particle.startTint.b +
						(particle.endTint.b - particle.startTint.b) * lifeProgress;
					sprite.tint.a = particle.startTint.a +
						(particle.endTint.a - particle.startTint.a) * lifeProgress;
				}

				if (particle.shrinkOverTime) {
					// Shrink over time
					float currentSize = particle.startSize +
						(particle.endSize - particle.startSize) * lifeProgress;
					transform.scale = { currentSize, currentSize };
				}
				else if (particle.growOverTime) {
					// Grow over time
					float currentSize = particle.endSize +
						(particle.startSize - particle.endSize) * (1.0f - lifeProgress);
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