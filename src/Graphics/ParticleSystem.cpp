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

    // Thread-local RNG for particle randomization
    static thread_local std::mt19937 s_rng{ std::random_device{}() };

    static float RandomFloat(float min, float max) {
        std::uniform_real_distribution<float> dist(min, max);
        return dist(s_rng);
    }

    static glm::vec2 RandomInCircle(float radius) {
        float angle = RandomFloat(0.0f, glm::two_pi<float>());
        float r = radius * std::sqrt(RandomFloat(0.0f, 1.0f));
        return { r * std::cos(angle), r * std::sin(angle) };
    }

    static glm::vec2 RandomInBox(const glm::vec2& halfExtents) {
        return { RandomFloat(-halfExtents.x, halfExtents.x),
                 RandomFloat(-halfExtents.y, halfExtents.y) };
    }

    // =========================================================================

    ParticleSystem::ParticleSystem() = default;
    ParticleSystem::~ParticleSystem() = default;

    void ParticleSystem::Initialize() {
        LOG_INFO("PARTICLE", "ParticleSystem initialized");
    }

    void ParticleSystem::SendEngineMessage(Message* /*message*/) {
        // No messages handled yet
    }

    // =========================================================================
    // Main update
    // =========================================================================

    void ParticleSystem::Update(float dt) {
        if (!entityManager) return;

        // Collect entities to destroy after iteration
        std::vector<Entity> toDestroy;

        for (Entity e : entityManager->GetAllEntities()) {
            if (!entityManager->HasComponent<ParticleEmitter>(e)) continue;
            if (!entityManager->HasComponent<Transform>(e)) continue;

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
                }
            }

            // Lazy-initialize particle pool
            if (emitter.particles.empty() && emitter.maxParticles > 0) {
                emitter.particles.resize(static_cast<size_t>(emitter.maxParticles));
            }

            // Handle duration
            if (emitter.duration > 0.0f) {
                emitter.elapsed += dt;
                if (emitter.elapsed >= emitter.duration) {
                    emitter.emit = false;
                }
            }

            // Spawn new particles
            SpawnParticles(emitter, transform, dt);

            // Update existing particles
            UpdateParticles(emitter, dt);

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

        for (Entity e : toDestroy) {
            entityManager->DestroyEntity(e);
        }
    }

    // =========================================================================
    // Spawn
    // =========================================================================

    void ParticleSystem::SpawnParticles(ParticleEmitter& emitter, const Transform& transform, float dt) {
        if (!emitter.emit) return;

        int toSpawn = 0;

        // Burst mode
        if (emitter.burstCount > 0 && !emitter.burstFired) {
            toSpawn = emitter.burstCount;
            emitter.burstFired = true;
            emitter.emit = false; // stop continuous emission after burst
        }
        else if (emitter.burstCount == 0) {
            // Continuous emission
            emitter.emitAccumulator += emitter.emissionRate * dt;
            toSpawn = static_cast<int>(emitter.emitAccumulator);
            emitter.emitAccumulator -= static_cast<float>(toSpawn);
        }

        for (int i = 0; i < toSpawn; ++i) {
            // Find a dead particle slot
            Particle* slot = nullptr;
            for (auto& p : emitter.particles) {
                if (!p.alive) {
                    slot = &p;
                    break;
                }
            }
            if (!slot) break; // pool full

            // Initialize particle
            slot->alive = true;
            slot->lifetime = 0.0f;
            slot->maxLifetime = RandomFloat(emitter.lifetimeMin, emitter.lifetimeMax);
            slot->size = emitter.sizeStart;
            slot->color = emitter.colorStart;
            slot->rotation = RandomFloat(0.0f, glm::two_pi<float>());
            slot->rotationSpeed = RandomFloat(emitter.rotationSpeedMin, emitter.rotationSpeedMax);

            // Velocity
            slot->velocity.x = RandomFloat(emitter.velocityMin.x, emitter.velocityMax.x);
            slot->velocity.y = RandomFloat(emitter.velocityMin.y, emitter.velocityMax.y);

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

            // World-space particles get absolute position; local-space particles stay relative to emitter
            if (emitter.worldSpace) {
                slot->position = glm::vec2(transform.position.x, transform.position.y) + offset;
            } else {
                slot->position = offset;
            }
        }
    }

    // =========================================================================
    // Update particles
    // =========================================================================

    void ParticleSystem::UpdateParticles(ParticleEmitter& emitter, float dt) {
        for (auto& p : emitter.particles) {
            if (!p.alive) continue;

            p.lifetime += dt;

            // Kill expired particles
            if (p.lifetime >= p.maxLifetime) {
                p.alive = false;
                continue;
            }

            // Normalized age [0, 1]
            float t = p.lifetime / p.maxLifetime;

            // Apply gravity
            p.velocity += emitter.gravity * dt;

            // Integrate position
            p.position += p.velocity * dt;

            // Interpolate size
            p.size = glm::mix(emitter.sizeStart, emitter.sizeEnd, t);

            // Interpolate color
            p.color = glm::mix(emitter.colorStart, emitter.colorEnd, t);

            // Rotate
            p.rotation += p.rotationSpeed * dt;
        }
    }

} // namespace Framework
