/**
===============================================================================
 File:           GameStateManager.cpp
 Description:    Game state manager implementation
===============================================================================
 */

#include "Precompiled.h"
#include "GameStateManager.h"
#include "MainMenu.h"
#include"level1.h"

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

// ============================================================================
// GSM FUNCTIONS
// ============================================================================

void GSM_Initialize(int startingState)
{
    current = previous = next = startingState;
    LOG_INFO("GSM", "Game State Manager initialized with state: %d", startingState);
}

void GSM_Update()
{
    LOG_INFO("GSM", "Updating function pointers for state: %d", current);

    switch (current)
    {
    case mainMenu:
        LOG_INFO("GSM", "  -> Main Menu state");
        fpLoad = mainMenu_Load;
        fpInitialize = mainMenu_Initialize;
        fpUpdate = mainMenu_Update;
        fpDraw = mainMenu_Draw;
        fpFree = mainMenu_Free;
        fpUnload = mainMenu_Unload;
        break;

    case LEVEL_1:
        LOG_INFO("GSM", "  -> Level 1 state (NOT IMPLEMENTED)");
        fpLoad = level1_Load;
        fpInitialize = level1_Initialize;
        fpUpdate = level1_Update;
        fpDraw = level1_Draw;
        fpFree = level1_Free;
        fpUnload = level1_Unload;
        break;

    case LEVEL_2:
        LOG_INFO("GSM", "  -> Level 2 state (NOT IMPLEMENTED)");
        fpLoad = []() { LOG_INFO("LEVEL2", "Load - TODO"); };
        fpInitialize = []() { LOG_INFO("LEVEL2", "Initialize - TODO"); };
        fpUpdate = []() {
            next = mainMenu;
            };
        fpDraw = []() {};
        fpFree = []() {};
        fpUnload = []() {};
        break;

    case LEVEL_3:
        LOG_INFO("GSM", "  -> Level 3 state (NOT IMPLEMENTED)");
        fpLoad = []() { LOG_INFO("LEVEL3", "Load - TODO"); };
        fpInitialize = []() { LOG_INFO("LEVEL3", "Initialize - TODO"); };
        fpUpdate = []() {
            next = mainMenu;
            };
        fpDraw = []() {};
        fpFree = []() {};
        fpUnload = []() {};
        break;

    case GS_RESTART:
        LOG_INFO("GSM", "  -> Restart state");
        break;

    case GS_QUIT:
        LOG_INFO("GSM", "  -> Quit state");
        break;

    default:
        LOG_ERROR("GSM", "  -> Unknown state: %d", current);
        fpLoad = nullptr;
        fpInitialize = nullptr;
        fpUpdate = nullptr;
        fpDraw = nullptr;
        fpFree = nullptr;
        fpUnload = nullptr;
        break;
    }
}