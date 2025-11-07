 /**
===============================================================================
 File:           RenderComponents.h
 Author:         Graphics System Overhaul
 Date:           2025-10-07
 ------------------------------------------------------------------------------
 Brief:
 Enhanced rendering components for the ECS system.
 Provides flexible rendering capabilities with material support.

 Design notes:
 - Renderable component replaces simple Sprite
 - Supports materials for advanced rendering
 - Layer-based sorting
 - Per-instance properties
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
    };

    // New name, same type. Can attach/get MeshRenderer in ECS code
    using MeshRenderer = Renderable;

    /**
    ===============================================================================
     Author:         Tan Wei Leong
     Email:          weileong.tan@digipen.edu
     Date:           2025-11-06
     Contribution:   100% (SpriteAnimation Component)
     ------------------------------------------------------------------------------

     @struct SpriteAnimation
     @brief ECS component representing a sprite sheet–based animation.

     The SpriteAnimation component defines all parameters required for
     frame-based 2D animations using a sprite sheet (texture atlas). It stores
     information about the frame layout, timing, looping, and playback state.

     This component is updated every frame by the AnimationSystem, which uses
     delta time to progress through frames, apply looping logic, and update
     frame indices. The GraphicsSystemV2 later uses these values to compute
     the UV coordinates for rendering the correct animation frame.

     Responsibilities:
     - Tracks animation timing, looping, and playback state
     - Stores sprite sheet layout (rows × columns)
     - Defines frame dimensions for UV slicing
     - Supports horizontal flipping and UV shrink adjustments

     Typical Usage:
     @code
     auto& anim = entityManager->AddComponent<SpriteAnimation>(entity);
     anim.spriteSheet = gfx->GetResourceManager().LoadTexture("assets/player_walk.png");
     anim.columns = 4;
     anim.rows = 2;
     anim.frameCount = 8;
     anim.frameTime = 0.1f;
     anim.loop = true;
     anim.playing = true;
     @endcode

     @see AnimationSystem
     @see GraphicsSystemV2
    ===============================================================================
     */
    struct SpriteAnimation : public Component<SpriteAnimation>
    {
        /** Handle to the texture containing the full sprite sheet. */
        TextureHandle spriteSheet;

        /** Index of the currently displayed frame (0-based). */
        int currentFrame = 0;

        /** Number of frames used by this animation (may be less than total cells). */
        int frameCount = 1;

        /** Total number of frames in the sprite sheet (rows × columns). */
        int totalFrames = 0;

        /** Number of horizontal cells (columns) in the sprite sheet. */
        int columns = 0;

        /** Number of vertical cells (rows) in the sprite sheet. */
        int rows = 0;

        /** Duration (in seconds) each frame is displayed before advancing. */
        float frameTime = 0.0f;

        /** Accumulated time elapsed since the current frame began. */
        float elapsedTime = 0.0f;

        /**
         * UV shrink value (in pixels) to reduce edge bleeding between frames.
         * Typically a small fraction of a pixel (e.g., 0.5–1.0).
         */
        float uvShrinkPx = 0.0f;

        /** Whether the animation should loop back to the first frame after finishing. */
        bool loop = true;

        /** Whether the animation is currently playing (true) or paused (false). */
        bool playing = true;

        /** Whether to horizontally flip the animation (useful for facing direction). */
        bool flipX = false;

        /** Width (in pixels) of a single frame within the sprite sheet. */
        int frameWidth = 0;

        /** Height (in pixels) of a single frame within the sprite sheet. */
        int frameHeight = 0;
    };

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
