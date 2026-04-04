/*
===============================================================================
 File:          StandardScriptedStateHooks.h
 Author:        ETHAN NG
 Email:         n.ethanyongle@digipen.edu
 Date:          2026-04-05
 Contribution:  100%
 ------------------------------------------------------------------------------
 StandardScriptedStateHooks Header

 Overview:
    The StandardScriptedStateHooks module provides runtime functionality for the game engine.

===============================================================================
*/
#pragma once

namespace Framework::StandardScriptedStateHooks {

/**
 * @brief Configures lifecycle callbacks for a standard scripted state.
 * @param state State identifier to configure.
 */
void Configure(int state);

} // namespace Framework::StandardScriptedStateHooks
