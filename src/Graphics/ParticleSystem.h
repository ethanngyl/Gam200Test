#pragma once
#include "Precompiled.h"
#include "Component.h"		// for Particle

namespace Framework {
	class ParticleSystem {
	public:
		struct Settings {
			std::string texturePath	{""};
			glm::vec4	tint		{ 0.0f, 0.0f, 0.0f, 1.0f };
			int			layer		{ 5 };
			float		spawnRate	{ 0.0f };
			float		maxLifetime { 0.0f };
			float		minLifetime { 0.0f };
			float		maxSpeed	{ 0.0f };
			float		minSpeed	{ 0.0f };
			float		size		{ 0.0f };	// visual size (pixels)

			// Optional “directional” emission (for smoke up / explosion radial)
			//Vector2D	dir			{ 0.0f, 0.0f }; // (0,0) = radial/random
			//float		dirFuzz		{ 0.0f };		// 0 = straight, 1 = wide cone
		};

		void CreateParticle();
		void Update(float dt);

		void SetSpawnRate(float perSecond) { spawnRate = perSecond; }
		void SetEmitter(float x, float y) { emitter = { x, y }; }
		void SetSettings(const Settings& s) { settings = s; spawnRate = s.spawnRate; }
		void SetTexture(const std::string& path) { settings.texturePath = path; }
		void SetTint(const glm::vec4& c) { settings.tint = c; }
		void SetLayer(int l) { settings.layer = l; }
		void SetSizePx(float px) { settings.size = px; }
		void SetLifetime(float minL, float maxL) { settings.minLifetime = minL; settings.maxLifetime = maxL; }
		void SetSpeed(float minS, float maxS) { settings.minSpeed = minS; settings.maxSpeed = maxS; }
		//void SetDirection(Vector2D d, float fuzz) { settings.dir = d; settings.dirFuzz = fuzz; }

		void MoveEmitter(float dx, float dy) { emitter.x += dx; emitter.y += dy; }

	private:
		std::vector<Entity> particles;
		Vector2D emitter { 0.0f, 0.0f };
		Vector2D direction { 0.0f, 0.0f };
				
		float spawnRate = 0.0f;  // particles per second
		float spawnAcc = 0.0f;   // accumulator

		Settings settings;
	};
} // namespace Framework