/*
===============================================================================
 File:          level1.h
 Author:        GE YONGQI
 Email:         yongqi.ge@digipen.edu
 Date:          2025-10-31
 Contribution:  100%
 ------------------------------------------------------------------------------
  Level 1 state (header)

  Responsibilities:
     - Declares lifecycle functions for Level 1 gameplay state
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

void level1_Load();
void level1_Initialize();
void level1_Update();
void level1_Draw();
void level1_Free();
void level1_Unload();