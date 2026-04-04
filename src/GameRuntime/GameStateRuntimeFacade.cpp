/*
===============================================================================
 File:          GameStateRuntimeFacade.cpp
 Author:        ETHAN NG
 Email:         n.ethanyongle@digipen.edu
 Date:          2026-04-05
 Contribution:  100%
 ------------------------------------------------------------------------------
 GameStateRuntimeFacade Implementation

 Overview:
    The GameStateRuntimeFacade module provides runtime functionality for the game engine.

===============================================================================
*/
#include "Precompiled.h"
#include "GameStateRuntimeFacade.h"

#include "StateDispatchTable.h"
#include "StateScriptRegistry.h"
#include "TransitionPolicyHooks.h"

namespace Framework::GameStateRuntimeFacade {

/**
 * @brief Validates required state-to-script mappings during startup.
 * @return True when all required mappings are valid; otherwise false.
 */
bool ValidateScriptMappingsAtStartup()
{
    return Framework::StateScriptRegistry::ValidateScriptMappingsAtStartup();
}

/**
 * @brief Configures runtime callbacks for the specified game state.
 * @param state State identifier to configure in the dispatch table.
 */
void ConfigureStateCallbacks(int state)
{
    Framework::StateDispatchTable::ConfigureState(state);
}

/**
 * @brief Applies transition policy and sets the next state.
 * @param nextState State identifier to transition into.
 */
void SetNextStateWithPolicy(int nextState)
{
    Framework::TransitionPolicyHooks::SetNextStateWithPolicy(nextState);
}

} // namespace Framework::GameStateRuntimeFacade
