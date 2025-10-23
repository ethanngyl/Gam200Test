#include "Precompiled.h"

namespace Framework {
	
	AnimationsSystem::AnimationsSystem()
		: currentFrame(0)
		, frameTimer(0.0f)
		, isPlaying(false)
		, demoInitialized(false)
	{}

	AnimationsSystem::~AnimationsSystem() {}

	// === CORE LIFECYCLE ===

	void AnimationsSystem::Initialize() {
		std::cout << "\n========================================\n";
		std::cout << "  AnimationSystem: Initializing\n";
		std::cout << "  >> Ready to create animations\n";
		std::cout << "========================================\n\n";
	}

	void AnimationsSystem::Update(float dt) {

	}

	void AnimationsSystem::SendEngineMessage(Message* message) {

	}

}