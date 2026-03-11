/*
===============================================================================
File:        ParticleSystem.h
-------------------------------------------------------------------------------
Brief:
ParticleSystem manages the lifecycle of all particles in the engine.
It iterates entities with ParticleEmitter + Transform, spawns new particles
based on emission rate, updates positions/colors/sizes over lifetime,
and removes dead particles. Rendering is handled by GraphicsSystemV2.
===============================================================================
*/
#pragma once
#include "Interface.h"

namespace Framework {
	class ParticleSystem {
	public:
		struct Settings {
			std::string texturePath	{""};
			glm::vec4	tint		{ 0.0f, 0.0f, 0.0f, 1.0f };
			int			layer		{ 0 };
			float		spawnRate	{ 0.0f };
			float		maxLifetime { 0.0f };
			float		minLifetime { 0.0f };
			float		maxSpeed	{ 0.0f };
			float		minSpeed	{ 0.0f };
			float		size		{ 0.0f };	// visual size (pixels)
			int			burstCnt	{ 0 };		// 0 = continuous; >0 = spawn this many once at start

			// directional emission (for smoke up / explosion radial)
			Vector2D	direction		{ 0.0f, 0.0f }; // (0,0) = radial/random
			float		directionFuzz	{ 0.0f };		// 0 = straight, 1 = wide cone

			glm::vec4	endTint{ 1.0f, 1.0f, 1.0f, 0.0f }; // For fade out
			float		endSize{ 0.0f }; // For size change
			Vector2D	gravity{ 0.0f, 0.0f };
			bool		fadeOut{ true };
			bool		shrinkOverTime{ false };
			bool		growOverTime{ false };

			int         ownerPlayerID{ -1 };  // -1 = always active, 0-2 = specific player
		};

    class EntityManager;
    class GraphicsSystemV2;
		void CreateParticle();
		void Update(float dt);

		void SetEmitter(float x, float y) { emitter = { x, y }; }
		void SetSettings(const Settings& s) { settings = s; }
    class ParticleSystem : public EngineSystem {
    public:
        ParticleSystem();
        ~ParticleSystem() override;

		const Settings& GetSettings() const { return settings; }
		Vector2D GetEmitter() const { return emitter; }

		bool IsActive() const { return active; }
		void SetActive(bool a) { active = a;  }

		size_t GetParticle() const { return particles.size(); } // Get particle count

		void SpawnBurst(int count) { for (int i{ 0 }; i < count; ++i) CreateParticle(); } // Burst spawning
        void Initialize() override;
        void Update(float dt) override;
        void SendEngineMessage(Message* message) override;

        void SetEntityManager(EntityManager* em) { entityManager = em; }
        void SetGraphicsSystem(GraphicsSystemV2* gs) { graphicsSystem = gs; }
		void SetOwnerPlayer(int playerID) { settings.ownerPlayerID = playerID; }
		int GetOwnerPlayer() const { return settings.ownerPlayerID; }

    private:
		std::vector<Entity> particles;
		Vector2D emitter { 0.0f, 0.0f };
				
		float spawnAcc = 0.0f;   // accumulator
		bool active = true;	     // toggle controlled emitter
        void SpawnParticles(struct ParticleEmitter& emitter, const struct Transform& transform, float dt);
        void UpdateParticles(struct ParticleEmitter& emitter, float dt);

        EntityManager* entityManager = nullptr;
        GraphicsSystemV2* graphicsSystem = nullptr;
		Settings settings;
    };

} // namespace Framework
