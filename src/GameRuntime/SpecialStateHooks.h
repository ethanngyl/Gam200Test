/*
===============================================================================
 File:          SpecialStateHooks.h
 Author:        ETHAN NG
 Email:         n.ethanyongle@digipen.edu
 Date:          2026-04-05
 Contribution:  100%
 ------------------------------------------------------------------------------
 SpecialStateHooks Header

 Overview:
    The SpecialStateHooks module provides runtime functionality for the game engine.

===============================================================================
*/
#pragma once

namespace Framework::SpecialStateHooks {

/**
 * @brief Configures handlers for restart state behavior.
 */
void ConfigureRestartState();

/**
 * @brief Configures handlers for quit state behavior.
 */
void ConfigureQuitState();

/**
 * @brief Configures safe fallback handlers for unknown states.
 * @param state Unknown state identifier.
 */
void ConfigureUnknownState(int state);

} // namespace Framework::SpecialStateHooks
