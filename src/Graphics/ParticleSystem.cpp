#include "ParticleSystem.h"
#include "ECSEntityManager.h"

namespace Framework {
	void ParticleSystem::CreateParticle() {
		// Query active graphics system to convert pixel size into world-space scale
		// Size (pixel -> world conversion)
		auto* graphics = CORE->GetGraphicsSystem();
		//float pixelSize = 15.0f; // desired size in pixels
		float worldScale = (2.0f * settings.size) / float(graphics->GetRenderHeight());

		Entity entity = CORE->GetEntityManager()->CreateEntity();
		
		CORE->GetEntityManager()->AddComponent<Transform>(entity, emitter);
		auto& transform = CORE->GetEntityManager()->GetComponent<Transform>(entity);
		transform.position = emitter;
		transform.scale = { worldScale , worldScale };	// Size

		CORE->GetEntityManager()->AddComponent<Particle>(entity);
		auto& particle = CORE->GetEntityManager()->GetComponent<Particle>(entity);
		
		// Sprite
		CORE->GetEntityManager()->AddComponent<Sprite>(entity);
		auto& sprite = CORE->GetEntityManager()->GetComponent<Sprite>(entity);
		sprite.texturePath = settings.texturePath; //"assets/UI/AP_Crystal.png";   //test
		sprite.layer = settings.layer; //5;
		sprite.tint = settings.tint; //{ 1.0f, 1.0f, 1.0f, 1.0f };  //white

		// frand: gives any random number from 0.0f to 1.0f
		auto frand = []() { return float(std::rand()) / float(RAND_MAX); };
		
		// Lifetime
		particle.lifetime = settings.minLifetime + frand() * (settings.maxLifetime - settings.minLifetime);

		// Speed
		float speed = settings.minSpeed + frand() * (settings.maxSpeed - settings.minSpeed);
		//float speed = (frand()*0.25f)+0.01f;
		//float speed = (frand()*4.0f)+1.0f; - 1 to 5
		//float speed = (frand() * 2.0f) + 1.0f;

		// scale up by 2 then shift by -1 (Range: -1 to 1)
		direction.x = (frand()*2.0f)-1.0f;
		direction.y = (frand()*2.0f)-1.0f;

		// direction normalized
		direction.normalize();

		particle.velocity = {
			direction.x * speed,
			direction.y * speed
		};

		particle.lifetime = 10.0f;
		particle.age = 0.0f;
		particle.size = { 1.0f, 1.0f };
		particle.r = 0.0f;
		particle.g = 0.0f;
		particle.b = 0.0f;

		// Direction: if direction is (0,0) => random radial, else biased cone
		//Vector2D dir;
		//if (settings.direction.x == 0.0f && settings.direction.y == 0.0f) {
		//	dir.x = frand() * 2.0f - 1.0f;
		//	dir.y = frand() * 2.0f - 1.0f;
		//	dir.normalize();
		//}
		//else {
		//	dir = settings.direction;
		//	dir.normalize();
		//	// add fuzz
		//	dir.x += (frand() * 2.0f - 1.0f) * settings.directionFuzz;
		//	dir.y += (frand() * 2.0f - 1.0f) * settings.directionFuzz;
		//	dir.normalize();
		//}
		//particle.velocity = { dir.x * spd, dir.y * spd };

		// debug test console
		//std::cout << "[PS] Spawned entity= " << entity.id << " at (" << emitter.x << "," << emitter.y << ")\n";
		particles.push_back(entity);
	}

	void ParticleSystem::Update(float dt) {
		EntityManager* entityManager = CORE->GetEntityManager();

		// debug test console
		//static float printAcc = 0.0f;
		//printAcc += dt;
		//if (printAcc >= 1.0f) {
		//	std::cout << "[PS] Alive=" << particles.size()
		//		<< " spawnAcc=" << spawnAcc << "\n";
		//	printAcc = 0.0f;
		//}

		spawnRate = 20.0f;

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

			// Checks if components if is exist (prevents GetComponent throwing if something removed it)
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
		//if (!particles.empty() && printAcc == 0.0f) {
		//	auto& t0 = entityManager->GetComponent<Transform>(particles[0]);
		//	std::cout << "[PS] P0 pos=(" << t0.position.x << "," << t0.position.y << ")\n";
		//}
	}
} // namespace Framework