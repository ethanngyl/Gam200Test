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

		// Lua-friendly interface
		int CreateEmitterRaw(const ParticleSystem::Settings& s, float x, float y, EntityID followTarget = INVALID_ENTITY);
		void SetFollowEntity(int emitterId, EntityID targetID);

		// Mandatory overrides
		virtual void Initialize() override;
		virtual void Update(float dt) override;
		virtual void SendEngineMessage(Message* msg) override;

	private:
		// Internal data (emitters, particle systems)
		std::unordered_map<std::string, ParticleSystem::Settings> settings;     // table: "smoke" -> settings, "explosion" -> settings
		std::vector<ParticleSystem> particleSystems;
		std::unordered_map<int, size_t> emitterIdToIndex; // Map IDs to indices
		int nextEmitterId{ 1 };
	};
} // namespace Framework