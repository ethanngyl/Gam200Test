#include "Precompiled.h"
#include "Level3StateHooks.h"

#include "LevelLoader.h"
#include "TimeConstants.h"
#include "TagHelper.h"
#include "PlayerManager.h"
#include "Pathfinding.h"

extern Framework::CoreEngine* engine;

namespace Framework::Level3StateHooks {

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

void OnFree()
{
    LOG_INFO("GSM", "Cleaning up Level3...");

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
