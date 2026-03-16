#include "Precompiled.h"
#include "GameStateRuntimeFacade.h"

#include "StateDispatchTable.h"
#include "StateScriptRegistry.h"
#include "TransitionPolicyHooks.h"

namespace Framework::GameStateRuntimeFacade {

bool ValidateScriptMappingsAtStartup()
{
    return Framework::StateScriptRegistry::ValidateScriptMappingsAtStartup();
}

void ConfigureStateCallbacks(int state)
{
    Framework::StateDispatchTable::ConfigureState(state);
}

void SetNextStateWithPolicy(int nextState)
{
    Framework::TransitionPolicyHooks::SetNextStateWithPolicy(nextState);
}

} // namespace Framework::GameStateRuntimeFacade
