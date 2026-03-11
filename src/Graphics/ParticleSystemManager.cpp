#include "Precompiled.h"

namespace Framework {
	ParticleSystemManager::ParticleSystemManager() {}
	ParticleSystemManager::~ParticleSystemManager() {}

	// JSON helpers
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
		s.spawnRate = j.value("spawnRate", 0.0f);
		s.size = j.value("size", 1.0f);
		s.endSize = j.value("endSize", 0.0f);
		s.minLifetime = j.value("minLifetime", 0.0f);
		s.maxLifetime = j.value("maxLifetime", 1.0f);
		s.minSpeed = j.value("minSpeed", 0.0f);
		s.maxSpeed = j.value("maxSpeed", 1.0f);
		s.burstCnt = j.value("burstCount", 0);
		s.directionFuzz = j.value("directionFuzz", 0.0f);
		s.fadeOut = j.value("fadeOut", true);
		s.shrinkOverTime = j.value("shrinkOverTime", false);
		s.growOverTime = j.value("growOverTime", false);
		s.ownerPlayerID = j.value("ownerPlayerID", -1);

		if (j.contains("tint"))      s.tint = ReadTint(j["tint"]);
		if (j.contains("endTint"))   s.endTint = ReadTint(j["endTint"]);
		if (j.contains("direction")) {
			s.direction.x = j["direction"][0].get<float>();
			s.direction.y = j["direction"][1].get<float>();
		}
		if (j.contains("gravity")) {
			s.gravity.x = j["gravity"][0].get<float>();
			s.gravity.y = j["gravity"][1].get<float>();
		}

		return s;
	}

	// Internal
	ParticleSystem& ParticleSystemManager::AddParticleSystem() {
		particleSystems.emplace_back();   // create a new ParticleSystem
		return particleSystems.back();    // return reference so caller can configure it
	}

	// Public API
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
	}

	void ParticleSystemManager::Initialize() {
		ReloadFromJSON("assets/JSON/particles.json");
	}

	void ParticleSystemManager::Update(float dt) {
    // Update emitter positions to follow their owner entities
    if (CORE && CORE->GetEntityManager())
    {
        auto* em = CORE->GetEntityManager();
        for (auto& ps : particleSystems)
        {
            int ownerID = ps.GetOwnerPlayer();
            if (ownerID < 0) continue;  // always-on, no tracking needed
            
            // ownerPlayer here stores entity ID directly
            Entity entity(static_cast<uint32_t>(ownerID));
            if (em->HasComponent<Transform>(entity))
            {
                auto& t = em->GetComponent<Transform>(entity);
                ps.SetEmitter(t.position.x, t.position.y);
            }
        }
    }

		// Loop through all emitters | Update all particle systems
		for (auto& particleSystem : particleSystems) {
			particleSystem.Update(dt);
		}
	}

	void ParticleSystemManager::SendEngineMessage(Message* msg) {
	// Optional: print debug info
		std::cout << "[ParticleSystemManager] " << msg->MessageId << std::endl;
	}

	// Creates a new emitter at runtime from a named preset
	int ParticleSystemManager::SpawnEmitterFromPreset(const std::string& presetName, int entityID)
	{
		auto it = settings.find(presetName);
		if (it == settings.end()) {
			std::cout << "[PSM] Preset '" << presetName << "' not found\n";
			return -1;
		}

		// Get initial position from entity
		float x = 0.0f, y = 0.0f;
		if (CORE && CORE->GetEntityManager()) {
			Entity entity(static_cast<uint32_t>(entityID));
			auto* em = CORE->GetEntityManager();
			if (em->HasComponent<Transform>(entity)) {
				auto& t = em->GetComponent<Transform>(entity);
				x = t.position.x;
				y = t.position.y;
			}
		}

		auto& ps = AddParticleSystem();
		ParticleSystem::Settings s = it->second;
		s.ownerPlayerID = entityID;  // store entity ID for tracking
		ps.SetSettings(s);
		ps.SetEmitter(x, y);
		ps.SetActive(true);  // start active immediately

		int emitterID = nextEmitterId++;
		emitterIdToIndex[emitterID] = particleSystems.size() - 1;

		if (s.burstCnt > 0) ps.SpawnBurst(s.burstCnt);

		std::cout << "[PSM] Spawned '" << presetName << "' tracking entity=" << entityID << "\n";
		return emitterID;
	}
} // namespace Framework