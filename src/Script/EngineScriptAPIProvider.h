/*
===============================================================================
 File:          EngineScriptAPIProvider.h
 Author:        ETHAN NG
 Email:         n.ethanyongle@digipen.edu
 Date:          2026-04-05
 Contribution:  100%
 ------------------------------------------------------------------------------
 EngineScriptAPIProvider Header

 Overview:
    The EngineScriptAPIProvider module provides runtime functionality for the game engine.

===============================================================================
*/
#pragma once

struct lua_State;

namespace Framework::EngineScriptAPIProvider {

/**
 * @brief Registers engine-facing Lua API functions.
 * @param L Active Lua state to populate.
 */
void Register(lua_State* L);

} // namespace Framework::EngineScriptAPIProvider
