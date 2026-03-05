#pragma once
#include "ParticleSystem.h"
#include <nlohmann/json.hpp>

using nlohmann::json;

namespace Framework {
	class ParticleSystemManager : public EngineSystem {
	public:
		ParticleSystemManager();
		virtual ~ParticleSystemManager();

		ParticleSystem& AddParticleSystem();

		int ReloadFromJSON(const std::string& path);

		int  SpawnEmitterFromPreset(const std::string& presetName, float x, float y, int ownerPlayerID);
		void SetActivePlayer(int playerIndex);

		// Manadatory overrides
		virtual void Initialize() override;
		virtual void Update(float dt) override;
		virtual void SendEngineMessage(Message* msg) override;
				
	private:
		// Internal data (emitters, particle systems)
		std::unordered_map<std::string, ParticleSystem::Settings> settings;     // table: "smoke" -> settings, "explosion" -> settings
		std::vector<ParticleSystem> particleSystems;
		std::unordered_map<int, size_t> emitterIdToIndex; // NEW: Map IDs to indices
		size_t controlled{ 0 };
		int nextEmitterId{ 1 };
	};
} // namespace Framework