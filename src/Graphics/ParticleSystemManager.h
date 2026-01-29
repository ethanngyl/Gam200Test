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

		// Lua-friendly interface
		int CreateEmitter(const std::string& presetName, float x = 0.0f, float y = 0.0f);
		void DestroyEmitter(int emitterId);
		void SetEmitterPosition(int emitterId, float x, float y);
		void SetEmitterActive(int emitterId, bool active);
		void SpawnBurst(int emitterId, int count);
		int CreateTemporaryEffect(const std::string& presetName, float x, float y, float duration = 0.0f);

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

		// Temporary effects tracking
		struct TemporaryEffect {
			int emitterId;
			float remainingTime;
		};
		std::vector<TemporaryEffect> temporaryEffects;
	};
} // namespace Framework