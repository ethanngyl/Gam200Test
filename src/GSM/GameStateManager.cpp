/*
===============================================================================
 File:          GameStateManager.cpp
 Author:        GE YONGQI
 Email:         yongqi.ge@digipen.edu
 Date:          2025-10-31
 Contribution:  100%
 ------------------------------------------------------------------------------
  Game State Manager (implementation)

  Responsibilities:
     - Maintains current, previous, and next game states
     - Maps each state to its respective function pointer set
     - Handles GSM_Initialize() and GSM_Update() logic

  Design notes:
     - Supports modular state transitions (main menu, levels, quit, etc.)
     - Logs all transitions for debugging via Log system
     - Function pointers are reset on invalid or undefined states
===============================================================================
*/

#include "Precompiled.h"
#include "level1.h"
#include "level2.h"
#include "level3.h"
#include "LevelLoader.h"

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
    LOG_INFO("GSM", "Using LUA-SCRIPTED MainMenu (Pure Lua mode)");
}

void GSM_Update()
{
    LOG_INFO("GSM", "Updating function pointers for state: %d", current);

    switch (current)
    {
    case mainMenu:
        LOG_INFO("GSM", "Main Menu state (Lua-scripted)");

        // ====================================================================
        // LOAD
        // ====================================================================
        fpLoad = []() {
            LOG_INFO("GSM", "Loading MainMenu Lua script...");

            // Get reference outside of nested calls
            auto& loader = Framework::LevelLoader::GetInstance();
            bool success = loader.LoadLevel("assets/scripts/MainMenuLevel.lua");

            if (!success) {
                LOG_ERROR("GSM", "CRITICAL: Failed to load MainMenu Lua script!");
                LOG_ERROR("GSM", "Check: assets/scripts/MainMenuLevel.lua exists");
                next = GS_QUIT;
            }
            else {
                LOG_INFO("GSM", "MainMenu Lua script loaded successfully");
            }
            };

        // ====================================================================
        // INITIALIZE
        // ====================================================================
        fpInitialize = []() {
            LOG_INFO("GSM", "MainMenu ready (Lua-scripted)");
            };

        // ====================================================================
        // UPDATE
        // ====================================================================
        fpUpdate = []() {
            // Get engine pointer
            extern Framework::CoreEngine* engine;

            // Call Lua OnUpdate(dt)
            Framework::LevelLoader::GetInstance().UpdateCurrentLevel(0.016f);

            };

        // ====================================================================
        // DRAW
        // ====================================================================
        fpDraw = []() {
            Framework::LevelLoader::GetInstance().DrawCurrentLevel();
            };

        // ====================================================================
        // FREE
        // ====================================================================
        fpFree = []() {
            LOG_INFO("GSM", "Cleaning up MainMenu Lua script...");
            Framework::LevelLoader::GetInstance().UnloadCurrentLevel();
            };

        // ====================================================================
        // UNLOAD
        // ====================================================================
        fpUnload = []() {
            LOG_INFO("GSM", "MainMenu Lua script unloaded");
            };
        break;

    case Level_select:
        LOG_INFO("GSM", "Level_select state (Lua-scripted)");

        // ====================================================================
        // LOAD
        // ====================================================================
        fpLoad = []() {
            LOG_INFO("GSM", "Loading LevelSelect Lua script...");

            // Get reference outside of nested calls
            auto& loader = Framework::LevelLoader::GetInstance();
            bool success = loader.LoadLevel("assets/scripts/LevelSelectLevel.lua");

            if (!success) {
                LOG_ERROR("GSM", "CRITICAL: Failed to load LevelSelect Lua script!");
                LOG_ERROR("GSM", "Check: assets/scripts/LevelSelectLevel.lua exists");
                next = GS_QUIT;
            }
            else {
                LOG_INFO("GSM", "LevelSelect Lua script loaded successfully");
            }
            };

        // ====================================================================
        // INITIALIZE
        // ====================================================================
        fpInitialize = []() {
            LOG_INFO("GSM", "LevelSelect ready (Lua-scripted)");
            };

        // ====================================================================
        // UPDATE
        // ====================================================================
        fpUpdate = []() {
            // Call Lua OnUpdate(dt)
            Framework::LevelLoader::GetInstance().UpdateCurrentLevel(0.016f);
            };

        // ====================================================================
        // DRAW
        // ====================================================================
        fpDraw = []() {
            Framework::LevelLoader::GetInstance().DrawCurrentLevel();
            };

        // ====================================================================
        // FREE
        // ====================================================================
        fpFree = []() {
            LOG_INFO("GSM", "Cleaning up LevelSelect Lua script...");
            Framework::LevelLoader::GetInstance().UnloadCurrentLevel();
            };

        // ====================================================================
        // UNLOAD
        // ====================================================================
        fpUnload = []() {
            LOG_INFO("GSM", "LevelSelect Lua script unloaded");
            };
        break;

    case LEVEL_1:
        LOG_INFO("GSM", "Level 1 state");
        fpLoad = level1_Load;
        fpInitialize = level1_Initialize;
        fpUpdate = level1_Update;
        fpDraw = level1_Draw;
        fpFree = level1_Free;
        fpUnload = level1_Unload;
        break;

    case LEVEL_2:
        LOG_INFO("GSM", "Level 2 state");
        fpLoad = level2_Load;
        fpInitialize = level2_Initialize;
        fpUpdate = level2_Update;
        fpDraw = level2_Draw;
        fpFree = level2_Free;
        fpUnload = level2_Unload;
        break;

    case LEVEL_3:
        LOG_INFO("GSM", "Level 3 state");
        fpLoad = level3_Load;
        fpInitialize = level3_Initialize;
        fpUpdate = level3_Update;
        fpDraw = level3_Draw;
        fpFree = level3_Free;
        fpUnload = level3_Unload;
        break;

    case GS_RESTART:
        LOG_INFO("GSM", "Restart state");
        break;

    case GS_QUIT:
        LOG_INFO("GSM", "Quit state");
        break;

    default:
        LOG_ERROR("GSM", "Unknown state: %d", current);
        fpLoad = nullptr;
        fpInitialize = nullptr;
        fpUpdate = nullptr;
        fpDraw = nullptr;
        fpFree = nullptr;
        fpUnload = nullptr;
        break;
    }
}