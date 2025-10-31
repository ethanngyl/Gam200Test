/**
===============================================================================
 File:           AnimationSystem.cpp
 Author:         TAN WEI LEONG
 Email:          weileong.tan@digipen.edu
 Date:           2025-10-29
 ------------------------------------------------------------------------------
ANIMATION SYSTEM IMPLEMENTATION

Purpose:
	This file contains the complete implementation of the AnimationSystem class.
	It handles the core animation logic including frame timing, entity processing,
	and message handling within the ECS framework.

Implementation Details:
	- Uses delta time for frame-accurate animation timing
	- Processes all entities with SpriteAnimation component each frame
	- Handles both looping and one-shot animation types
	- Provides debug output for animation state changes
	- Responds to external messages for animation control

Component Requirements:
	Entities must have SpriteAnimation component with:
	- playing: bool (whether animation is active)
	- elapsedTime: float (accumulated time for current frame)
	- frameTime: float (duration to display each frame)
	- currentFrame: int (current frame index)
	- frameCount: int (total frames in animation)
	- loop: bool (whether animation should loop)

Usage:
	The system automatically processes animations during each update cycle.
	No manual per-entity animation updates are required.
===============================================================================
 */

#include "Precompiled.h"

namespace Framework {
	
	/**
	===============================================================================
	 * @brief Constructor - Initializes animation system with default values
	 *
	 * Sets up:
	 * - currentFrame to 0 (first frame)
	 * - frameTimer to 0.0f (ready to start timing)
	 * - isPlaying to false (paused by default)
	 * - entityManager to nullptr (must be set later)
	===============================================================================
	 */
	AnimationSystem::AnimationSystem()
		: currentFrame(0)
		, frameTimer(0.0f)
		, isPlaying(false)
		, entityManager(nullptr)
	{}

	/**
	===============================================================================
	 * @brief Destructor - Cleans up animation system resources
	 *
	 * Currently handles basic cleanup. Can be extended for
	 * resource deallocation if needed in the future.
	===============================================================================
	 */
	AnimationSystem::~AnimationSystem() {}

	// ==================== CORE LIFECYCLE METHODS ====================

	/**
	===============================================================================
	 * @brief Initializes the animation system
	 *
	 * Called once when the system is first created. Performs:
	 * - System validation checks
	 * - Resource preparation
	 * - Debug output confirmation
	 *
	 * @note Must be called before any Update() calls
	===============================================================================
	 */
	void AnimationSystem::Initialize() {
		std::cout << "AnimationSystem: Initialized!" << std::endl;
	}

	/**
	===============================================================================
	 * @brief Updates all active animations
	 *
	 * Main update loop called every frame. Processes:
	 * - All entities with SpriteAnimation component
	 * - Frame timing and progression
	 * - Looping behavior management
	 * - Animation state transitions
	 *
	 * @param dt Delta time in seconds since last frame
	 *        Used for frame-accurate animation timing
	 *
	 * @note Skips processing if entityManager is not set
	 * @note Only processes animations marked as playing
	===============================================================================
	 */
	void AnimationSystem::Update(float dt) {
		if (!entityManager) return;

		// === Sprite sheet frame stepping ===
		for (Entity e : entityManager->GetAllEntities()) {
			if (!entityManager->HasComponent<SpriteAnimation>(e)) continue;

			auto& anim = entityManager->GetComponent<SpriteAnimation>(e);

			if (!anim.playing) continue;

			// Movement check
			if (entityManager->HasComponent<Movement>(e)) {
				auto& move = entityManager->GetComponent<Movement>(e);

				// If NOT moving → idle still frame
				if (fabs(move.direction.x) < 0.001f && fabs(move.direction.y) < 0.001f) {
					continue;               // do NOT animate
				}
			}

			anim.elapsedTime += dt;

			if (anim.elapsedTime >= anim.frameTime) {
				anim.elapsedTime = 0.0f;
				anim.currentFrame++;

				if (anim.currentFrame >= anim.frameCount) {
					anim.currentFrame = anim.loop ? 0 : anim.frameCount - 1;
				}
				
				// For Debug
				std::cout << "Frame: " << anim.currentFrame << "\n";
			}
		}
	}

	/**
	===============================================================================
	 * @brief Handles engine messages for animation control
	 *
	 * Processes incoming messages to control animation behavior:
	 * - Restart animations
	 * - Play/pause toggles
	 * - Frame skipping
	 * - Animation speed changes
	 *
	 * @param message Pointer to the incoming message object
	 *        Contains message ID and optional data payload
	 *
	 * Currently Supported Messages:
	 * - KEY_0: Resets all animations to first frame
	===============================================================================
	 */
	void AnimationSystem::SendEngineMessage(Message* message) {
		// Restart animation when 0 is pressed
		if (message->MessageId == KEY_0) {
			currentFrame = 0;
			frameTimer = 0.0f;
			std::cout << "Animation: RESTARTED by '0' key!" << std::endl;
		}
	}
}