#include "Precompiled.h"

namespace Framework {
	
	AnimationSystem::AnimationSystem()
		: currentFrame(0)
		, frameTimer(0.0f)
		, isPlaying(false)
		, demoInitialized(false)
	{}

	AnimationSystem::~AnimationSystem() {}

	// === CORE LIFECYCLE ===

	void AnimationSystem::Initialize() {
		std::cout << "\n========================================\n";
		std::cout << "  AnimationSystem: Initializing\n";
		std::cout << "  >> Ready to create animations\n";
		std::cout << "========================================\n\n";
	}

	void AnimationSystem::Update(float dt) {

	}

	void AnimationSystem::SendEngineMessage(Message* message) {

	}

}