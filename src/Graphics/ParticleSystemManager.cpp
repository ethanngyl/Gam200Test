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
		auto& ps = AddParticleSystem();
		ps.SetSettings(s);
		ps.SetEmitter(x, y);
		if (followTarget != INVALID_ENTITY) ps.SetFollowEntity(followTarget);
		if (s.burstCnt > 0) ps.SpawnBurst(s.burstCnt);
		int emitterId = nextEmitterId++;
		emitterIdToIndex[emitterId] = particleSystems.size() - 1;
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
		nextEmitterId = 1;
	}

	void ParticleSystemManager::Initialize() {}

	void ParticleSystemManager::Update(float dt) {
		// Loop through all emitters | Update all particle systems
		for (auto& particleSystem : particleSystems) {
			particleSystem.UpdateParticle(dt);
		}
	}

	void ParticleSystemManager::SendEngineMessage(Message* msg) {
	// Optional: print debug info
		std::cout << "[ParticleSystemManager] " << msg->MessageId << std::endl;
	}
} // namespace Framework
