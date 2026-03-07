/*
===============================================================================
File:        RenderComponents.h
Author:      Sim Kah Yan
Email:       kahyan.sim@digipen.edu
Date:        2025-11-07
Contribution: 100%
-------------------------------------------------------------------------------
Brief:
ECS components related to rendering. Includes Renderable (MeshRenderer)
and ParticleEmitter.

- Renderable: core rendering data (mesh/material/texture/tint/layer)
- ParticleEmitter: basic parameters for particle systems (future use)

NOTE: SpriteAnimation has been moved to ECS/Component.h to support
      enhanced animation features with directional animations.

Used by:
- GraphicsSystemV2 (drawing)
===============================================================================
*/
#pragma once
#include "ECSComponent.h"
#include "ECSEntity.h"
#include "ResourceHandle.h"
#include "Material.h"
#include <glm/glm.hpp>
#include <string>
#include <vector>

namespace Framework {

    /**
     * @struct Renderable
     * @brief Enhanced rendering component with material support
     * 
     * Replaces the old Sprite component with a more flexible system
     * that supports materials, meshes, and advanced rendering properties.
     */
    struct Renderable : public Component<Renderable> {
        // Resource references
        MeshHandle mesh;
        MaterialHandle material;
        TextureHandle  texture;

        std::string    spriteName;// consider renaming to `texturePath` later

        // Rendering properties
        int layer = 0;            // Sorting layer (lower = drawn first)
        int orderInLayer = 0;     // Order within layer
        bool visible = true;       // Is this renderable visible?
        bool castShadows = false;  // Cast shadows (future)
        bool receiveShadows = false; // Receive shadows (future)

        // Per-instance overrides
        glm::vec4 tint = glm::vec4(1.0f);  // Color tint multiplier

        Renderable() = default;

        // Constructor for quick setup
        Renderable(MeshHandle m, MaterialHandle mat, int lyr = 0)
            : mesh(m), material(mat), layer(lyr) {}

        // Legacy constructor using sprite names
        explicit Renderable(const std::string& name)
            : spriteName(name) {}
    };

    // New name, same type. Can attach/get MeshRenderer in ECS code
    using MeshRenderer = Renderable;

    /**
     * @struct Particle
     * @brief Individual particle data (stored in flat array inside emitter)
     */
    struct Particle {
        glm::vec2 position{ 0.0f };
        glm::vec2 velocity{ 0.0f };
        glm::vec4 color{ 1.0f };
        float size = 1.0f;
        float rotation = 0.0f;
        float rotationSpeed = 0.0f;
        float lifetime = 0.0f;
        float maxLifetime = 1.0f;
        bool alive = false;
    };

    /**
     * @enum ParticleEmitShape
     * @brief Shape from which particles are spawned
     */
    enum class ParticleEmitShape {
        Point,
        Circle,
        Box
    };

    /**
     * @struct ParticleEmitter
     * @brief Component for particle effects with built-in particle pool
     *
     * Stores all particles in a flat array for cache-friendly iteration.
     * ParticleSystem updates particles; GraphicsSystemV2 renders them.
     */
    struct ParticleEmitter : public Component<ParticleEmitter> {
        // --- Emitter config ---
        int maxParticles = 100;
        float emissionRate = 10.0f;       // particles per second
        bool emit = true;                 // actively spawning?
        bool worldSpace = true;           // particles in world or local space?
        int layer = 10;                   // render layer (default: Effects)

        // --- Spawn shape ---
        ParticleEmitShape emitShape = ParticleEmitShape::Point;
        float emitRadius = 0.0f;          // for Circle shape
        glm::vec2 emitSize{ 0.0f };       // for Box shape (half-extents)

        // --- Per-particle initial values (with random ranges) ---
        glm::vec2 velocityMin{ -0.5f, 0.5f };
        glm::vec2 velocityMax{ 0.5f, 1.5f };
        float lifetimeMin = 0.5f;
        float lifetimeMax = 2.0f;
        float sizeStart = 0.1f;
        float sizeEnd = 0.0f;
        float rotationSpeedMin = 0.0f;
        float rotationSpeedMax = 0.0f;

        // --- Color over lifetime ---
        glm::vec4 colorStart{ 1.0f, 1.0f, 1.0f, 1.0f };
        glm::vec4 colorEnd{ 1.0f, 1.0f, 1.0f, 0.0f };

        // --- Gravity / forces ---
        glm::vec2 gravity{ 0.0f, 0.0f };

        // --- Material / texture ---
        MaterialHandle material;
        TextureHandle texture;
        BlendMode blendMode = BlendMode::Additive;

        // --- Runtime state ---
        std::vector<Particle> particles;
        float emitAccumulator = 0.0f;     // fractional particle accumulation

        // --- Burst mode ---
        int burstCount = 0;               // if > 0, emit this many immediately then stop
        bool burstFired = false;

        // --- Follow target (emitter follows another entity's position) ---
        EntityID followEntity = INVALID_ENTITY;  // entity to follow, 0 = none

        // --- Lifetime (optional: auto-destroy emitter entity after duration) ---
        float duration = 0.0f;            // 0 = infinite
        float elapsed = 0.0f;
        bool autoDestroy = false;         // destroy entity when duration expires and all particles dead
    };

} // namespace Framework
