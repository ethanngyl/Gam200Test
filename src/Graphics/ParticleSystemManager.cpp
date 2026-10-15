/**
===============================================================================
 File:           ParticleSystemManager.cpp
 Author:         TAN WEI LEONG
 Email:          weileong.tan@digipen.edu
 Date:           2026-03-12
 Contribution:   100%
 ------------------------------------------------------------------------------

===============================================================================
 */

#include "Precompiled.h"

namespace Framework {
	ParticleSystemManager::ParticleSystemManager() {}

	ParticleSystemManager::~ParticleSystemManager() {}

	ParticleSystem& ParticleSystemManager::AddParticleSystem() {
		particleSystems.emplace_back();   // create a new ParticleSystem
		return particleSystems.back();    // return reference so caller can configure it
	}

	int ParticleSystemManager::CreateEmitterRaw(const ParticleSystem::Settings& s, float x, float y, EntityID followTarget) {
		// Reserve before emplace_back so the vector doesn't reallocate and
		// invalidate any live references into particleSystems
		particleSystems.reserve(particleSystems.size() + 1);
		particleSystems.emplace_back();

		// Use index not reference — a reference would dangle after future push_backs
		size_t idx = particleSystems.size() - 1;
		particleSystems[idx].SetSettings(s);
		particleSystems[idx].SetEmitter(x, y);
		if (followTarget != INVALID_ENTITY) particleSystems[idx].SetFollowEntity(followTarget);
		if (s.burstCnt > 0) particleSystems[idx].SpawnBurst(s.burstCnt);

		int emitterId = nextEmitterId++;
		emitterIdToIndex[emitterId] = idx;
		return emitterId;
	}

	// Lua-friendly functions
	void ParticleSystemManager::SetFollowEntity(int emitterId, EntityID targetID) {
		auto it = emitterIdToIndex.find(emitterId);
		if (it == emitterIdToIndex.end()) return;
		particleSystems[it->second].SetFollowEntity(targetID);
	}

	void ParticleSystemManager::ClearAllEmitters() {
		for (auto& particleSystem : particleSystems) {
			particleSystem.Clear();
		}

		particleSystems.clear();
		emitterIdToIndex.clear();
		// nextEmitterId intentionally NOT reset — keeps IDs unique across level loads
		// to prevent stale Lua emitter IDs from silently hitting new emitters
		// nextEmitterId = 1;
	}

	void ParticleSystemManager::Initialize() {}

	void ParticleSystemManager::Update(float dt) {
		// Do not update particles while the window is suspended
		if (suspended) return;

		// Loop through all emitters | Update all particle systems
		for (auto& particleSystem : particleSystems) {
			particleSystem.UpdateParticle(dt);
		}
	}

	void ParticleSystemManager::SendEngineMessage(Message* msg) {
		// ALT+TAB pause is handled by GlobalPauseManager — Update() is gated
		// by Core.cpp before this system is ticked, so no action needed here.
		(void)msg;  // suppress unused parameter warning
	}
} // namespace Framework
