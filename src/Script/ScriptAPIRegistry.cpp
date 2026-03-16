#include "Precompiled.h"
#include "ScriptAPIRegistry.h"

#include <vector>

namespace {
    std::vector<Framework::ScriptAPIRegistry::ApiProvider> g_engineProviders;
    std::vector<Framework::ScriptAPIRegistry::ApiProvider> g_gameProviders;
}

namespace Framework::ScriptAPIRegistry {

void ClearProviders()
{
    g_engineProviders.clear();
    g_gameProviders.clear();
}

void RegisterEngineProvider(ApiProvider provider)
{
    if (provider) {
        g_engineProviders.push_back(provider);
    }
}

void RegisterGameProvider(ApiProvider provider)
{
    if (provider) {
        g_gameProviders.push_back(provider);
    }
}

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
