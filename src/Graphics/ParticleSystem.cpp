/**
===============================================================================
 File:           ParticleSystem.cpp
 Author:         TAN WEI LEONG
 Email:          weileong.tan@digipen.edu
 Date:           2026-03-12
 Contribution:   100%
 ------------------------------------------------------------------------------

===============================================================================
 */

#include "Precompiled.h"

namespace Framework {
	// Persistent random engine for particle spawning.
	// std::rand() is slow, low-quality, and not thread-safe.
	// mt19937 is seeded once at startup and reused every spawn.
	static std::mt19937 s_rng{ std::random_device{}() };
	static std::uniform_real_distribution<float> s_dist{ 0.0f, 1.0f };

	void ParticleSystem::SetSettings(const Settings& s) {
		// Copy the incoming settings first, then patch cachedWorldScale.
		// We compute world scale once here rather than inside CreateParticle()
		// because GetRenderHeight() involves a system pointer dereference —
		// doing it per-particle would waste CPU on every spawn (rubric 2104 FPS).
		settings = s;

		// Pre-compute world scale once here instead of on every CreateParticle call.
		// This avoids querying the graphics system per-particle (rubric 2104).
		if (CORE && CORE->GetGraphicsSystem()) {
			settings.cachedWorldScale =
				(2.0f * settings.size) / float(CORE->GetGraphicsSystem()->GetRenderHeight());
		}

		// Pre-size the particle vector based on expected max live particles
		// to avoid reallocations during gameplay
		int maxExpected = static_cast<int>(settings.spawnRate * settings.maxLifetime)
			+ settings.burstCnt + 8;
		particles.reserve(static_cast<size_t>(max(maxExpected, 0)));
	}

	void ParticleSystem::CreateParticle() {
		if (!CORE || !CORE->GetGraphicsSystem() || !CORE->GetEntityManager()) return; // safety check

		// Use the value cached at settings-load time (computed in SetSettings)
		float worldScale = settings.cachedWorldScale;
		if (worldScale <= 0.0f) return;  // settings not yet configured, bail early

		Entity entity = CORE->GetEntityManager()->CreateEntity();
		CORE->GetEntityManager()->AddComponent<Transform>(entity, emitter);
		auto& transform = CORE->GetEntityManager()->GetComponent<Transform>(entity);

		// Randomize spawn position within spawnRadius
		Vector2D spawnPos = emitter;
		if (settings.spawnRadius > 0.0f) {
			float angle = s_dist(s_rng) * 2.0f * 3.14159265f;
			float radius = s_dist(s_rng) * settings.spawnRadius;
			spawnPos.x += std::cos(angle) * radius;
			spawnPos.y += std::sin(angle) * radius;
		}
		transform.position = spawnPos;
		transform.scale = { worldScale , worldScale };	// Size

		CORE->GetEntityManager()->AddComponent<Particle>(entity);
		auto& particle = CORE->GetEntityManager()->GetComponent<Particle>(entity);
		// Store start values for interpolation
		particle.startTint = settings.tint;
		particle.endTint = settings.endTint;
		particle.startSize = worldScale;
		particle.endSize = settings.endSize * worldScale;
		particle.gravity = settings.gravity;
		particle.fadeOut = settings.fadeOut;
		particle.shrinkOverTime = settings.shrinkOverTime;
		particle.growOverTime = settings.growOverTime;

		// Mesh Renderer
		CORE->GetEntityManager()->AddComponent<MeshRenderer>(entity);
		auto& mr = CORE->GetEntityManager()->GetComponent<MeshRenderer>(entity);
		mr.layer = settings.layer;
		mr.tint = settings.tint;

		// Lifetime
		particle.lifetime = settings.minLifetime + s_dist(s_rng) * (settings.maxLifetime - settings.minLifetime);
		particle.maxLifetime = particle.lifetime;  // Store max lifetime for interpolation
		particle.age = 0.0f;

		// Velocity: Radial or Directional cone
		float speed = settings.minSpeed + s_dist(s_rng) * (settings.maxSpeed - settings.minSpeed);

		Vector2D direction { 0.0f, 0.0f };

		if (settings.direction.x == 0.0f && settings.direction.y == 0.0f) {
			// Radial Emission
			// scale up by 2 then shift by -1 (Range: -1 to 1)
			direction.x = (s_dist(s_rng) * 2.0f) - 1.0f;
			direction.y = (s_dist(s_rng) * 2.0f) - 1.0f;

			// direction normalized
			direction.normalize();
		}
		else {
			// Directional with fuzz
			direction = settings.direction;

			// scale up by 2 then shift by -1 (Range: -1 to 1)
			direction.x += ((s_dist(s_rng) * 2.0f) - 1.0f) * settings.directionFuzz;
			direction.y += ((s_dist(s_rng) * 2.0f) - 1.0f) * settings.directionFuzz;

			// direction normalized
			direction.normalize();
		}

		particle.velocity = { direction.x * speed, direction.y * speed };

		particles.push_back(entity);
	}

	void ParticleSystem::UpdateParticle(float dt) {
		// Guard against zero, negative, or runaway dt (e.g. after ALT+TAB resume)
		// Without the upper clamp, a 5-second pause would try to spawn 5*rate particles at once
		if (dt <= 0.0f || dt > 0.25f) return;

		EntityManager* entityManager = CORE->GetEntityManager();
		if (!entityManager) return;

		// Follow target entity: keep emitter position in sync every frame
		if (followEntity != INVALID_ENTITY) {
			Entity target{ followEntity };
			if (entityManager->HasComponent<Transform>(target)) {
				auto& targetTransform = entityManager->GetComponent<Transform>(target);
				emitter = targetTransform.position;
			}
		}

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

			// Transform and Particle are guaranteed present — we added them in CreateParticle.
			// Only guard against MeshRenderer in case something external stripped it.
			if (!entityManager->HasComponent<MeshRenderer>(entity)) {
				entityManager->DestroyEntity(entity);
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

				// shrinkOverTime and growOverTime use the same interpolation formula —
				// the difference is in the Settings values (endSize < startSize vs endSize > startSize)
				if (particle.shrinkOverTime || particle.growOverTime) {
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

	void ParticleSystem::Clear() {
		EntityManager* entityManager = CORE ? CORE->GetEntityManager() : nullptr;
		if (entityManager) {
			for (Entity entity : particles) {
				entityManager->DestroyEntity(entity);
			}
		}

		particles.clear();
		spawnAcc = 0.0f;
		followEntity = INVALID_ENTITY;
		active = false;
	}
} // namespace Framework
