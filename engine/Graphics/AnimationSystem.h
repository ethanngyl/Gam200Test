/**
===============================================================================
 File:           AnimationSystem.h
 Author:         TAN WEI LEONG
 Email:          weileong.tan@digipen.edu
 Date:           2025-11-04
 ------------------------------------------------------------------------------
ANIMATION SYSTEM HEADER

Purpose:
    This system manages sprite-based animations within the Entity-Component-System (ECS) framework.
    It handles frame progression, timing, looping, and animation state management for all entities
    that contain the SpriteAnimation component.

Key Features:
    - Frame-based animation with customizable durations
    - Support for looping and one-shot animations
    - Message-driven animation control (restart, pause, etc.)
    - Integration with ECS architecture

Dependencies:
    - ECSEntityManager.h for entity and component management
    - Precompiled.h for common includes and definitions

Usage:
    1. Create AnimationSystem instance
    2. Set EntityManager using SetEntityManager()
    3. System automatically processes entities with SpriteAnimation component
    4. Send messages for external control (Not in use)

Example:
    AnimationSystem* animSystem = new AnimationSystem();
    animSystem->SetEntityManager(entityManager);
    animSystem->Initialize();
===============================================================================
 */

#pragma once
#include "Precompiled.h"
#include "ECSEntityManager.h"

namespace Framework {

    /**
    ===============================================================================
     * @class AnimationSystem
     * @brief Manages and updates sprite animations for entities in the ECS framework
     *
     * The AnimationSystem processes all entities that have a SpriteAnimation component.
     * It advances frames based on elapsed time, handles looping behavior, and responds
     * to external messages for animation control.
     *
     * System Flow:
     *   1. During Update(), iterates through all entities
     *   2. Checks for SpriteAnimation component presence
     *   3. Updates animation state for active animations
     *   4. Advances frames when frame duration is exceeded
     *   5. Handles looping or clamping at animation end
     *
     * Message Handling:
     *   - KEY_0: Restarts all animations (resets to frame 0)
    ===============================================================================
     */
	class AnimationSystem : public EngineSystem {
	public:
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
		AnimationSystem();

        /**
        ===============================================================================
         * @brief Destructor - Cleans up animation system resources
         *
         * Currently handles basic cleanup. Can be extended for
         * resource deallocation if needed in the future.
        ===============================================================================
         */
		virtual ~AnimationSystem();


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
		virtual void Initialize() override;

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
		virtual void Update(float dt) override;

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
        ===============================================================================
         */
		virtual void SendEngineMessage(Message* message) override;

        /**
        ===============================================================================
         * @brief Sets the entity manager for component access
         *
         * Provides the system with access to entity and component data.
         * Must be called before the system can process any animations.
         *
         * @param em Pointer to the EntityManager instance
         *        Used to query entities and access SpriteAnimation components
        ===============================================================================
         */
		void SetEntityManager(Framework::EntityManager* em) { entityManager = em; }

	private:
        // ==================== ANIMATION DATA MEMBERS ====================

        /**
        ===============================================================================
         * @brief Collection of animation frame indices
         *
         * Stores the sequence of frame indices that make up an animation.
         * Each integer represents a frame position in the sprite sheet.
         *
         * Example: [0, 1, 2, 3] for a 4-frame walk cycle
        ===============================================================================
         */
        std::vector<int> animationFrames;

        /**
        ===============================================================================
         * @brief Duration (in seconds) for each corresponding frame
         *
         * Parallel array to animationFrames. Specifies how long each
         * frame should be displayed before advancing to the next.
         *
         * Example: [0.1f, 0.1f, 0.1f, 0.15f] for varied frame timing
        ===============================================================================
         */
        std::vector<float> frameDurations;

        /**
        ===============================================================================
         * @brief Index of the currently displayed frame
         *
         * Points to the current position in animationFrames array.
         * Range: 0 to (animationFrames.size() - 1)
        ===============================================================================
         */
        int currentFrame;

        /**
        ===============================================================================
         * @brief Timer tracking time spent on current frame
         *
         * Accumulates delta time each frame. When this value exceeds
         * the current frame's duration, the animation advances.
         * Resets to 0.0f when frame changes.
        ===============================================================================
         */
        float frameTimer;

        /**
        ===============================================================================
         * @brief Flag indicating if animation is currently playing
         *
         * When true: animation advances based on frame timing
         * When false: animation is paused on current frame
         *
         * Can be toggled via messages or component properties
        ===============================================================================
         */
        bool isPlaying;

        /**
        ===============================================================================
         * @brief Pointer to the entity manager for component access
         *
         * Provides access to:
         * - Entity iteration (GetAllEntities())
         * - Component checking (HasComponent<SpriteAnimation>())
         * - Component data access (GetComponent<SpriteAnimation>())
         *
         * @warning Must be set via SetEntityManager() before use
        ===============================================================================
         */
		Framework::EntityManager* entityManager;
        float max_static_threshold;
        float default_zero;
        int anim_current_frame;
        int anim_frame_mod;
	};

} // namespace Framework