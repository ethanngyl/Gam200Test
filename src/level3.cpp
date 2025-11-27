/**
===============================================================================
 File:           level3.cpp
 Author:         PADILLA CARL JAMESON Z.
 Email:          c.padilla@digipen.edu
 Date:           2025/11/07
 Contribution:   100%
 ------------------------------------------------------------------------------

  Modified: 2025-11-22
  - Added pause functionality using GlobalPauseManager

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
#include "TileMapLoader.h"
#include "ImguiSystem.h"
namespace {
    Framework::Entity gPlayer{ Framework::INVALID_ENTITY };
   // Framework::Entity gEnemy{ Framework::INVALID_ENTITY };
    std::vector<Framework::Entity>gEnemies;
    bool imguiEnabled = true;  // Toggle ImGui rendering with F1
}

Framework::Entity FindPlayer(Framework::EntityManager* em) {
    // Player has Movement + CircleCollider but NOT EnemyAI
    for (Framework::Entity e : em->GetAllEntities()) {
        if (em->HasComponent<Framework::Movement>(e) &&
            em->HasComponent<Framework::CircleCollider>(e) &&
            !em->HasComponent<Framework::EnemyAI>(e)) {
            return e;
        }
    }
    return Framework::Entity{ Framework::INVALID_ENTITY };
}

std::vector<Framework::Entity> FindAllEnemies(Framework::EntityManager* em) {
    std::vector<Framework::Entity> result;
    for (Framework::Entity e : em->GetAllEntities()) {
        if (em->HasComponent<Framework::EnemyAI>(e)) {
            result.push_back(e);
        }
    }
    return result;
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

    // --- Load Animation Configuration ---
    auto* animSys = engine->GetAnimationSystem();
    if (animSys) {
        animSys->LoadAnimationConfig("assets/animations.json");
        LOG_INFO("LEVEL3", "Animation config loaded from assets/animations.json");
    } else {
        LOG_ERROR("LEVEL3", "AnimationSystem is null!");
    }

    // --- Camera: simple baseline to match other levels ---
    gfx->SetCameraPosition(glm::vec3(0.0f, 0.0f, 0.0f));
    gfx->SetCameraZoom(1.0f);

    // --- Grid config (adjust freely) ---
    //const int       gridCols = 16;
    //const int       gridRows = 20;
    const Vector2D  kStart = Vector2D(-0.6f, -0.4f);
    const Vector2D  kSpacing = Vector2D(0.1f, 0.1f);
	const Vector2D  kTileSize = Vector2D(0.1f, 0.1f);

    // Spawn grid immediately
   // spawner->SpawnGrid("wireframequad", gridCols, gridRows, kStart, kSpacing);

    bool loaded = TileMapLevelLoader::LoadLevel(
        "assets/scripts/JSON/TileMap.json",
        spawner,
        em,
        kStart,
        kSpacing,
        kTileSize
    );

    if (!loaded) {
        LOG_ERROR("LEVEL3", "Failed to load level from JSON!");
        return;
    }

    LOG_INFO("LEVEL3", "Level loaded successfully!");
    LOG_INFO("LEVEL3", "Grid: %d cols � %d rows", GetGrid().cols, GetGrid().rows);

    //// Link to global grid so movement/pathfinding can query it
    //Grid& grid = GetGrid();
    //grid.cols = gridCols;
    //grid.rows = gridRows;
    //grid.startPos = kStart;
    //grid.spacing = kSpacing;
    //grid.em = em;

    //// --- Player: spawn at grid center ---
    //const int  centerX = grid.cols / 2;
    //const int  centerY = grid.rows / 2;
    //Vector2D   playerWorldPos = TileToWorld({ centerX, centerY });
    //gPlayer = spawner->SpawnPlayer(playerWorldPos);

    ////  1. ADD PLAYER STATS HERE (Configuration) 
    //em->AddComponent<Framework::AP>(gPlayer, 5, 3);

     // ========================================================================
    // FIND PLAYER (spawned by loader)
    // ========================================================================

    gPlayer = FindPlayer(em);
    if (gPlayer.GetID() == INVALID_ENTITY) {
        LOG_ERROR("LEVEL3", "No player found in loaded level!");
        return;
    }

    LOG_INFO("LEVEL3", "Found Player (ID: %u)", gPlayer.GetID());

    // Add player stats if not already added
    //if (!em->HasComponent<AP>(gPlayer)) {
    //    em->AddComponent<AP>(gPlayer, AP{ 5 });  // 100 HP, 5 AP
    //}

    // DISABLE ImGui for debugging - isolate player + map only
    // if (engine->GetImGuiSystem()) {
    //     engine->GetImGuiSystem()->Enable();
    //     engine->GetImGuiSystem()->SetPlayerEntity(gPlayer);
    //     LOG_INFO("LEVEL3", "ImGui enabled and player entity set");
    // }
    LOG_INFO("LEVEL3", "ImGui DISABLED for debugging - only player + map active");

    // ========================================================================
    // FIND ENEMIES AND SET TARGETS - COMMENTED OUT FOR DEBUGGING
    // ========================================================================

    /*
    gEnemies = FindAllEnemies(em);
    LOG_INFO("LEVEL3", "Found %zu enemies", gEnemies.size());

    for (Entity enemy : gEnemies) {
        // Set AI target
        if (em->HasComponent<EnemyAI>(enemy)) {
            auto& ai = em->GetComponent<EnemyAI>(enemy);
            ai.targetEntity = gPlayer;  // *** CRITICAL! ***
            ai.moveDelay = 0.7f;
            LOG_INFO("LEVEL3", "Enemy %u now targeting Player %u",
                enemy.GetID(), gPlayer.GetID());
        }

        // Ensure enemy has AP
        if (!em->HasComponent<AP>(enemy)) {
            em->AddComponent<AP>(enemy, 3);  // 50 HP, 3 AP
        }
    }
    */

    // Wire controller (minimal, just what's required for movement)
    playerController->SetPlayerEntity(gPlayer);
    playerController->SetEntitySpawner(spawner);
    playerController->SetEntityManager(em);
    playerController->SetInputSystem(input);
    playerController->SetGridMovementEnabled(true);
    // --- Enemy: spawn furthest, uses A* to chase player ---
   // gEnemies = PathfindingSystem::SpawnEnemyFurthestFromPlayer(gPlayer, em, spawner);

    //  2. ADD ENEMY STATS HERE (Configuration) 
   // em->AddComponent<Framework::AP>(gEnemy, 2, 3);

    // --- Start turns on Player phase ---
    auto& turn = Turn();
    turn.phase = TurnPhase::Player;
    turn.busy = false;

   /* LOG_INFO("LEVEL3", "Grid=%dx%d, Player(%u) at (%.3f, %.3f), Enemy(%u)",
        grid.cols, grid.rows, gPlayer.GetID(), playerWorldPos.x, playerWorldPos.y, gEnemy.GetID());*/

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
    using namespace Framework;
    CoreEngine* engine = CORE;
    if (!engine) return;

    // F1 toggle disabled - ImGui completely off for debugging
    // if (engine->GetInputSystem() && engine->GetInputSystem()->IsKeyPressed(Framework::KEY_F1)) {
    //     imguiEnabled = !imguiEnabled;
    //     LOG_INFO("LEVEL3", "ImGui %s", imguiEnabled ? "ENABLED" : "DISABLED");
    // }

    // Return to main menu with KEY_5
    if (engine->GetInputSystem() && engine->GetInputSystem()->IsKeyPressed(Framework::KEY_5)) {
        next = mainMenu;
    }

    auto* pcs = engine->GetPlayerController();
    auto* pfs = engine->GetPathfindingSystem();
    if (!pcs || !pfs) return;

    // Player movement & interactions (internally handles click/arrow)
    pcs->Update(0.016f);

    // Enemy pathfinding tick - COMMENTED OUT FOR DEBUGGING
    // pfs->Update(0.016f);

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

    // DEBUGGING: All UI disabled - only rendering player + map
    // extern Framework::CoreEngine* engine;
    //
    // // Draw pause system if enabled (toggle with F1)
    // // Note: ImGui renders automatically when enabled, no Draw() call needed
    // if (imguiEnabled && engine && engine->GetPauseSystem()) {
    //     engine->GetPauseSystem()->Draw();
    // }

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

    // ImGui disabled for debugging, so no need to disable
    // if (CORE && CORE->GetImGuiSystem()) {
    //     CORE->GetImGuiSystem()->Disable();
    //     LOG_INFO("LEVEL3", "ImGui disabled");
    // }

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
    //gEnemy = Entity{ INVALID_ENTITY };
    gEnemies.clear();
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
