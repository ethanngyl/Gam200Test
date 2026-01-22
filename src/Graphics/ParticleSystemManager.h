#pragma once
#include "Precompiled.h"
#include "ParticleSystem.h"

namespace Framework {
	class ParticleSystem;

	class ParticleSystemManager : public EngineSystem {
	public:
		ParticleSystemManager();
		virtual ~ParticleSystemManager();

		ParticleSystem& AddParticleSystem();

		// Manadatory overrides
		virtual void Initialize() override;
		virtual void Update(float dt) override;
		virtual void SendEngineMessage(Message* msg) override;
				
	private:
		// Internal data (emitters, particle systems)
		std::vector<ParticleSystem> particleSystems;
	};
} // namespace Framework