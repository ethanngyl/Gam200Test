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
#include "ResourceHandle.h"
#include <glm/glm.hpp>
#include <string>

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

        // Destructor to release resources
        ~Renderable();
    };

    // New name, same type. Can attach/get MeshRenderer in ECS code
    using MeshRenderer = Renderable;

    /**
     * @struct ParticleEmitter
     * @brief Component for particle effects
     * 
     * Future extension for particle systems
     */
    struct ParticleEmitter : public Component<ParticleEmitter> {
        MaterialHandle material;
        int maxParticles = 100;
        float emissionRate = 10.0f;
        glm::vec3 velocity = glm::vec3(0.0f, 1.0f, 0.0f);
        float lifetime = 2.0f;
        bool emit = true;
    };

} // namespace Framework
