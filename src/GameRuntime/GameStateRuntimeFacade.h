#pragma once

namespace Framework::GameStateRuntimeFacade {

bool ValidateScriptMappingsAtStartup();
void ConfigureStateCallbacks(int state);
void SetNextStateWithPolicy(int nextState);

} // namespace Framework::GameStateRuntimeFacade
