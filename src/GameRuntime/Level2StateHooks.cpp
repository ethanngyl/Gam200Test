/*
===============================================================================
 File:          Level2StateHooks.cpp
 Author:        ETHAN NG
 Email:         n.ethanyongle@digipen.edu
 Date:          2026-04-05
 Contribution:  100%
 ------------------------------------------------------------------------------
 Level2StateHooks Implementation

 Overview:
    The Level2StateHooks module provides runtime functionality for the game engine.

===============================================================================
*/
#include "Precompiled.h"
#include "Level2StateHooks.h"

#include "GameStateList.h"
#include "GameStateManager.h"
#include "LevelLoader.h"
#include "StateScriptRegistry.h"
#include "TimeConstants.h"

namespace Framework::Level2StateHooks {

/**
 * @brief Loads the Level 2 Lua scene using the registry path.
 */
void OnLoad()
{
    Framework::LevelLoader::GetInstance().LoadLevel(
        Framework::StateScriptRegistry::GetLevelScript(LEVEL_2),
        g_loadAsEditorMode
    );
    g_loadAsEditorMode = false;
}

/**
 * @brief Performs post-load initialization for Level 2.
 */
void OnInitialize()
{
    // LevelLoader handles init via OnInit().
}

/**
 * @brief Advances Level 2 systems by one fixed update tick.
 */
void OnUpdate()
{
    Framework::LevelLoader::GetInstance().UpdateCurrentLevel(
        Framework::Time::FIXED_DT
    );
}

/**
 * @brief Draws currently loaded Level 2 content.
 */
void OnDraw()
{
    Framework::LevelLoader::GetInstance().DrawCurrentLevel();
}

/**
 * @brief Unloads current Level 2 content from runtime systems.
 */
void OnFree()
{
    Framework::LevelLoader::GetInstance().UnloadCurrentLevel();
}

/**
 * @brief Finalizes Level 2 unload lifecycle step.
 */
void OnUnload()
{
    // Cleanup handled by LevelLoader.
}

} // namespace Framework::Level2StateHooks
