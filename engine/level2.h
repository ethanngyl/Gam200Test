/*
===============================================================================
 File:          level2.h
 Author:        GE YONGQI
 Email:         yongqi.ge@digipen.edu
 Date:          2025-10-31
 Contribution:  100%
 ------------------------------------------------------------------------------
  Level 2 state (header)

  Responsibilities:
     - Declares lifecycle functions for Level 2 gameplay state
     - Used by the Game State Manager (GSM)

  Notes:
     - Implements six standard GSM state functions:
       Load, Initialize, Update, Draw, Free, Unload
===============================================================================
*/


#pragma once
#include "Precompiled.h"

// ============================================================================
// MAIN MENU STATE FUNCTIONS
// ============================================================================

void level2_Load();
void level2_Initialize();
void level2_Update();
void level2_Draw();
void level2_Free();
void level2_Unload();