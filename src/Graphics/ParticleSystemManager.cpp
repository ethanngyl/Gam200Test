#include "Precompiled.h"
#include "ECSComponent.h"			// for entity/component access
#include "Component.h"				// for Transform/Velocity, etc.

namespace Framework {
	ParticleSystemManager::ParticleSystemManager() {}

	ParticleSystemManager::~ParticleSystemManager() {}

	ParticleSystem& ParticleSystemManager::AddParticleSystem() {
		particleSystems.emplace_back();   // create a new ParticleSystem
		return particleSystems.back();    // return reference so caller can configure it
		//particleSystems.push_back(std::make_unique<ParticleSystem>());
		//return *particleSystems.back();
	}

	void ParticleSystemManager::Initialize() {
		// Load particle systems (explosion, fire, smoke, etc.)
		particleSystems.clear();

		//SetEmitter(0.0f, 0.0f);    // center
		//SetEmitter(0.5f, 0.5f);    // top-right
		//SetEmitter(0.5f, -0.5f);   // bottom-right (4th quadrant)
		//SetEmitter(-0.5f, -0.5f);  // bottom-left

		 // Smoke emitter (continuous)
		auto& smoke = AddParticleSystem();
		ParticleSystem::Settings smokeSetting;
		smokeSetting.texturePath = "assets/UI/AP_Crystal.png";     // or smoke puff sprite
		smokeSetting.tint = { 1.0f, 1.0f, 1.0f, 1.0f };
		smokeSetting.layer = 5;
		smokeSetting.spawnRate = 40.0f;
		smokeSetting.size = 20.0f;
		smokeSetting.minLifetime = 1.0f;
		smokeSetting.maxLifetime = 5.0f;
		smokeSetting.minSpeed = 0.1f;
		smokeSetting.maxSpeed = 0.3f;
		//smokeSetting.direction = { 0.0f, 1.0f };        // up
		//smokeSetting.directionFuzz = 0.4f;            // a bit spread
		smoke.SetSettings(smokeSetting);
		smoke.SetEmitter(0.0f, 0.0f);
		smoke.CreateParticle();

		// Explosion emitter (burst-like feel)
		auto& explosion = AddParticleSystem();
		ParticleSystem::Settings explosionSetting;
		explosionSetting.texturePath = "assets/TileMap/Grass_Block.png";  // replace with explosion sprite
		explosionSetting.tint = { 1.0f, 1.0f, 1.0f, 1.0f };
		explosionSetting.layer = 5;
		explosionSetting.spawnRate = 20.0f;    // don’t continuously spawn
		explosionSetting.size = 20.0f;
		explosionSetting.minLifetime = 0.2f;
		explosionSetting.maxLifetime = 0.6f;
		explosionSetting.minSpeed = 1.0f;
		explosionSetting.maxSpeed = 5.0f;
		//explosionSetting.direction = { 0.0f, 0.0f }; // radial
		//explosionSetting.directionFuzz = 1.0f;
		explosion.SetSettings(explosionSetting);
		explosion.SetEmitter(0.5f, 0.0f);
		explosion.CreateParticle();

		// do a burst by manually spawning a bunch once
		//for (int i = 0; i < 60; ++i) explosion.CreateParticle();

		controlled = 0; // smoke emitter
	}

	void ParticleSystemManager::Update(float dt) {
		// Loop through all emitters | Update all particle systems (spawning + moving particles)
		for (auto& particleSystem : particleSystems) {
			particleSystem.Update(dt);

			// Move the controlled emitter
			if (particleSystems.empty()) return;
			if (controlled >= particleSystems.size()) controlled = 0;

			auto* input = CORE->GetInputSystem();
			if (!input) return;

			float speed = 1.0f; // world units per second
			float dx = 0.0f, dy = 0.0f;

			if (input->IsKeyDown(KeyCode::KEY_W)) dy += speed * dt;
			if (input->IsKeyDown(KeyCode::KEY_A)) dx -= speed * dt;
			if (input->IsKeyDown(KeyCode::KEY_S)) dy -= speed * dt;
			if (input->IsKeyDown(KeyCode::KEY_D)) dx += speed * dt;

			particleSystems[controlled].MoveEmitter(dx, dy);
		}
	}

	void ParticleSystemManager::SendEngineMessage(Message* msg) {
	// Optional: print debug info
		std::cout << "[ParticleSystemManager] " << msg->MessageId << std::endl;
	}
} // namespace Framework