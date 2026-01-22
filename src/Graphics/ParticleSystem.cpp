#include "ParticleSystem.h"
#include "ECSEntityManager.h"
#include "EntitySpawner.h"

namespace Framework {
	ParticleSystem::ParticleSystem() {}
	//ParticleSystem::~ParticleSystem() {}

	void ParticleSystem::CreateParticle() {
		Entity entity = engine->GetEntityManager()->CreateEntity();
		
		engine->GetEntityManager()->AddComponent<Transform>(entity, emitter);
		auto& transform = engine->GetEntityManager()->GetComponent<Transform>(entity);
		transform.position = emitter;
		transform.scale = { 0.05f, 0.05f };

		engine->GetEntityManager()->AddComponent<Particle>(entity);
		auto& particle = engine->GetEntityManager()->GetComponent<Particle>(entity);

		//auto& meshrenderer = engine->GetEntityManager()->AddComponent<MeshRenderer>(entity);
		//meshrenderer.tint.r = particle.r;
		//meshrenderer.tint.g = particle.g;
		//meshrenderer.tint.b = particle.b;

		engine->GetEntityManager()->AddComponent<Sprite>(entity);
		auto& sprite = engine->GetEntityManager()->GetComponent<Sprite>(entity);
		sprite.texturePath = "assets/TileMap/Grass_Block.png";   //test
		sprite.layer = 5;
		sprite.tint = { 1.0f, 1.0f, 1.0f, 1.0f };  //red

		auto frand = []() { return float(std::rand()) / float(RAND_MAX); };	  // 0 to 1

		//particle.position = emitter;
		
		//particle.velocity = { (frand() * 2.0f - 1.0f) * 100.0f, (frand() * 2.0f - 1.0f) * 100.0f };
		
		float speed = (frand()*0.25f)+0.01f;
		float fuzz = 0.3f;

		dir.x = (frand()*2.0f)-1.0f; // scale up by 2 then shift by -1 ( to get -1 to 1)
		dir.y = (frand()*2.0f)-1.0f;

		dir.normalize(); // direction normalized

		particle.velocity = {
			dir.x * speed,
			dir.y * speed
		};

		particle.lifetime = 10.0f;
		particle.age = 0.0f;
		particle.size = { 1.0f, 1.0f };
		particle.r = 0.0f;
		particle.g = 0.0f;
		particle.b = 0.0f;

		// debug test console
		std::cout << "[PS] Spawned entity= " << entity.id << " at (" << emitter.x << "," << emitter.y << ")\n";
		particles.push_back(entity);
	}

	void ParticleSystem::Update(float dt) {
		EntityManager* entityManager = engine->GetEntityManager();

		// debug test console
		static float printAcc = 0.0f;
		printAcc += dt;
		if (printAcc >= 1.0f) {
			std::cout << "[PS] Alive=" << particles.size()
				<< " spawnAcc=" << spawnAcc << "\n";
			printAcc = 0.0f;
		}

		// spawn new particles over time
		if (spawnRate > 0.0f) {
			int spawned = 0;
			spawnAcc += dt * spawnRate; // how many particles we "owe"
			while (spawnAcc >= 1.0f && spawned < 2) {
				CreateParticle();
				spawnAcc -= 1.0f;
				spawned++;
			}
		}

		// update existing particles
		for (auto count = particles.begin(); count != particles.end(); ) {
			Entity entity = *count;

			// safety (prevents GetComponent throwing if something removed it)
			if (!entityManager->HasComponent<Transform>(entity) || !entityManager->HasComponent<Particle>(entity)) {
				count = particles.erase(count);
				continue;
			}

			auto& transform = entityManager->GetComponent<Transform>(entity);
			auto& particle = entityManager->GetComponent<Particle>(entity);

			transform.position.x += particle.velocity.x * dt;
			transform.position.y += particle.velocity.y * dt;

			particle.age += dt;
			particle.lifetime -= dt;

			if (particle.lifetime <= 0.0f) {
				entityManager->DestroyEntity(entity);
				count = particles.erase(count);
			}
			else {
				++count;
			}
		}

		// debug test console
		if (!particles.empty() && printAcc == 0.0f) {
			auto& t0 = entityManager->GetComponent<Transform>(particles[0]);
			std::cout << "[PS] P0 pos=(" << t0.position.x << "," << t0.position.y << ")\n";
		}

		/*for (auto& p : particles) {
			
			p += p.velocity.x * dt;
			p.position.y += p.velocity.y * dt;
			p.lifetime -= dt;
		}
		particles.erase(
			std::remove_if(particles.begin(), particles.end(),
				[](Particle& p) { return p.lifetime <= 0.0f; }),
			particles.end()
		);

		emitter.x += spawnRate * dt;
		emitter.y += spawnRate * dt;*/
	}
} // namespace Framework