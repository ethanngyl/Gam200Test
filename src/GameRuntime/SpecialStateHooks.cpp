/*
===============================================================================
 File:          SpecialStateHooks.cpp
 Author:        ETHAN NG
 Email:         n.ethanyongle@digipen.edu
 Date:          2026-04-05
 Contribution:  100%
 ------------------------------------------------------------------------------
 SpecialStateHooks Implementation

 Overview:
    The SpecialStateHooks module provides runtime functionality for the game engine.

===============================================================================
*/
#include "Precompiled.h"
#include "SpecialStateHooks.h"

#include "GameStateManager.h"

namespace Framework::SpecialStateHooks {

/**
 * @brief Configures restart-state logging and callbacks.
 */
void ConfigureRestartState()
{
    LOG_INFO("GSM", "  -> Restart state");
}

/**
 * @brief Configures quit-state logging and callbacks.
 */
void ConfigureQuitState()
{
    LOG_INFO("GSM", "  -> Quit state");
}

/**
 * @brief Applies null callbacks for unsupported state ids.
 * @param state Unsupported state identifier.
 */
void ConfigureUnknownState(int state)
{
    LOG_ERROR("GSM", "  -> Unknown state: %d", state);
    fpLoad = nullptr;
    fpInitialize = nullptr;
    fpUpdate = nullptr;
    fpDraw = nullptr;
    fpFree = nullptr;
    fpUnload = nullptr;
}

} // namespace Framework::SpecialStateHooks
