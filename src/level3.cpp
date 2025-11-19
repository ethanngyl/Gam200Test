/**
===============================================================================
 File:           level3.cpp
 Author:         PADILLA CARL JAMESON Z.
 Email:          c.padilla@digipen.edu
 Date:           2025/11/07
 Contribution:   100%
 ------------------------------------------------------------------------------

  Design notes:
  Implements Level 3 setup, update loop, and teardown.

  Initialization:
  - Grabs core subsystems (Graphics, Spawner, PlayerController, EntityManager,
    Pathfinding, Input), configures a tile grid (cols/rows, origin, spacing).
  - Spawns player at grid center and enemy at a furthest walkable tile to chase
    via A*; sets initial turn phase to Player.
  - Sets baseline camera transform and enables follow target (player).

  Per-frame Update:
  - Delegates input & movement to PlayerControllerSystem (click/arrow step).
  - Ticks PathfindingSystem for enemy pursuit along computed paths.
  - Maintains camera follow while engine is playing.

  Teardown:
  - Clears camera follow, destroys all level entities, and resets cached entity
    handles to invalid values to avoid dangling references.

  Notes:
  - Uses grid utilities to keep movement discrete and turn-based.
  - Designed to be self-contained so other levels can reuse engine subsystems
    with different configurations.
===============================================================================
 */


#include "Precompiled.h"
#include "level3.h"

#include "Core.h"
#include "EntitySpawner.h"
#include "PlayerManager.h"
#include "Pathfinding.h"
#include "GraphicsSystemV2.h"
#include "Turn.h"
#include "Pause/Pause.h"

namespace {
    Framework::Entity gPlayer{ Framework::INVALID_ENTITY };
    Framework::Entity gEnemy{ Framework::INVALID_ENTITY };
}

/**
 * @brief Loads Level 3 state
 *
 * Logs the load phase and prepares for initialization.
 * No heavy setup or entity creation occurs here.
 */
void level3_Load()
{
    LOG_INFO("LEVEL3", "Load");
}

/**
 * @brief Initializes Level 3
 *
 * Sets up all main systems and objects:
 * - Initializes grid layout and links it globally
 * - Spawns player and enemy (A* pathfinding)
 * - Sets camera position and turn phase
 */
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

    //  1. ADD PLAYER STATS HERE (Configuration) 
    em->AddComponent<Framework::AP>(gPlayer, 5, 3);

    // Wire controller (minimal, just what's required for movement)
    playerController->SetPlayerEntity(gPlayer);
    playerController->SetEntitySpawner(spawner);
    playerController->SetEntityManager(em);
    playerController->SetInputSystem(input);
    playerController->SetGridMovementEnabled(true);
    // --- Enemy: spawn furthest, uses A* to chase player ---
    gEnemy = PathfindingSystem::SpawnEnemyFurthestFromPlayer(gPlayer, em, spawner);

    //  2. ADD ENEMY STATS HERE (Configuration) 
    em->AddComponent<Framework::AP>(gEnemy, 2, 3);

    // --- Start turns on Player phase ---
    auto& turn = Turn();
    turn.phase = TurnPhase::Player;
    turn.busy = false;

    LOG_INFO("LEVEL3", "Grid=%dx%d, Player(%u) at (%.3f, %.3f), Enemy(%u)",
        grid.cols, grid.rows, gPlayer.GetID(), playerWorldPos.x, playerWorldPos.y, gEnemy.GetID());
}

/**
 * @brief Updates Level 3 every frame
 *
 * Handles per-frame logic:
 * - Processes player input and movement
 * - Updates enemy pathfinding behavior
 * - Keeps the camera centered on the player
 */
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
/**
 * @brief Draws Level 3
 *
 * Placeholder for custom render logic.
 * Usually handled by the graphics system.
 */
void level3_Draw() {

    extern Framework::CoreEngine* engine;
    if (engine && engine->GetPauseSystem()) {
        engine->GetPauseSystem()->Draw();
    }

}

/**
 * @brief Frees all Level 3 data
 *
 * Cleans up entities and resets references:
 * - Clears camera follow target
 * - Destroys all level entities
 * - Resets player and enemy IDs
 */
void level3_Free()
{
    using namespace Framework;
    if (CORE && CORE->GetPlayerController()) {
        CORE->GetPlayerController()->ResetGridState();
        CORE->GetPlayerController()->SetGridMovementEnabled(false);
    }
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

/**
 * @brief Unloads Level 3
 *
 * Logs the unload phase for debugging and profiling.
 */
void level3_Unload()
{
    LOG_INFO("LEVEL3", "Unload");
}
