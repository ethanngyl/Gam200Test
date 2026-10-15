/*
===============================================================================
 File:          StateScriptRegistry.h
 Author:        ETHAN NG
 Email:         n.ethanyongle@digipen.edu
 Date:          2026-04-05
 Contribution:  100%
 ------------------------------------------------------------------------------
 StateScriptRegistry Header

 Overview:
    The StateScriptRegistry module provides runtime functionality for the game engine.

===============================================================================
*/
#pragma once

namespace Framework::StateScriptRegistry {

// Returns resolved Lua script path for a state id, or an empty string when unmapped.
/**
 * @brief Returns the resolved Lua script path for a state id.
 * @param state State identifier.
 * @return Script path string or empty string when unmapped.
 */
const char* GetLevelScript(int state);

// Returns a stable state name for logs/UI, using registry name when available.
/**
 * @brief Returns a stable state name for logs and UI.
 * @param state State identifier.
 * @return State name from registry or fallback mapping.
 */
const char* GetStateName(int state);

// Validates required scripted states are mapped at startup.
/**
 * @brief Validates required scripted state mappings at startup.
 * @return True when required mappings are valid; otherwise false.
 */
bool ValidateScriptMappingsAtStartup();

} // namespace Framework::StateScriptRegistry
