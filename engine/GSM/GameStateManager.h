/**
===============================================================================
 File:           GameStateManager.h
 Description:    Game state manager with function pointers
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

// ============================================================================
// GSM FUNCTIONS
// ============================================================================
void GSM_Initialize(int startingState);
void GSM_Update();