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
	};
} // namespace Framework
