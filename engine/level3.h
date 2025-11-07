/**
===============================================================================
 File:           level3.h
 Author:         PADILLA CARL JAMESON Z.
 Email:          c.padilla@digipen.edu
 Date:           2025/11/07
 Contribution:   100%
 ------------------------------------------------------------------------------

  Design notes:
  Level 3 state interface for the game state machine. Declares lifecycle
  entry points:
    - level3_Load() / level3_Unload(): lightweight asset hooks
    - level3_Initialize() / level3_Free(): create/destroy level entities & state
    - level3_Update(): per-frame logic (turn flow, input, AI ticks, camera)
    - level3_Draw(): reserved for explicit render calls if needed

  Header remains minimal; implementation owns wiring of subsystems and all
  per-level configuration.
===============================================================================
 */

#pragma once
#include "Precompiled.h"

// ============================================================================
// LEVEL 3 STATE FUNCTIONS
// ============================================================================

void level3_Load();
void level3_Initialize();
void level3_Update();
void level3_Draw();
void level3_Free();
void level3_Unload();