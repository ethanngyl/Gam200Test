/**
===============================================================================
 File:           ParticleSystemManager.h
 Author:         TAN WEI LEONG
 Email:          weileong.tan@digipen.edu
 Date:           2026-03-12
 Contribution:   100%
 ------------------------------------------------------------------------------
===============================================================================
 */
#pragma once
#include "ParticleSystem.h"

namespace Framework {

    /**
     * @class ParticleSystemManager
     * @brief Owns and updates all active ParticleSystem emitters.
     *
     * Emitters are created via CreateEmitterRaw() which returns an integer emitter ID.
     * Use that ID with SetFollowEntity() to attach an emitter to a moving entity.
     * Call ClearAllEmitters() on level unload to destroy all particle entities.
     *
     * The emitter ID system:
     *   - IDs are sequential integers starting from 1
     *   - emitterIdToIndex maps IDs to indices in particleSystems vector
     *   - IDs reset to 1 after ClearAllEmitters() — do not hold stale IDs across level loads
     *
     * ALT+TAB handling:
     *   - Particles pause via GlobalPauseManager — Core.cpp gates Update() when paused
     *   - suspended flag provides an additional explicit guard in Update()
     *   - SendEngineMessage() takes no action; pause is handled upstream
     */
    class ParticleSystemManager : public EngineSystem {
    public:

        /**
         * @brief Constructs the manager with no emitters and nextEmitterId = 1.
         */
        ParticleSystemManager();

        /**
         * @brief Destructor. Calls ClearAllEmitters() to destroy all live particle
         *        entities before the manager is torn down.
         */
        virtual ~ParticleSystemManager();

        /**
         * @brief Creates an untracked ParticleSystem and returns a reference to it.
         *
         * The new emitter is appended to the internal particleSystems vector and is
         * not assigned an integer ID. Intended for internal/legacy use. Prefer
         * CreateEmitterRaw() for all Lua-driven emitters.
         *
         * @warning Do not store this reference across any call that may resize
         *          particleSystems (e.g. another AddParticleSystem() or
         *          CreateEmitterRaw() call) — the vector may reallocate and
         *          invalidate it.
         *
         * @return Reference to the newly created ParticleSystem.
         */
        ParticleSystem& AddParticleSystem();

        // -----------------------------------------------------------------------
        // Lua-friendly interface
        // -----------------------------------------------------------------------

        /**
         * @brief Creates a fully configured emitter and returns a stable integer ID.
         *
         * Appends a new ParticleSystem to particleSystems, applies the given
         * settings, positions the emitter at (x, y), and optionally wires up entity
         * tracking. The returned ID is the only safe way to reference this emitter
         * after construction — do not use vector indices directly.
         *
         * If followTarget is a valid EntityID the emitter will track that entity's
         * Transform position every frame via SetFollowEntity().
         *
         * @param s             Settings block to apply to the new emitter.
         * @param x             World X position of the emitter spawn point.
         * @param y             World Y position of the emitter spawn point.
         * @param followTarget  Optional EntityID to track. Pass INVALID_ENTITY (default)
         *                      for a stationary emitter.
         * @return              Integer emitter ID (>= 1). Store this to drive the
         *                      emitter later. Becomes invalid after ClearAllEmitters().
         */
        int CreateEmitterRaw(const ParticleSystem::Settings& s, float x, float y,
            EntityID followTarget = INVALID_ENTITY);

        /**
         * @brief Attaches an existing emitter to an ECS entity so it follows
         *        that entity's Transform position each frame.
         *
         * Looks up the emitter by ID in emitterIdToIndex. If the ID is not found
         * the call is silently ignored. Pass INVALID_ENTITY to detach tracking.
         *
         * @param emitterId  Integer ID returned by CreateEmitterRaw().
         * @param targetID   EntityID to follow, or INVALID_ENTITY to detach.
         */
        void SetFollowEntity(int emitterId, EntityID targetID);

        /**
         * @brief Destroys all live particle entities and removes every emitter.
         *
         * Calls Clear() on every ParticleSystem, then empties particleSystems,
         * emitterIdToIndex, and resets nextEmitterId to 1.
         *
         * Call this on every level unload. Any integer emitter IDs held by Lua
         * scripts become invalid after this call.
         */
        void ClearAllEmitters();

        // -----------------------------------------------------------------------
        // EngineSystem overrides
        // -----------------------------------------------------------------------

        /**
         * @brief Loads particle settings from assets/JSON/particles.json.
         *
         * Parses the JSON file and populates the internal settings map keyed by
         * emitter name. Fails silently if the file is not found — check console
         * output for load errors when emitters appear to produce no particles.
         */
        virtual void Initialize() override;

        /**
         * @brief Ticks every active emitter forward by dt seconds.
         *
         * Skips all updates if suspended is true (window has lost focus).
         * For each emitter in particleSystems, calls ParticleSystem::UpdateParticle(dt)
         * only when the emitter's ownerPlayerID matches the current active player
         * (or is -1 for always-on emitters).
         *
         * @param dt  Delta time in seconds since the last frame.
         */
        virtual void Update(float dt) override;

        /**
         * @brief Handles engine-level messages sent to this system.
         *
         * ALT+TAB / focus loss is handled upstream by GlobalPauseManager which
         * gates Core.cpp's Update() loop — this function intentionally takes no
         * action. Kept as a required override of EngineSystem.
         *
         * @param msg  Pointer to the incoming engine message. Not used.
         */
        virtual void SendEngineMessage(Message* msg) override;

    private:

        /**
         * Named particle settings loaded from particles.json at Initialize().
         * Keyed by emitter name string (e.g. "player_trail", "hit_spark").
         */
        std::unordered_map<std::string, ParticleSystem::Settings> settings;

        /** All currently active particle emitters owned by this manager. */
        std::vector<ParticleSystem> particleSystems;

        /**
         * Maps integer emitter IDs (returned by CreateEmitterRaw) to their
         * index in the particleSystems vector. Cleared by ClearAllEmitters().
         */
        std::unordered_map<int, size_t> emitterIdToIndex;

        /**
         * Counter used to assign the next emitter ID. Starts at 1 and
         * increments with each CreateEmitterRaw() call. Resets to 1 after
         * ClearAllEmitters().
         */
        int nextEmitterId{ 1 };

        /**
         * When true, Update() skips all particle updates.
         * Set to true when the application window loses focus (ALT+TAB or
         * minimize) and false when focus is restored.
         */
        bool suspended{ false };
    };

} // namespace Framework