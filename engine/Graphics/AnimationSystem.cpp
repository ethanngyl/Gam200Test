#include "Precompiled.h"

namespace Framework {
	
	AnimationSystem::AnimationSystem()
		: currentFrame(0)
		, frameTimer(0.0f)
		, isPlaying(false)
		, entityManager(nullptr)
	{}

	AnimationSystem::~AnimationSystem() {}

	// === CORE LIFECYCLE ===

	void AnimationSystem::Initialize() {
		std::cout << "AnimationSystem: Initialized!" << std::endl;
	}

	void AnimationSystem::Update(float dt) {
		if (!entityManager) return;

		// === Sprite sheet frame stepping ===
		for (Entity e : entityManager->GetAllEntities()) {
			if (!entityManager->HasComponent<SpriteAnimation>(e)) continue;

			auto& anim = entityManager->GetComponent<SpriteAnimation>(e);

			if (!anim.playing) continue;

			anim.elapsedTime += dt;

			if (anim.elapsedTime >= anim.frameTime) {
				anim.elapsedTime = 0.0f;
				anim.currentFrame++;

				if (anim.currentFrame >= anim.frameCount) {
					anim.currentFrame = anim.loop ? 0 : anim.frameCount - 1;
				}

				std::cout << "Frame: " << anim.currentFrame << "\n";
			}
		}
	}

	void AnimationSystem::SendEngineMessage(Message* message) {
		// Restart animation when SPACE is pressed
		if (message->MessageId == KEY_SPACE) {
			currentFrame = 0;
			frameTimer = 0.0f;
			std::cout << "Animation: RESTARTED by space key!" << std::endl;
		}
	}

}