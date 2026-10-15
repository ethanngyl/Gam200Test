/*
===============================================================================
 File:          GameScriptAPIProvider.h
 Author:        ETHAN NG
 Email:         n.ethanyongle@digipen.edu
 Date:          2026-04-05
 Contribution:  100%
 ------------------------------------------------------------------------------
 GameScriptAPIProvider Header

 Overview:
    The GameScriptAPIProvider module provides runtime functionality for the game engine.

===============================================================================
*/
#pragma once

struct lua_State;

namespace Framework::GameScriptAPIProvider {

/**
 * @brief Registers game-facing Lua API functions.
 * @param L Active Lua state to populate.
 */
void Register(lua_State* L);

} // namespace Framework::GameScriptAPIProvider
