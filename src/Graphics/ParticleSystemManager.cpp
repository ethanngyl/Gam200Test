#include "Precompiled.h"
#include "ECSComponent.h"			// for entity/component access
#include "Component.h"				// for Transform/Velocity, etc.

namespace Framework {
	ParticleSystemManager::ParticleSystemManager() {}

	ParticleSystemManager::~ParticleSystemManager() {}

	ParticleSystem& ParticleSystemManager::AddParticleSystem() {
		particleSystems.emplace_back();   // create a new ParticleSystem
		return particleSystems.back();    // return reference so caller can configure it
	}

	void ParticleSystemManager::Initialize() {
		// Load any default particle systems (fire, smoke, etc.)
		auto& particleSystem = AddParticleSystem();
		particleSystem.SetSpawnRate(10.0f);

		// Spawn ONE particle to verify it works (or set an emitter position)
		particleSystem.CreateParticle();
	}

	void ParticleSystemManager::Update(float dt) {
		// Loop through all emitters (for now only 1)
		for (auto& particleSystem : particleSystems) {
			particleSystem.Update(dt);
		}
	}

	void ParticleSystemManager::SendEngineMessage(Message* msg) {
	// Optional: print debug info
		std::cout << "[ParticleSystemManager] " << msg->MessageId << std::endl;
	}
} // namespace Framework