#include "Precompiled.h"
#include "level3.h"

#include "Core.h"
#include "EntitySpawner.h"
#include "PlayerManager.h"
#include "Pathfinding.h"
#include "GraphicsSystemV2.h"
#include "Turn.h"

namespace {
    Framework::Entity gPlayer{ Framework::INVALID_ENTITY };
    Framework::Entity gEnemy{ Framework::INVALID_ENTITY };
}

void level3_Load()
{
    LOG_INFO("LEVEL3", "Load");
}

void level3_Initialize()
{
    using namespace Framework;

    LOG_INFO("LEVEL3", "Initialize");

    CoreEngine* engine = CORE;
    if (!engine) { LOG_ERROR("LEVEL3", "CORE is null!"); return; }

    engine->SetPlaying(false);

    auto* gfx = engine->GetGraphicsSystem();
    auto* spawner = engine->GetSpawner();
    auto* playerController = engine->GetPlayerController();
    auto* em = engine->GetEntityManager();
    auto* pathfinding = engine->GetPathfindingSystem();
    auto* input = engine->GetInputSystem();

    if (!gfx || !spawner || !playerController || !em || !pathfinding || !input) {
        LOG_ERROR("LEVEL3", "Missing systems (gfx/spawner/pcs/em/pathfinding/input)");
        return;
    }

    // --- Camera: simple baseline to match other levels ---
    gfx->SetCameraPosition(glm::vec3(0.0f, 0.0f, 0.0f));
    gfx->SetCameraZoom(1.0f);

    // --- Grid config (adjust freely) ---
    const int       gridCols = 16;
    const int       gridRows = 20;
    const Vector2D  kStart = Vector2D(-0.6f, -0.4f);
    const Vector2D  kSpacing = Vector2D(0.1f, 0.1f);

    // Spawn grid immediately
    spawner->SpawnGrid("wireframequad", gridCols, gridRows, kStart, kSpacing);

    // Link to global grid so movement/pathfinding can query it
    Grid& grid = GetGrid();
    grid.cols = gridCols;
    grid.rows = gridRows;
    grid.startPos = kStart;
    grid.spacing = kSpacing;
    grid.em = em;

    // --- Player: spawn at grid center ---
    const int  centerX = grid.cols / 2;
    const int  centerY = grid.rows / 2;
    Vector2D   playerWorldPos = TileToWorld({ centerX, centerY });
    gPlayer = spawner->SpawnPlayer(playerWorldPos);

    // Wire controller (minimal, just what's required for movement)
    playerController->SetPlayerEntity(gPlayer);
    playerController->SetEntitySpawner(spawner);
    playerController->SetEntityManager(em);
    playerController->SetInputSystem(input);

    // --- Enemy: spawn furthest, uses A* to chase player ---
    gEnemy = PathfindingSystem::SpawnEnemyFurthestFromPlayer(gPlayer, em, spawner);

    // --- Start turns on Player phase ---
    auto& turn = Turn();
    turn.phase = TurnPhase::Player;
    turn.busy = false;

    LOG_INFO("LEVEL3", "Grid=%dx%d, Player(%u) at (%.3f, %.3f), Enemy(%u)",
        grid.cols, grid.rows, gPlayer.GetID(), playerWorldPos.x, playerWorldPos.y, gEnemy.GetID());
}

void level3_Update()
{
    if (engine && engine->GetInputSystem() &&
        engine->GetInputSystem()->IsKeyPressed(Framework::KEY_5))
    {
        next = mainMenu;
    }

    using namespace Framework;
    CoreEngine* engine = CORE;
    if (!engine) return;

    auto* pcs = engine->GetPlayerController();
    auto* pfs = engine->GetPathfindingSystem();
    if (!pcs || !pfs) return;

    // Player movement & interactions (internally handles click/arrow)
    pcs->Update(0.016f);

    // Enemy pathfinding tick
    pfs->Update(0.016f);

    // Camera follow (keeps existing zoom/offset)
    if (auto* gfx = engine->GetGraphicsSystem()) {
        engine->SetPlaying(true);
        gfx->SetFollowTarget(gPlayer);
    }
}

void level3_Draw() {}

void level3_Free()
{
    using namespace Framework;
    if (auto* gfx = CORE ? CORE->GetGraphicsSystem() : nullptr) {
        gfx->ClearFollowTarget();
    }
    if (CORE && CORE->GetEntityManager()) {
        auto ents = CORE->GetEntityManager()->GetAllEntities();
        for (auto e : ents) CORE->GetEntityManager()->DestroyEntity(e);
    }
    gPlayer = Entity{ INVALID_ENTITY };
    gEnemy = Entity{ INVALID_ENTITY };
}

void level3_Unload()
{
    LOG_INFO("LEVEL3", "Unload");
}
