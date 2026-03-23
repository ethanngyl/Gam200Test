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
		ParticleSystemManager();
		virtual ~ParticleSystemManager();

		ParticleSystem& AddParticleSystem();

		// Lua-friendly interface
		int CreateEmitterRaw(const ParticleSystem::Settings& s, float x, float y, EntityID followTarget = INVALID_ENTITY);
		void SetFollowEntity(int emitterId, EntityID targetID);
		void ClearAllEmitters();

		// Mandatory overrides
		virtual void Initialize() override;
		virtual void Update(float dt) override;
		virtual void SendEngineMessage(Message* msg) override;

	private:
		// Internal data (emitters, particle systems)
		std::unordered_map<std::string, ParticleSystem::Settings> settings;
		std::vector<ParticleSystem> particleSystems;
		std::unordered_map<int, size_t> emitterIdToIndex; // Map IDs to indices
		int nextEmitterId{ 1 };
		bool suspended{ false };  // true while window has no focus (ALT+TAB / minimize)
	};
} // namespace Framework
