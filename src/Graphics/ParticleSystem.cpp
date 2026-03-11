/*
===============================================================================
File:        ParticleSystem.cpp
-------------------------------------------------------------------------------
Brief:
Implementation of ParticleSystem. Handles particle spawning, physics
integration, color/size interpolation over lifetime, and cleanup.
===============================================================================
*/
#include "Precompiled.h"
#include "ParticleSystem.h"
#include "ECSEntityManager.h"
#include "Component.h"
#include "RenderComponents.h"
#include <random>

namespace Framework {
	void ParticleSystem::CreateParticle() {
		if (!active) return;
		if (!CORE || !CORE->GetGraphicsSystem() || !CORE->GetEntityManager()) return; // safety check

		// Query active graphics system to convert pixel size into world-space scale
		// Size (pixel -> world conversion)
		auto* graphics = CORE->GetGraphicsSystem();
		float worldScale = settings.size * 0.1f;  // size is now in tile units (1.0 = one tile)
    // Thread-local RNG for particle randomization
    //thread_local prevents crashes in the event that multiple threads accesses the same rng state
    //mt19937 prevents patterns from forming in the randomness
    //random_device ensures every starting seed of the particle is random
    static thread_local std::mt19937 s_rng{ std::random_device{}() };

		Entity entity = CORE->GetEntityManager()->CreateEntity();

		CORE->GetEntityManager()->AddComponent<Transform>(entity, emitter);
		auto& transform = CORE->GetEntityManager()->GetComponent<Transform>(entity);
		transform.position = emitter;
		transform.scale = { worldScale, worldScale };	// Size
    //Gets a random value between min and max, ensures all values has an equal chance to get selected
    static float RandomFloat(float min, float max) {
        std::uniform_real_distribution<float> dist(min, max);
        return dist(s_rng);
    }

		CORE->GetEntityManager()->AddComponent<Particle>(entity);
		auto& particle = CORE->GetEntityManager()->GetComponent<Particle>(entity);
    //Generates a random 2D coordinate in a circle, sqrt is used to ensure even distribution so that the particles do not clump together
    static glm::vec2 RandomInCircle(float radius) {
        float angle = RandomFloat(0.0f, glm::two_pi<float>());
        float r = radius * std::sqrt(RandomFloat(0.0f, 1.0f));
        return { r * std::cos(angle), r * std::sin(angle) };
    }

    static glm::vec2 RandomInBox(const glm::vec2& halfExtents) {
        return { RandomFloat(-halfExtents.x, halfExtents.x),
                 RandomFloat(-halfExtents.y, halfExtents.y) };
    }
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
    // =========================================================================

		// Mesh (Body for the particle)
		CORE->GetEntityManager()->AddComponent<MeshRenderer>(entity);
		auto& mr = CORE->GetEntityManager()->GetComponent<MeshRenderer>(entity);
		mr.layer = settings.layer;
		mr.tint = settings.tint;
    ParticleSystem::ParticleSystem() = default;
    ParticleSystem::~ParticleSystem() = default;

		// Lifetime
		particle.lifetime = settings.minLifetime + frand() * (settings.maxLifetime - settings.minLifetime);
		particle.maxLifetime = particle.lifetime;  // Store max lifetime for interpolation
		particle.age = 0.0f;
    void ParticleSystem::Initialize() {
        LOG_INFO("PARTICLE", "ParticleSystem initialized");
    }

    void ParticleSystem::SendEngineMessage(Message* /*message*/) {
        // No messages handled yet
    }
		// Velocity: Radial or Directional cone
		float speed = settings.minSpeed + frand() * (settings.maxSpeed - settings.minSpeed);

    // =========================================================================
    // Main update
    // =========================================================================
		Vector2D direction { 0.0f, 0.0f };

		if (settings.direction.x == 0.0f && settings.direction.y == 0.0f) {
			// Radial Emission
			// scale up by 2 then shift by -1 (Range: -1 to 1)
			direction.x = (frand() * 2.0f) - 1.0f;
			direction.y = (frand() * 2.0f) - 1.0f;
    //dt is used here as the time passed since the last frame
    void ParticleSystem::Update(float dt) {
        if (!entityManager) return;

        // Collect entities to destroy after iteration
        std::vector<Entity> toDestroy;
			// direction normalized
			direction.normalize();
		}
		else {
			// Directional with fuzz
			direction = settings.direction;

			// scale up by 2 then shift by -1 (Range: -1 to 1)
			//direction.x += ((frand() * 2.0f) - 1.0f) * settings.directionFuzz;
			//direction.y += ((frand() * 2.0f) - 1.0f) * settings.directionFuzz;
        //Loops through every entity to check for the particle emitter component
        for (Entity e : entityManager->GetAllEntities()) {
            if (!entityManager->HasComponent<ParticleEmitter>(e)) continue;
            if (!entityManager->HasComponent<Transform>(e)) continue;

			direction.y = std::abs(direction.y); // push mostly upward for smoke
			direction.x += ((frand() * 2.0f) - 1.0f) * 0.2f; // a bit more sideways
            //Grabbing entity data
            auto& emitter = entityManager->GetComponent<ParticleEmitter>(e);
            auto& transform = entityManager->GetComponent<Transform>(e);

            // Follow target entity: update emitter position to match target
            if (emitter.followEntity != INVALID_ENTITY) {
                Entity target{ emitter.followEntity };
                if (entityManager->HasComponent<Transform>(target)) {
                    auto& targetTransform = entityManager->GetComponent<Transform>(target);
                    transform.position = targetTransform.position;
                } else {
                    // Target entity no longer exists — stop emitting and auto-destroy
                    emitter.emit = false;
                    emitter.autoDestroy = true;
			// direction normalized
			direction.normalize();
                }
            }

		particle.velocity = { direction.x * speed, direction.y * speed };
            // Lazy-initialization particle pool
            // Memory is not allocated for particles until the first time it is needed
            if (emitter.particles.empty() && emitter.maxParticles > 0) {
                emitter.particles.resize(static_cast<size_t>(emitter.maxParticles));
            }

            // Handles particle duration
            if (emitter.duration > 0.0f) {
                emitter.elapsed += dt;
                if (emitter.elapsed >= emitter.duration) {
                    emitter.emit = false;
		particles.push_back(entity);
                }
            }

	void ParticleSystem::Update(float dt) {
		if (dt <= 0.0f) return; // saftey check
            // Spawn new particles
            SpawnParticles(emitter, transform, dt);

            // Update existing particles
            UpdateParticles(emitter, dt);
		if (!active) { spawnAcc = 0.0f; return; }

		EntityManager* entityManager = CORE->GetEntityManager();
		if (!entityManager) return;
            // Auto-destroy: if emitter stopped and all particles are dead
            if (emitter.autoDestroy && !emitter.emit) {
                bool anyAlive = false;
                for (const auto& p : emitter.particles) {
                    if (p.alive) { anyAlive = true; break; }
                }
                if (!anyAlive) {
                    toDestroy.push_back(e);
                }
            }
        }

		// spawn new particles over time
		if (settings.spawnRate > 0.0f) {
			spawnAcc += dt * settings.spawnRate; // how many particles we "owe"
			while (spawnAcc >= 1.0f) {
				CreateParticle();
				spawnAcc -= 1.0f;
        for (Entity e : toDestroy) {
            entityManager->DestroyEntity(e);
        }
    }

    // =========================================================================
    // Spawn
    // =========================================================================

    void ParticleSystem::SpawnParticles(ParticleEmitter& emitter, const Transform& transform, float dt) {
        if (!emitter.emit) return;

        //Calculated based on burst/continuous setting
        int toSpawn = 0;
		// update existing particles
		for (auto count = particles.begin(); count != particles.end(); ) {
			Entity entity = *count;

        // Burst mode
        if (emitter.burstCount > 0 && !emitter.burstFired) {
            toSpawn = emitter.burstCount;
            emitter.burstFired = true;
            emitter.emit = false; // stop continuous emission after burst
        }
        else if (emitter.burstCount == 0) {
            // Continuous emission
            emitter.emitAccumulator += emitter.emissionRate * dt; //In the event that the frame rate is faster than the emission rate
            toSpawn = static_cast<int>(emitter.emitAccumulator);
            emitter.emitAccumulator -= static_cast<float>(toSpawn);
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
        // This looks for an existing particle in the list that is currently 
        // inacitve/not alive and reactivates it to reduce memory fragmentation
        for (int i = 0; i < toSpawn; ++i) {
            // Find a dead particle slot
            Particle* slot = nullptr;
            for (auto& p : emitter.particles) {
                if (!p.alive) {
                    slot = &p;
                    break;
                }
            }

			// Update position
			transform.position.x += particle.velocity.x * dt;
			transform.position.y += particle.velocity.y * dt;
            if (!slot) break; // pool full

            // Initialize particle
            slot->alive = true;
            slot->lifetime = 0.0f;
            slot->maxLifetime = RandomFloat(emitter.lifetimeMin, emitter.lifetimeMax);
            slot->size = emitter.sizeStart;
            slot->color = emitter.colorStart;
            slot->rotation = RandomFloat(0.0f, glm::two_pi<float>());
            slot->rotationSpeed = RandomFloat(emitter.rotationSpeedMin, emitter.rotationSpeedMax);
			// Update age
			particle.age += dt;

            // Velocity
            slot->velocity.x = RandomFloat(emitter.velocityMin.x, emitter.velocityMax.x);
            slot->velocity.y = RandomFloat(emitter.velocityMin.y, emitter.velocityMax.y);
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
            // Position based on emit shape
            glm::vec2 offset{ 0.0f };
            switch (emitter.emitShape) {
            case ParticleEmitShape::Point:
                break;
            case ParticleEmitShape::Circle:
                offset = RandomInCircle(emitter.emitRadius);
                break;
            case ParticleEmitShape::Box:
                offset = RandomInBox(emitter.emitSize);
                break;
            }

            // World-space makes the particle stay in the position it was created even if the emitter moves
            if (emitter.worldSpace) {
                slot->position = glm::vec2(transform.position.x, transform.position.y) + offset;
            } 
            // Local-space makes the particle move with its emitter
            else {
                slot->position = offset;
            }
				if (particle.shrinkOverTime || particle.growOverTime)
				{
					float lifeProgress = particle.age / particle.maxLifetime;
					float currentSize = particle.startSize +
						(particle.endSize - particle.startSize) * lifeProgress;
					transform.scale = { currentSize, currentSize };
        }
    }

    // =========================================================================
    // Update particles
    // =========================================================================

    void ParticleSystem::UpdateParticles(ParticleEmitter& emitter, float dt) {
        for (auto& p : emitter.particles) {
            if (!p.alive) continue;

            p.lifetime += dt;

            // Kill expired particles if they have exceeded their lifetime
            if (p.lifetime >= p.maxLifetime) {
                p.alive = false;
                continue;
			// Check lifetime
			if (particle.age >= particle.maxLifetime) {
				entityManager->DestroyEntity(entity);
				count = particles.erase(count);
			}
			else {
				++count;
            }

            // Normalized particle lifetime
            float t = p.lifetime / p.maxLifetime;

            // Apply gravity
            p.velocity += emitter.gravity * dt;

            // Integrate position
            p.position += p.velocity * dt;

            // Interpolate size
            p.size = glm::mix(emitter.sizeStart, emitter.sizeEnd, t);

            // Interpolate color
            p.color = glm::mix(emitter.colorStart, emitter.colorEnd, t);

            // Particle Rotation Speed
            p.rotation += p.rotationSpeed * dt;
        }
    }

} // namespace Framework
