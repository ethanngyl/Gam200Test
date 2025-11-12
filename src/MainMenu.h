/*
===============================================================================
 File:          MainMenu.h
 Author:        GE YONGQI
 Email:         yongqi.ge@digipen.edu
 Date:          2025-10-31
 Contribution:  100%
 ------------------------------------------------------------------------------
  Main menu state (header)

  Responsibilities:
     - Declares lifecycle functions for the Main Menu state
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

void mainMenu_Load();
void mainMenu_Initialize();
void mainMenu_Update();
void mainMenu_Draw();
void mainMenu_Free();
void mainMenu_Unload();