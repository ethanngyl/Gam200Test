/*
===============================================================================
 File:          GameStateManager.h
 Author:        GE YONGQI
 Email:         yongqi.ge@digipen.edu
 Date:          2025-10-31
 Contribution:  100%
 ------------------------------------------------------------------------------
  Game State Manager (header)

  Purpose:
     Declares the GSM interface and function pointer structure used
     to control transitions between game states.

  Design notes:
     - Each state defines its own set of load, initialize, update, draw,
       free, and unload functions.
     - The GSM swaps these function pointers dynamically as the game changes state.

  Usage:
     1) Call GSM_Initialize(startState) once at startup.
     2) Call GSM_Update() each frame to refresh function pointers
        according to the current active state.


 Copyright (C) 2026 DigiPen Institute of Technology.
 Reproduction or disclosure of this file or its contents
 without the prior written consent of DigiPen Institute of
 Technology is prohibited.
===============================================================================
*/


#pragma once
#include "Precompiled.h"

 // ============================================================================
 // FUNCTION POINTER TYPE
 // ============================================================================
typedef void(*FP)(void);

// ============================================================================
// GLOBAL VARIABLES (Declared here, defined in .cpp)
// ============================================================================
extern int current, previous, next;
extern FP fpLoad, fpInitialize, fpUpdate, fpDraw, fpFree, fpUnload;

// Flag to indicate if the next level should be loaded in editor mode
// Set by ImGuiSystem::LoadLevelViaGSM, consumed by GSM_Update
extern bool g_loadAsEditorMode;

// Flag to indicate if the game was playing when transitioning to next level
// When true, the next level will start in playing state (not paused in editor)
extern bool g_preservePlayingState;

// ============================================================================
// GSM FUNCTIONS
// ============================================================================
void GSM_Initialize(int startingState);
void GSM_Update();

/**
 * @brief Set the next game state, preserving editor mode if ImGui is enabled
 * @param nextState The next game state to transition to
 * 
 * Use this function instead of directly setting 'next' to ensure
 * ImGui stays enabled during level transitions when in editor mode.
 */
void GSM_SetNextState(int nextState);