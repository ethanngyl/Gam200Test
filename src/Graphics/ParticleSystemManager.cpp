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

		return s;
	}

	ParticleSystem& ParticleSystemManager::AddParticleSystem() {
		particleSystems.emplace_back();   // create a new ParticleSystem
		return particleSystems.back();    // return reference so caller can configure it
	}

	// Lua-friendly emitter creation
	int ParticleSystemManager::CreateEmitter(const std::string& presetName, float x, float y) {
		auto it = settings.find(presetName);
		if (it == settings.end()) {
			std::cout << "[PSM] Unknown preset: " << presetName << std::endl;
			return -1;
		}

		auto& ps = AddParticleSystem();
		ps.SetSettings(it->second);
		ps.SetEmitter(x, y);

		int emitterId = nextEmitterId++;
		emitterIdToIndex[emitterId] = particleSystems.size() - 1;

		// If it's a burst emitter, spawn immediately
		if (ps.GetSettings().burstCnt > 0) {
			ps.SpawnBurst(ps.GetSettings().burstCnt);
		}

		return emitterId;
	}

	void ParticleSystemManager::DestroyEmitter(int emitterId) {
		auto it = emitterIdToIndex.find(emitterId);
		if (it != emitterIdToIndex.end()) {
			size_t index = it->second;
			if (index < particleSystems.size()) {
				// Mark as inactive
				particleSystems[index].SetActive(false);
				emitterIdToIndex.erase(it);
			}
		}
	}

	void ParticleSystemManager::SetEmitterPosition(int emitterId, float x, float y) {
		auto it = emitterIdToIndex.find(emitterId);
		if (it != emitterIdToIndex.end()) {
			size_t index = it->second;
			if (index < particleSystems.size()) {
				particleSystems[index].SetEmitter(x, y);
			}
		}
	}

	void ParticleSystemManager::SetEmitterActive(int emitterId, bool active) {
		auto it = emitterIdToIndex.find(emitterId);
		if (it != emitterIdToIndex.end()) {
			size_t index = it->second;
			if (index < particleSystems.size()) {
				particleSystems[index].SetActive(active);
			}
		}
	}

	void ParticleSystemManager::SpawnBurst(int emitterId, int count) {
		auto it = emitterIdToIndex.find(emitterId);
		if (it != emitterIdToIndex.end()) {
			size_t index = it->second;
			if (index < particleSystems.size()) {
				particleSystems[index].SpawnBurst(count);
			}
		}
	}

	int ParticleSystemManager::CreateTemporaryEffect(const std::string& presetName, float x, float y, float duration) {
		int emitterId = CreateEmitter(presetName, x, y);
		if (emitterId > 0 && duration > 0.0f) {
			temporaryEffects.push_back({ emitterId, duration });
		}
		return emitterId;
	}

	void ParticleSystemManager::Initialize() {
		particleSystems.clear();
		settings.clear();
		emitterIdToIndex.clear();
		temporaryEffects.clear();
		nextEmitterId = 1;

		std::ifstream file("assets/JSON/particles.json");
		if (!file.is_open()) {
			std::cout << "[PSM] particles.json not found, using defaults\n";
			return;
		}

		json root;
		file >> root;

		// load settings with enhanced properties
		if (root.contains("settings")) {
			for (auto& [name, val] : root["settings"].items()) {
				ParticleSystem::Settings s = ReadSettings(val);

				// Read enhanced properties if they exist
				if (val.contains("endTint")) s.endTint = ReadTint(val["endTint"]);
				s.endSize = val.value("endSize", 0.0f);
				if (val.contains("gravity")) {
					s.gravity.x = val["gravity"][0].get<float>();
					s.gravity.y = val["gravity"][1].get<float>();
				}
				s.fadeOut = val.value("fadeOut", true);
				s.shrinkOverTime = val.value("shrinkOverTime", false);

				settings[name] = s;
			}
		}

		// spawn emitters
		if (root.contains("emitters")) {
			for (auto& emitter : root["emitters"]) {
				std::string settingName = emitter.value("setting", "");
				auto settingIterator = settings.find(settingName);
				if (settingIterator == settings.end()) continue;

				auto& ps = AddParticleSystem();
				ps.SetSettings(settingIterator->second);

				auto position = emitter["emitter"];
				ps.SetEmitter(position[0].get<float>(), position[1].get<float>());

				// Store emitter ID
				int emitterId = nextEmitterId++;
				emitterIdToIndex[emitterId] = particleSystems.size() - 1;

				// Burst emitter, spawn once
				int burst = ps.GetSettings().burstCnt;
				if (burst > 0) {
					ps.SpawnBurst(burst);
				}
			}
		}

		controlled = 0; // smoke emitter
	}

	void ParticleSystemManager::Update(float dt) {
		// Update temporary effects
		for (auto it = temporaryEffects.begin(); it != temporaryEffects.end(); ) {
			it->remainingTime -= dt;
			if (it->remainingTime <= 0.0f) {
				DestroyEmitter(it->emitterId);
				it = temporaryEffects.erase(it);
			}
			else {
				++it;
			}
		}

		// Loop through all emitters | Update all particle systems
		for (auto& particleSystem : particleSystems) {
			particleSystem.Update(dt);
		}

			// Move the controlled emitter
			if (particleSystems.empty()) return;
			if (controlled >= particleSystems.size()) controlled = 0;

			auto* input = CORE->GetInputSystem();
			if (!input) return;

			float speed = 1.0f; // world units per second
			float dx = 0.0f, dy = 0.0f;

			// Movement for controlld emitter
			if (input->IsKeyDown(KeyCode::KEY_W)) dy += speed * dt;
			if (input->IsKeyDown(KeyCode::KEY_A)) dx -= speed * dt;
			if (input->IsKeyDown(KeyCode::KEY_S)) dy -= speed * dt;
			if (input->IsKeyDown(KeyCode::KEY_D)) dx += speed * dt;

			particleSystems[controlled].MoveEmitter(dx, dy);
	}

	// Serialization methods
	void ParticleSystemManager::SaveToJSON(const std::string& filename) {
		json root;

		// Save settings
		json settingsJson;
		for (const auto& [name, setting] : settings) {
			json s;
			s["texturePath"] = setting.texturePath;
			s["layer"] = setting.layer;
			s["tint"] = { setting.tint.r, setting.tint.g, setting.tint.b, setting.tint.a };
			s["endTint"] = { setting.endTint.r, setting.endTint.g, setting.endTint.b, setting.endTint.a };
			s["spawnRate"] = setting.spawnRate;
			s["size"] = setting.size;
			s["endSize"] = setting.endSize;
			s["minLifetime"] = setting.minLifetime;
			s["maxLifetime"] = setting.maxLifetime;
			s["minSpeed"] = setting.minSpeed;
			s["maxSpeed"] = setting.maxSpeed;
			s["burstCount"] = setting.burstCnt;
			s["direction"] = { setting.direction.x, setting.direction.y };
			s["directionFuzz"] = setting.directionFuzz;
			s["gravity"] = { setting.gravity.x, setting.gravity.y };
			s["fadeOut"] = setting.fadeOut;
			s["shrinkOverTime"] = setting.shrinkOverTime;

			settingsJson[name] = s;
		}
		root["settings"] = settingsJson;

		// Save active emitters
		json emittersJson = json::array();
		for (const auto& [emitterId, index] : emitterIdToIndex) {
			if (index < particleSystems.size()) {
				const auto& ps = particleSystems[index];
				if (ps.IsActive()) {
					json emitter;
					emitter["id"] = emitterId;
					emitter["preset"] = "Custom"; // Would need to find which preset
					const auto& pos = ps.GetEmitter();
					emitter["position"] = { pos.x, pos.y };
					emitter["active"] = ps.IsActive();
					emittersJson.push_back(emitter);
				}
			}
		}
		root["activeEmitters"] = emittersJson;

		std::ofstream file(filename);
		file << root.dump(2);
	}

	void ParticleSystemManager::LoadFromJSON(const std::string& filename) {
		std::ifstream file(filename);
		if (!file.is_open()) return;

		json root;
		file >> root;

		// Clear existing
		particleSystems.clear();
		emitterIdToIndex.clear();
		temporaryEffects.clear();

		// Load settings
		if (root.contains("settings")) {
			settings.clear();
			for (auto& [name, val] : root["settings"].items()) {
				settings[name] = ReadSettings(val);
			}
		}

		// Load active emitters
		if (root.contains("activeEmitters")) {
			for (auto& emitter : root["activeEmitters"]) {
				std::string preset = emitter.value("preset", "");
				if (preset == "Custom") continue; // Skip custom for now

				auto position = emitter["position"];
				int emitterId = CreateEmitter(preset, position[0].get<float>(), position[1].get<float>());

				bool active = emitter.value("active", true);
				SetEmitterActive(emitterId, active);
			}
		}
	}

	void ParticleSystemManager::SendEngineMessage(Message* msg) {
	// Optional: print debug info
		std::cout << "[ParticleSystemManager] " << msg->MessageId << std::endl;
	}
} // namespace Framework