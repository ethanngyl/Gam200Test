#pragma once

namespace Framework::StateScriptRegistry {

// Returns resolved Lua script path for a state id, or an empty string when unmapped.
const char* GetLevelScript(int state);

// Returns a stable state name for logs/UI, using registry name when available.
const char* GetStateName(int state);

// Validates required scripted states are mapped at startup.
bool ValidateScriptMappingsAtStartup();

} // namespace Framework::StateScriptRegistry
