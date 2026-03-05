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
		
		s.texturePath = j.value("texturePath", "");
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
		temporaryEffects.clear();
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

		// spawn emitters from JSON (POSITIONS FROM JSON)
		if (root.contains("emitters")) {
			for (auto& emitter : root["emitters"]) {
				std::string settingName = emitter.value("setting", "");
				auto it = settings.find(settingName);
				if (it == settings.end()) continue;

				auto& ps = AddParticleSystem();
				ps.SetSettings(it->second);

				auto pos = emitter["emitter"];
				ps.SetEmitter(pos[0].get<float>(), pos[1].get<float>());

				int emitterId = nextEmitterId++;
				emitterIdToIndex[emitterId] = particleSystems.size() - 1;

				int burst = ps.GetSettings().burstCnt;
				if (burst > 0) ps.SpawnBurst(burst);

				created++;
			}
		}

		controlled = 0;
		return created;
	}

	void ParticleSystemManager::Initialize() {
		ReloadFromJSON("assets/JSON/particles.json");
	}

	void ParticleSystemManager::Update(float dt) {
		// Loop through all emitters | Update all particle systems
		for (auto& particleSystem : particleSystems) {
			particleSystem.Update(dt);
		}
	}

	void ParticleSystemManager::SendEngineMessage(Message* msg) {
	// Optional: print debug info
		std::cout << "[ParticleSystemManager] " << msg->MessageId << std::endl;
	}
} // namespace Framework