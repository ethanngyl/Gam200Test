#pragma once

struct lua_State;

namespace Framework::ScriptAPIRegistry {

using ApiProvider = void(*)(lua_State* L);

void ClearProviders();
void RegisterEngineProvider(ApiProvider provider);
void RegisterGameProvider(ApiProvider provider);
void RegisterAll(lua_State* L);

} // namespace Framework::ScriptAPIRegistry
