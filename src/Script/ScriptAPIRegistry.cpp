/*
===============================================================================
 File:          ScriptAPIRegistry.cpp
 Author:        ETHAN NG
 Email:         n.ethanyongle@digipen.edu
 Date:          2026-04-05
 Contribution:  100%
 ------------------------------------------------------------------------------
 ScriptAPIRegistry Implementation

 Overview:
    The ScriptAPIRegistry module provides runtime functionality for the game engine.

===============================================================================
*/
#include "Precompiled.h"
#include "ScriptAPIRegistry.h"

#include <vector>

namespace {
    std::vector<Framework::ScriptAPIRegistry::ApiProvider> g_engineProviders;
    std::vector<Framework::ScriptAPIRegistry::ApiProvider> g_gameProviders;
}

namespace Framework::ScriptAPIRegistry {

/**
 * @brief Clears currently registered engine and game provider lists.
 */
void ClearProviders()
{
    g_engineProviders.clear();
    g_gameProviders.clear();
}

/**
 * @brief Adds an engine provider callback if valid.
 * @param provider Provider callback.
 */
void RegisterEngineProvider(ApiProvider provider)
{
    if (provider) {
        g_engineProviders.push_back(provider);
    }
}

/**
 * @brief Adds a game provider callback if valid.
 * @param provider Provider callback.
 */
void RegisterGameProvider(ApiProvider provider)
{
    if (provider) {
        g_gameProviders.push_back(provider);
    }
}

/**
 * @brief Invokes all registered providers against the given Lua state.
 * @param L Active Lua state.
 */
void RegisterAll(lua_State* L)
{
    for (auto provider : g_engineProviders) {
        provider(L);
    }
    for (auto provider : g_gameProviders) {
        provider(L);
    }
}

} // namespace Framework::ScriptAPIRegistry
