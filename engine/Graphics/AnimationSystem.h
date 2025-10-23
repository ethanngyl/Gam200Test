#pragma
#include "Precompiled.h"

namespace Framework {

	class AnimationSystem : public EngineSystem {
	public:
		AnimationSystem();
		virtual ~AnimationSystem();

		// === CORE LIFECYCLE ===

		virtual void Initialize() override;
		virtual void Update(float dt) override;
		virtual void SendEngineMessage(Message* message) override;

	private:
        // === SIMPLE ANIMATION DATA ===

		// Animation frames: each number is a "color code"
		// 0 = Red, 1 = Green, 2 = Blue, etc.
        std::vector<int> animationFrames;

        // How long to show each frame (in seconds)
        std::vector<float> frameDurations;

        // Current animation state
        int currentFrame;
        float frameTimer;
        bool isPlaying;

        // Demo variables
        bool demoInitialized;
	};

}