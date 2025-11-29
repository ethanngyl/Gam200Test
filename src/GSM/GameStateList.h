/*
===============================================================================
 File:          GameStateList.h
 Author:        GE YONGQI
 Email:         yongqi.ge@digipen.edu
 Date:          2025-10-31
 Contribution:  100%
 ------------------------------------------------------------------------------
  Game state enumeration list

  Purpose:
     Defines all available game states used by the Game State Manager (GSM).

  Notes:
     - Each enumerator corresponds to a unique game flow state.
     - Add new states here before extending GSM logic.
===============================================================================
*/


#pragma once

// ============================================================================
// GAME STATES ENUM
// ============================================================================
enum GS_STATES
{
    mainMenu = 0,
    settingsMenu,
    Level_select,
    LEVEL_1,
    LEVEL_2,
    LEVEL_3,
    LEVEL_END,
    TUTORIAL,
    GS_QUIT,
    GS_RESTART,
    LevelSelectionMenu,
    Copyright,
    Credits
};