#include "Precompiled.h"
#include "TransitionPolicyHooks.h"

#include "GameStateManager.h"
#include "ImguiSystem.h"

namespace Framework::TransitionPolicyHooks {

void SetNextStateWithPolicy(int nextState)
{
    if (Framework::CORE) {
        auto* imgui = Framework::CORE->GetImGuiSystem();
        if (imgui && imgui->IsEnabled()) {
            g_loadAsEditorMode = true;
            LOG_INFO("GSM", "ImGui enabled - next level will load in editor mode");
        }

        if (Framework::CORE->IsPlaying()) {
            g_preservePlayingState = true;
            LOG_INFO("GSM", "Game is playing - next level will start in playing state");
        }
    }

    next = nextState;
}

} // namespace Framework::TransitionPolicyHooks
