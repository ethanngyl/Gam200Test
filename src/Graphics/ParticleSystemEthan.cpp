#include "Precompiled.h"
#include "ParticleSystemEthan.h"

namespace Framework {
	void ParticleSystemEthan::CreateParticle() {
		if (!active) return;
		if (!CORE->GetEntityManager()) return; // safety check

		float worldScale = settings.size * 0.1f;  // size is now in tile units (1.0 = one tile)

		EntityManager* em = CORE->GetEntityManager();
		Entity entity = em->CreateEntity();

		// Transform
		em->AddComponent<Transform>(entity);
		auto& transform = em->GetComponent<Transform>(entity);
		transform.position = emitter;
		transform.scale = { worldScale, worldScale };

		// Particle
		em->AddComponent<Particle>(entity);
		auto& particle = em->GetComponent<Particle>(entity);

		// frand: gives any random number from 0.0f to 1.0f
		auto frand = []() { return float(std::rand()) / float(RAND_MAX); };

		// Store start values
		particle.startTint = settings.tint;
		particle.endTint = settings.endTint;
		particle.startSize = worldScale;
		particle.endSize = worldScale * settings.endSize;
		particle.gravity = settings.gravity;
		particle.fadeOut = settings.fadeOut;
		particle.shrinkOverTime = settings.shrinkOverTime;
		particle.growOverTime = settings.growOverTime;

		// MeshRenderer
		em->AddComponent<MeshRenderer>(entity);
		auto& mr = em->GetComponent<MeshRenderer>(entity);
		mr.layer = settings.layer;
		mr.tint = settings.tint;

		// Lifetime
		particle.lifetime = settings.minLifetime + frand() * (settings.maxLifetime - settings.minLifetime);
		particle.maxLifetime = particle.lifetime;
		particle.age = 0.0f;

		// Velocity
		float speed = settings.minSpeed + frand() * (settings.maxSpeed - settings.minSpeed);
		Vector2D direction{ 0.0f, 0.0f };

		if (settings.direction.x == 0.0f && settings.direction.y == 0.0f) {
			// Radial: random direction in all directions
			direction.x = (frand() * 2.0f) - 1.0f;
			direction.y = (frand() * 2.0f) - 1.0f;
			direction.normalize();
		}
		else {
			// Directional with fuzz
			direction = settings.direction;
			direction.x += ((frand() * 2.0f) - 1.0f) * settings.directionFuzz;
			direction.y += ((frand() * 2.0f) - 1.0f) * settings.directionFuzz;
			direction.normalize();
		}

		particle.velocity = { direction.x * speed, direction.y * speed };
		particles.push_back(entity);
	}

	void ParticleSystemEthan::Update(float dt) {
		if (dt <= 0.0f) return; // safety check
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

			// Physics
			particle.velocity.x += particle.gravity.x * dt;
			particle.velocity.y += particle.gravity.y * dt;

			// Position
			transform.position.x += particle.velocity.x * dt;
			transform.position.y += particle.velocity.y * dt;


			// Update age
			particle.age += dt;

			// Calculate life progress (0 to 1)
			float lifeProgress = particle.age / particle.maxLifetime;

			// Tint interpolation
			if (particle.fadeOut) {
				mr.tint.r = particle.startTint.r + (particle.endTint.r - particle.startTint.r) * lifeProgress;
				mr.tint.g = particle.startTint.g + (particle.endTint.g - particle.startTint.g) * lifeProgress;
				mr.tint.b = particle.startTint.b + (particle.endTint.b - particle.startTint.b) * lifeProgress;
				mr.tint.a = particle.startTint.a + (particle.endTint.a - particle.startTint.a) * lifeProgress;
			}

			// Size interpolation
			if (particle.shrinkOverTime || particle.growOverTime) {
				float currentSize = particle.startSize + (particle.endSize - particle.startSize) * lifeProgress;
				transform.scale = { currentSize, currentSize };
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
