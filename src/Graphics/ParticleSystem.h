/**
===============================================================================
 File:           ParticleSystem.h
 Author:         TAN WEI LEONG
 Email:          weileong.tan@digipen.edu
 Date:           2026-03-12
 Contribution:   100%
 ------------------------------------------------------------------------------
===============================================================================
 */
#pragma once
#include "Vector2D.h"
#include "ECSEntity.h"
#include <string>
#include <vector>
#include <glm/glm.hpp>

namespace Framework {

    /**
     * @class ParticleSystem
     * @brief Manages a single particle emitter — spawning, updating, and destroying
     *        particle entities each frame.
     *
     * Each ParticleSystem owns a list of ECS particle entities. Call SetSettings()
     * before use (it caches world-scale and pre-reserves the particle vector).
     * Call UpdateParticle(dt) every frame. Call Clear() before destroying.
     *
     * The followEntity field makes the emitter track another entity's Transform
     * position each frame — used to attach particles to moving players/enemies.
     *
     * ownerPlayerID controls turn-based visibility:
     *   -1 = always active
     *    0, 1, 2 = only active on that player's turn
     */
    class ParticleSystem {
    public:

        /**
         * @struct Settings
         * @brief All tunable parameters for a particle emitter.
         *
         * Passed to SetSettings() before the emitter is used. cachedWorldScale
         * is computed once inside SetSettings() from the size field and the
         * current render-window height, so individual particles never need to
         * query the graphics system.
         */
        struct Settings {
            /** RGBA tint applied to every particle at spawn time. */
            glm::vec4   tint{ 0.0f, 0.0f, 0.0f, 1.0f };

            /** Render layer the particle entities are placed on. */
            int         layer{ 0 };

            /** Number of particles spawned per second (continuous mode). */
            float       spawnRate{ 0.0f };

            /** Maximum particle lifetime in seconds. */
            float       maxLifetime{ 0.0f };

            /** Minimum particle lifetime in seconds. */
            float       minLifetime{ 0.0f };

            /** Maximum initial speed of a spawned particle (world units/sec). */
            float       maxSpeed{ 0.0f };

            /** Minimum initial speed of a spawned particle (world units/sec). */
            float       minSpeed{ 0.0f };

            /** Visual diameter of each particle in pixels. */
            float       size{ 0.0f };

            /**
             * Burst count. 0 = continuous emission driven by spawnRate.
             * Any value > 0 = spawn exactly this many particles once at emitter
             * start, then stop.
             */
            int         burstCnt{ 0 };

            /**
             * Preferred emission direction in world space.
             * (0, 0) = radial / fully random direction.
             * Normalise before passing in for directional emitters (e.g. smoke
             * rising upward would use (0, 1)).
             */
            Vector2D    direction{ 0.0f, 0.0f };

            /**
             * Spread factor applied to the emission direction.
             * 0.0 = all particles travel exactly along `direction`.
             * 1.0 = particles spread into a full cone / hemisphere.
             */
            float       directionFuzz{ 0.0f };

            /**
             * Maximum random radial offset applied to the spawn position.
             * Particles spawn at a random point within this radius of the
             * emitter's world position.
             */
            float       spawnRadius{ 0.0f };

            /**
             * RGBA tint the particle fades toward over its lifetime.
             * Default alpha = 0 so particles fade out naturally.
             */
            glm::vec4   endTint{ 1.0f, 1.0f, 1.0f, 0.0f };

            /** Visual diameter (pixels) the particle shrinks/grows toward over its lifetime. */
            float       endSize{ 0.0f };

            /** Per-frame gravity vector added to each particle's velocity (world units/sec²). */
            Vector2D    gravity{ 0.0f, 0.0f };

            /** If true, particle alpha is linearly interpolated toward endTint.a over its lifetime. */
            bool        fadeOut{ true };

            /** If true, particle size shrinks linearly toward endSize over its lifetime. */
            bool        shrinkOverTime{ false };

            /** If true, particle size grows linearly toward endSize over its lifetime. */
            bool        growOverTime{ false };

            /**
             * Turn-based ownership filter.
             * -1  = emitter is always active regardless of whose turn it is.
             *  0  = only active during player 0's turn.
             *  1  = only active during player 1's turn.
             *  2  = only active during player 2's turn.
             */
            int         ownerPlayerID{ -1 };

            /**
             * World-scale diameter pre-computed from `size` and the render-window
             * height. Set automatically by SetSettings() — do not set manually.
             * Avoids a per-particle graphics query every frame.
             */
            float       cachedWorldScale{ 0.0f };
        };

        // -----------------------------------------------------------------------
        // Core lifetime methods
        // -----------------------------------------------------------------------

        /**
         * @brief Spawns a single new particle entity at the current emitter position.
         *
         * Randomises lifetime, speed, and direction within the ranges defined in
         * Settings. The new entity is appended to the internal particles vector.
         * Called automatically by UpdateParticle() in continuous mode, or in bulk
         * by SpawnBurst().
         */
        void CreateParticle();

        /**
         * @brief Advances all live particles by one frame and removes dead ones.
         *
         * Each call:
         *  1. If followEntity is valid, snaps the emitter to that entity's
         *     current Transform position.
         *  2. Spawns new particles according to spawnRate and spawnAcc (continuous
         *     mode only — burst emitters do not spawn here).
         *  3. Ticks every live particle: applies velocity, gravity, fade, and
         *     size change.
         *  4. Destroys and removes any particle whose lifetime has expired.
         *
         * @param dt  Delta time in seconds since the last frame.
         */
        void UpdateParticle(float dt);

        /**
         * @brief Destroys all live particle entities and clears the internal list.
         *
         * Must be called before the ParticleSystem itself is destroyed to avoid
         * leaving orphaned ECS entities in the world.
         */
        void Clear();

        // -----------------------------------------------------------------------
        // Configuration
        // -----------------------------------------------------------------------

        /**
         * @brief Applies a new Settings block to this emitter.
         *
         * Caches the world-scale value (settings.cachedWorldScale) by querying
         * the current render-window height once, so per-particle updates never
         * need to touch the graphics system.
         *
         * @param s  The settings to apply. Copied by value into the emitter.
         */
        void SetSettings(const Settings& s);

        /**
         * @brief Returns a const reference to the current settings block.
         * @return Const reference to the internal Settings struct.
         */
        const Settings& GetSettings() const { return settings; }

        // -----------------------------------------------------------------------
        // Position and follow
        // -----------------------------------------------------------------------

        /**
         * @brief Sets the world-space position from which particles are spawned.
         * @param x  World X coordinate.
         * @param y  World Y coordinate.
         */
        void SetEmitter(float x, float y) { emitter = { x, y }; }

        /**
         * @brief Makes the emitter track an ECS entity's Transform every frame.
         *
         * When set to a valid EntityID, UpdateParticle() will read that entity's
         * Transform position at the start of each frame and move the emitter
         * there automatically. Pass INVALID_ENTITY to detach.
         *
         * @param id  EntityID to follow, or INVALID_ENTITY to disable tracking.
         */
        void SetFollowEntity(EntityID id) { followEntity = id; }

        /**
         * @brief Returns the current world-space emitter position.
         * @return Vector2D containing the emitter's X and Y coordinates.
         */
        Vector2D GetEmitter() const { return emitter; }

        // -----------------------------------------------------------------------
        // Active state
        // -----------------------------------------------------------------------

        /**
         * @brief Returns whether this emitter is currently active.
         *
         * Inactive emitters do not spawn new particles, but existing live
         * particles continue to update and expire normally.
         *
         * @return true if active, false if paused/disabled.
         */
        bool IsActive() const { return active; }

        /**
         * @brief Enables or disables particle spawning for this emitter.
         * @param a  true to activate, false to pause spawning.
         */
        void SetActive(bool a) { active = a; }

        // -----------------------------------------------------------------------
        // Utilities
        // -----------------------------------------------------------------------

        /**
         * @brief Resets the internal spawn-time accumulator to zero.
         *
         * Call this after a long pause or when re-activating an emitter to
         * prevent a sudden burst of catch-up spawns on the next UpdateParticle()
         * call.
         */
        void ResetSpawnAccumulator() { spawnAcc = 0.0f; }

        /**
         * @brief Returns the number of currently live particle entities.
         * @return Count of particles in the internal list.
         */
        size_t GetParticle() const { return particles.size(); }

        /**
         * @brief Immediately spawns a fixed number of particles at the emitter.
         *
         * Useful for one-shot effects such as explosions or hit sparks.
         * Calls CreateParticle() `count` times in a single frame.
         *
         * @param count  Number of particles to spawn immediately.
         */
        void SpawnBurst(int count) { for (int i{ 0 }; i < count; ++i) CreateParticle(); }

        // -----------------------------------------------------------------------
        // Turn-based ownership
        // -----------------------------------------------------------------------

        /**
         * @brief Sets which player's turn this emitter should be active on.
         *
         * Mirrors Settings::ownerPlayerID. Pass -1 to keep the emitter always
         * active regardless of turn state.
         *
         * @param playerID  Player index (0–2) or -1 for always-on.
         */
        void SetOwnerPlayer(int playerID) { settings.ownerPlayerID = playerID; }

        /**
         * @brief Returns the ownerPlayerID for this emitter.
         * @return Player index (0–2), or -1 if always active.
         */
        int GetOwnerPlayer() const { return settings.ownerPlayerID; }

    private:
        /** ECS entities representing currently live particles. */
        std::vector<Entity> particles;

        /** World-space position from which new particles are spawned. */
        Vector2D emitter{ 0.0f, 0.0f };

        /**
         * Entity this emitter tracks each frame.
         * INVALID_ENTITY means no tracking — emitter stays at its last set position.
         */
        EntityID followEntity = INVALID_ENTITY;

        /**
         * Fractional-particle accumulator for continuous emission.
         * Incremented by spawnRate * dt each frame; a particle is spawned and
         * 1.0 is subtracted each time it reaches or exceeds 1.0.
         */
        float spawnAcc = 0.0f;

        /** Whether this emitter is currently spawning new particles. */
        bool active = true;

        /** Current settings block. Modified only through SetSettings(). */
        Settings settings;
    };

} // namespace Framework