/*
===============================================================================
 File:          Level3StateHooks.cpp
 Author:        ETHAN NG
 Email:         n.ethanyongle@digipen.edu
 Date:          2026-04-05
 Contribution:  100%
 ------------------------------------------------------------------------------
 Level3StateHooks Implementation

 Overview:
    The Level3StateHooks module provides runtime functionality for the game engine.

===============================================================================
*/
#include "Precompiled.h"
#include "Level3StateHooks.h"

#include "LevelLoader.h"
#include "TimeConstants.h"
#include "TagHelper.h"
#include "PlayerManager.h"
#include "Pathfinding.h"
#include "ScriptSystem.h"

extern Framework::CoreEngine* engine;

namespace Framework::Level3StateHooks {

/**
 * @brief Configures engine systems and player bindings for Level 3.
 */
void OnInitialize()
{
    LOG_INFO("GSM", "Level3 ready (Lua-scripted)");

    if (!::engine) {
        return;
    }

    auto* em = ::engine->GetEntityManager();
    auto* pc = ::engine->GetPlayerController();
    auto* spawner = ::engine->GetSpawner();
    auto* input = ::engine->GetInputSystem();

    if (em && pc && spawner && input) {
        Framework::Entity player = Framework::FindFirstByTag(em, "Player");
        if (player.GetID() != Framework::INVALID_ENTITY) {
            pc->SetPlayerEntity(player);
            pc->SetEntitySpawner(spawner);
            pc->SetEntityManager(em);
            pc->SetInputSystem(input);
            LOG_INFO("GSM", "Player controller configured for entity %u", player.GetID());
        }
    }
}

/**
 * @brief Handles Level 3 update flow including hot-reload and system updates.
 */
void OnUpdate()
{
    if (::engine && ::engine->GetInputSystem()) {
        auto* input = ::engine->GetInputSystem();
        if (input->IsKeyPressed(Framework::KEY_F9)) {
            LOG_INFO("GSM", "F9 pressed - Hot reloading Level3...");
            Framework::LevelLoader::GetInstance().ReloadCurrentLevel();
        }
    }

    if (::engine) {
        if (!::engine->IsPlaying()) {
            if (auto* gfx = ::engine->GetGraphicsSystem()) {
                gfx->ClearFollowTarget();
            }
            return;
        }

        auto* pcs = ::engine->GetPlayerController();
        if (pcs) {
            pcs->Update(Framework::Time::FIXED_DT_F);
        }

        auto* pfs = ::engine->GetPathfindingSystem();
        if (pfs) {
            pfs->Update(Framework::Time::FIXED_DT_F);
        }
    }

    Framework::LevelLoader::GetInstance().UpdateCurrentLevel(
        Framework::Time::FIXED_DT_F
    );
}

/**
 * @brief Performs full Level 3 cleanup including script OnDestroy calls.
 */
void OnFree()
{
    LOG_INFO("GSM", "Cleaning up Level3...");

    // Reset all C++ static state that persists across level transitions
    // (EnemyTurnState, TurnState, sharedIntStore)
    Framework::LevelLoader::ResetCppLevelState();

    // Properly call OnDestroy on all entity scripts before destroying them.
    // ClearAllEntities only calls component destructors (closes lua_State)
    // but does NOT invoke the Lua OnDestroy function. We need to do that
    // here so enemy tile-occupancy, health bars, etc. are cleaned up.
    if (::engine) {
        if (auto* ss = ::engine->GetScriptSystem()) {
            auto* em = ::engine->GetEntityManager();
            if (em) {
                auto entities = em->GetAllEntities();
                for (auto entity : entities) {
                    if (em->HasComponent<ScriptComponent>(entity)) {
                        auto& script = em->GetComponent<ScriptComponent>(entity);
                        if (script.hasOnDestroy && script.L) {
                            lua_getglobal(script.L, "OnDestroy");
                            if (lua_isfunction(script.L, -1)) {
                                if (lua_pcall(script.L, 0, 0, 0) != LUA_OK) {
                                    lua_pop(script.L, 1);
                                }
                            } else {
                                lua_pop(script.L, 1);
                            }
                        }
                    }
                }
            }
        }
    }

    Framework::LevelLoader::GetInstance().ResetLuaState();

    if (!::engine) {
        return;
    }

    if (auto* pcs = ::engine->GetPlayerController()) {
        pcs->ResetGridState();
        pcs->SetGridMovementEnabled(false);
    }

    if (auto* gfx = ::engine->GetGraphicsSystem()) {
        gfx->ClearFollowTarget();
    }

    if (auto* em = ::engine->GetEntityManager()) {
        size_t count = em->GetAllEntities().size();
        LOG_INFO("GSM", "Final cleanup: Clearing %zu remaining entities...", count);
        em->ClearAllEntities();
        LOG_INFO("GSM", "All entities cleared. Remaining: %zu", em->GetAllEntities().size());
    }
}

} // namespace Framework::Level3StateHooks
