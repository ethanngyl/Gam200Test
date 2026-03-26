/**
===============================================================================
 File:          GameStateManager.cpp
 Author:        Padilla Carl Jameson Z.
 Email:         c.padilla@digipen.edu
 Date:          2025-11-30
 Contribution:  100%
 ------------------------------------------------------------------------------

 GAME STATE MANAGER - State Lifecycle & Hot Reload

 Brief:
    Manages game state transitions and lifecycle for all game levels and menus.
    Supports Lua-scripted states with hot reload capability via F9 key. Each
    state defines load, initialize, update, draw, free, and unload function
    pointers. Integrates with LevelLoader for Lua script management and handles
    proper cleanup order (Lua first, then C++ systems). Uses fixed delta time
    for consistent gameplay updates across all states.

 Usage:
    // Register a new game state
    GSM::AddState("MainMenu", &MainMenuLoad, &MainMenuInit,
                  &MainMenuUpdate, &MainMenuDraw,
                  &MainMenuFree, &MainMenuUnload);

    // Transition to a state
    GSM::SetNextState("MainMenu");

    // Hot reload current Lua state (F9)
    if (Input::IsKeyTriggered(KEY_F9)) {
        GSM::ReloadCurrentState();
    }


 Copyright (C) 2026 DigiPen Institute of Technology.
 Reproduction or disclosure of this file or its contents
 without the prior written consent of DigiPen Institute of
 Technology is prohibited.
===============================================================================
*/


#include "Precompiled.h"
#include "GameRuntime/GameStateRuntimeFacade.h"
#include "TimeConstants.h"

// ============================================================================
// GLOBAL VARIABLE DEFINITIONS
// ============================================================================
int current = 0;
int previous = 0;
int next = 0;

FP fpLoad = nullptr;
FP fpInitialize = nullptr;
FP fpUpdate = nullptr;
FP fpDraw = nullptr;
FP fpFree = nullptr;
FP fpUnload = nullptr;

// Flag to indicate if the next level should be loaded in editor mode
bool g_loadAsEditorMode = false;

// Flag to indicate if the game was playing when transitioning to next level
bool g_preservePlayingState = false;

// ============================================================================
// GSM FUNCTIONS
// ============================================================================

/** @brief Initializes the game state manager with a starting state. */
void GSM_Initialize(int startingState)
{
    current = previous = next = startingState;

    const bool mappingOk = Framework::GameStateRuntimeFacade::ValidateScriptMappingsAtStartup();
    LOG_WARN("GSM", "[PHASE1] State registry validation result: %s",
        mappingOk ? "PASS" : "FAIL");
    std::cout << "[GSM][PHASE1] State registry validation result: "
        << (mappingOk ? "PASS" : "FAIL") << std::endl;

    LOG_INFO("GSM", "Game State Manager initialized with state: %d", startingState);
    LOG_INFO("GSM", "Using LUA-SCRIPTED MainMenu (Pure Lua mode)");
    LOG_INFO("GSM", "Fixed DT: %.4f seconds", Framework::Time::FIXED_DT);
    LOG_INFO("GSM", "Press F9 to hot reload Lua scripts");
}

/** @brief Updates function pointers based on current game state. */
void GSM_Update()
{
    LOG_INFO("GSM", "Updating function pointers for state: %d", current);
    Framework::GameStateRuntimeFacade::ConfigureStateCallbacks(current);
}

/**
 * @brief Set the next game state, preserving editor mode if ImGui is enabled
 */
void GSM_SetNextState(int nextState) {
    Framework::GameStateRuntimeFacade::SetNextStateWithPolicy(nextState);
}