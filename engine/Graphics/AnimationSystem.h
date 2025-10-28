#pragma
#include "Precompiled.h"
#include "ECSEntityManager.h"     // ADD THIS

namespace Framework {

	class AnimationSystem : public EngineSystem {
	public:
		AnimationSystem();
		virtual ~AnimationSystem();

		// === CORE LIFECYCLE ===

		virtual void Initialize() override;
		virtual void Update(float dt) override;
		virtual void SendEngineMessage(Message* message) override;

		void SetEntityManager(Framework::EntityManager* em) { entityManager = em; }

	private:
        // === SIMPLE ANIMATION DATA ===

		// Animation frames
        std::vector<int> animationFrames;

        // How long to show each frame (in seconds)
        std::vector<float> frameDurations;

        // Current animation state
        int currentFrame;
        float frameTimer;
        bool isPlaying;

		Framework::EntityManager* entityManager; // ADD THIS
	};

}