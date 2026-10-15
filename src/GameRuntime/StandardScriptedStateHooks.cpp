/*
===============================================================================
 File:          StandardScriptedStateHooks.cpp
 Author:        ETHAN NG
 Email:         n.ethanyongle@digipen.edu
 Date:          2026-04-05
 Contribution:  100%
 ------------------------------------------------------------------------------
 StandardScriptedStateHooks Implementation

 Overview:
    The StandardScriptedStateHooks module provides runtime functionality for the game engine.

===============================================================================
*/
#include "Precompiled.h"
#include "StandardScriptedStateHooks.h"

#include "GameStateList.h"
#include "GameStateManager.h"
#include "LevelLoader.h"
#include "StateScriptRegistry.h"
#include "TimeConstants.h"

extern Framework::CoreEngine* engine;

namespace {
    int g_activeStandardLuaState = mainMenu;

    /**
     * @brief Resets Lua runtime and clears remaining entities for the current state.
     */
    void ResetLuaAndClearEntities()
    {
        Framework::LevelLoader::GetInstance().ResetLuaState();

        if (::engine) {
            if (auto* em = ::engine->GetEntityManager()) {
                size_t count = em->GetAllEntities().size();
                LOG_INFO("GSM", "Final cleanup: Clearing %zu remaining entities...", count);
                em->ClearAllEntities();
                LOG_INFO("GSM", "All entities cleared. Remaining: %zu", em->GetAllEntities().size());
            }

            if (auto* gfx = ::engine->GetGraphicsSystem()) {
                gfx->ClearFollowTarget();
            }
        }
    }
}

namespace Framework::StandardScriptedStateHooks {

/**
 * @brief Sets up scripted callback pipeline for the provided state.
 * @param state State identifier that should use standard scripted hooks.
 */
void Configure(int state)
{
    g_activeStandardLuaState = state;
    LOG_INFO("GSM", "%s state (Lua-scripted)",
        Framework::StateScriptRegistry::GetStateName(g_activeStandardLuaState));

    fpLoad = []() {
        const int activeState = g_activeStandardLuaState;
        LOG_INFO("GSM", "Loading %s Lua script...",
            Framework::StateScriptRegistry::GetStateName(activeState));

        auto& loader = Framework::LevelLoader::GetInstance();
        bool success = loader.LoadLevel(
            Framework::StateScriptRegistry::GetLevelScript(activeState),
            g_loadAsEditorMode);
        g_loadAsEditorMode = false;

        if (!success) {
            LOG_ERROR("GSM", "CRITICAL: Failed to load %s Lua script!",
                Framework::StateScriptRegistry::GetStateName(activeState));
            LOG_ERROR("GSM", "Check: %s exists",
                Framework::StateScriptRegistry::GetLevelScript(activeState));
            next = GS_QUIT;
        }
        else {
            LOG_INFO("GSM", "%s Lua script loaded successfully",
                Framework::StateScriptRegistry::GetStateName(activeState));
        }
    };

    fpInitialize = []() {
        LOG_INFO("GSM", "%s ready (Lua-scripted)",
            Framework::StateScriptRegistry::GetStateName(g_activeStandardLuaState));
    };

    fpUpdate = []() {
        if (::engine && ::engine->GetInputSystem()) {
            auto* input = ::engine->GetInputSystem();
            if (input->IsKeyPressed(Framework::KEY_F9)) {
                LOG_INFO("GSM", "F9 pressed - Hot reloading %s...",
                    Framework::StateScriptRegistry::GetStateName(g_activeStandardLuaState));
                Framework::LevelLoader::GetInstance().ReloadCurrentLevel();
            }
        }

        Framework::LevelLoader::GetInstance().UpdateCurrentLevel(
            Framework::Time::FIXED_DT_F
        );
    };

    fpDraw = []() {
        // Drawing is handled in Core.cpp via DrawCurrentLevel().
    };

    fpFree = []() {
        LOG_INFO("GSM", "Cleaning up %s Lua script...",
            Framework::StateScriptRegistry::GetStateName(g_activeStandardLuaState));
        ResetLuaAndClearEntities();
    };

    fpUnload = []() {
        LOG_INFO("GSM", "%s Lua script unloaded",
            Framework::StateScriptRegistry::GetStateName(g_activeStandardLuaState));
    };
}

} // namespace Framework::StandardScriptedStateHooks
