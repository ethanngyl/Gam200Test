/*
===============================================================================
 File:          ScriptAPIRegistry.h
 Author:        ETHAN NG
 Email:         n.ethanyongle@digipen.edu
 Date:          2026-04-05
 Contribution:  100%
 ------------------------------------------------------------------------------
 ScriptAPIRegistry Header

 Overview:
    The ScriptAPIRegistry module provides runtime functionality for the game engine.

===============================================================================
*/
#pragma once

struct lua_State;

namespace Framework::ScriptAPIRegistry {

using ApiProvider = void(*)(lua_State* L);

/**
 * @brief Clears all registered engine and game API providers.
 */
void ClearProviders();

/**
 * @brief Registers an engine-level API provider callback.
 * @param provider Provider callback function.
 */
void RegisterEngineProvider(ApiProvider provider);

/**
 * @brief Registers a game-level API provider callback.
 * @param provider Provider callback function.
 */
void RegisterGameProvider(ApiProvider provider);

/**
 * @brief Invokes all registered providers to bind APIs into Lua.
 * @param L Active Lua state to receive API bindings.
 */
void RegisterAll(lua_State* L);

} // namespace Framework::ScriptAPIRegistry
