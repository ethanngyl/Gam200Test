/*
===============================================================================
 File:          GameStateRuntimeFacade.h
 Author:        ETHAN NG
 Email:         n.ethanyongle@digipen.edu
 Date:          2026-04-05
 Contribution:  100%
 ------------------------------------------------------------------------------
 GameStateRuntimeFacade Header

 Overview:
    The GameStateRuntimeFacade module provides runtime functionality for the game engine.

===============================================================================
*/
#pragma once

namespace Framework::GameStateRuntimeFacade {

/**
 * @brief Validates required script mappings during game startup.
 * @return True when all required mappings exist; otherwise false.
 */
bool ValidateScriptMappingsAtStartup();

/**
 * @brief Configures callbacks for the provided state id.
 * @param state State identifier to configure.
 */
void ConfigureStateCallbacks(int state);

/**
 * @brief Sets the next state using transition policy rules.
 * @param nextState State identifier that should become active next.
 */
void SetNextStateWithPolicy(int nextState);

} // namespace Framework::GameStateRuntimeFacade
