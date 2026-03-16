#pragma once

namespace Framework::SpecialStateHooks {

void ConfigureRestartState();
void ConfigureQuitState();
void ConfigureUnknownState(int state);

} // namespace Framework::SpecialStateHooks
