/*
===============================================================================
 File:          LevelSelect.h
 Author:        GE YONGQI
 Email:         yongqi.ge@digipen.edu
 Date:          2025-10-31
 Contribution:  100%
 ------------------------------------------------------------------------------
  LevelSelect

  Responsibilities:
     - Declares lifecycle functions for the LevelSelect
     - Used by the Game State Manager (GSM) for transitions

  Notes:
     - Menu buttons are managed globally through the UISystem
     - All six standard GSM lifecycle functions are defined elsewhere
===============================================================================
*/


#pragma once
#include "Precompiled.h"


// ============================================================================
// MAIN MENU STATE FUNCTIONS
// ============================================================================

void level_select_Load();
void level_select_Initialize();
void level_select_Update();
void level_select_Draw();
void level_select_Free();
void level_select_Unload();