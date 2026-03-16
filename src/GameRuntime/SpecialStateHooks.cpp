#include "Precompiled.h"
#include "SpecialStateHooks.h"

#include "GameStateManager.h"

namespace Framework::SpecialStateHooks {

void ConfigureRestartState()
{
    LOG_INFO("GSM", "  -> Restart state");
}

void ConfigureQuitState()
{
    LOG_INFO("GSM", "  -> Quit state");
}

void ConfigureUnknownState(int state)
{
    LOG_ERROR("GSM", "  -> Unknown state: %d", state);
    fpLoad = nullptr;
    fpInitialize = nullptr;
    fpUpdate = nullptr;
    fpDraw = nullptr;
    fpFree = nullptr;
    fpUnload = nullptr;
}

} // namespace Framework::SpecialStateHooks
