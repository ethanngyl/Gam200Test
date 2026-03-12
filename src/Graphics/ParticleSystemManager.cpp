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

	// Helper functions
	static glm::vec4 ReadTint(const json& j) {
		return { j[0].get<float>(),
				 j[1].get<float>(),
				 j[2].get<float>(),
				 j[3].get<float>()
		};
	}

	static ParticleSystem::Settings ReadSettings(const json& j) {
		ParticleSystem::Settings s;

		s.layer = j.value("layer", 5);
		if (j.contains("tint")) s.tint = ReadTint(j["tint"]);
		s.spawnRate = j.value("spawnRate", 0.0f);
		s.size = j.value("size", 1.0f);
		s.minLifetime = j.value("minLifetime", 0.0f);
		s.maxLifetime = j.value("maxLifetime", 1.0f);
		s.minSpeed = j.value("minSpeed", 0.0f);
		s.maxSpeed = j.value("maxSpeed", 1.0f);
		s.burstCnt = j.value("burstCount", 0);
		if (j.contains("direction")) {
			s.direction.x = j["direction"][0].get<float>();
			s.direction.y = j["direction"][1].get<float>();
		}
		s.directionFuzz = j.value("directionFuzz", 0.0f);
		s.spawnRadius = j.value("spawnRadius", 0.0f);

		if (j.contains("endTint")) s.endTint = ReadTint(j["endTint"]);
		s.endSize = j.value("endSize", 0.0f);
		if (j.contains("gravity")) {
			s.gravity.x = j["gravity"][0].get<float>();
			s.gravity.y = j["gravity"][1].get<float>();
		}
		s.fadeOut = j.value("fadeOut", true);
		s.shrinkOverTime = j.value("shrinkOverTime", false);
		s.growOverTime = j.value("growOverTime", false);
		s.ownerPlayerID = j.value("ownerPlayerID", -1);

		return s;
	}

	ParticleSystem& ParticleSystemManager::AddParticleSystem() {
		particleSystems.emplace_back();   // create a new ParticleSystem
		return particleSystems.back();    // return reference so caller can configure it
	}

	int Framework::ParticleSystemManager::ReloadFromJSON(const std::string& path) {
		particleSystems.clear();
		settings.clear();
		emitterIdToIndex.clear();
		nextEmitterId = 1;

		std::ifstream file(path);
		if (!file.is_open()) {
			std::cout << "[PSM] " << path << " not found\n";
			return 0;
		}

		json root;
		file >> root;

		// load settings
		if (root.contains("settings")) {
			for (auto& [name, val] : root["settings"].items()) {
				ParticleSystem::Settings s = ReadSettings(val);
				settings[name] = s;
			}
		}

		int created = 0;
		return created;
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

	void ParticleSystemManager::Initialize() {
		ReloadFromJSON("assets/JSON/particles.json");
	}

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