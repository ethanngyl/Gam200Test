/*
===============================================================================
 File:          GameBootstrap.cpp
 Author:        ETHAN NG
 Email:         n.ethanyongle@digipen.edu
 Date:          2026-04-05
 Contribution:  100%
 ------------------------------------------------------------------------------
 GameBootstrap Implementation

 Overview:
    The GameBootstrap module provides runtime functionality for the game engine.

===============================================================================
*/
#include "Precompiled.h"
#include "GameBootstrap.h"

#include "EngineScriptAPIProvider.h"
#include "GameScriptAPIProvider.h"
#include "ScriptAPIRegistry.h"

namespace Framework::GameBootstrap {

/**
 * @brief Initializes script API providers used by game runtime Lua states.
 */
void Initialize()
{
    Framework::ScriptAPIRegistry::ClearProviders();
    Framework::ScriptAPIRegistry::RegisterEngineProvider(Framework::EngineScriptAPIProvider::Register);
    Framework::ScriptAPIRegistry::RegisterGameProvider(Framework::GameScriptAPIProvider::Register);

    LOG_INFO("GameBootstrap", "Script API providers initialized (engine + game)");
}

} // namespace Framework::GameBootstrap
