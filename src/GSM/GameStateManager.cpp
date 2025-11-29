/*
============================================================================
 File:          GameStateManager.cpp (With F9 Hot Reload + Fixed DT)
 Features:      1. Lua-scripted menu states
                2. F9 hot reload support
                3. Proper Fixed DT integration
============================================================================
*/


#include "Precompiled.h"
#include "level1.h"
#include "level2.h"
#include "level3.h"
#include "LevelLoader.h"
#include "TimeConstants.h"

// Level 3 Lua-specific includes
#include "PlayerManager.h"
#include "Turn.h"
#include "Pathfinding.h"  // EnemyAI component
#include "Component.h"    // Movement, CircleCollider components

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
    LOG_INFO("GSM", "Fixed DT: %.4f seconds", Framework::Time::FIXED_DT);
    LOG_INFO("GSM", "Press F9 to hot reload Lua scripts");
}

void GSM_Update()
{
    LOG_INFO("GSM", "Updating function pointers for state: %d", current);

    switch (current)
    {
    case mainMenu:
        LOG_INFO("GSM", "Main Menu state (Lua-scripted)");

        // ============================================================================
        // LOAD
        // ============================================================================
        fpLoad = []() {
            LOG_INFO("GSM", "Loading MainMenu Lua script...");

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

        // ============================================================================
        // INITIALIZE
        // ============================================================================
        fpInitialize = []() {
            LOG_INFO("GSM", "MainMenu ready (Lua-scripted)");
            };

        // ============================================================================
        // UPDATE
        // ============================================================================
        fpUpdate = []() {
            // Get engine pointer
            extern Framework::CoreEngine* engine;

            // F9 Hot Reload Support
            if (engine && engine->GetInputSystem()) {
                auto* input = engine->GetInputSystem();
                if (input->IsKeyPressed(Framework::KEY_F9)) {
                    LOG_INFO("GSM", "F9 pressed - Hot reloading MainMenu...");
                    Framework::LevelLoader::GetInstance().ReloadCurrentLevel();
                }
            }

            // Call Lua OnUpdate with FIXED_DT from shared constant
            Framework::LevelLoader::GetInstance().UpdateCurrentLevel(
                Framework::Time::FIXED_DT_F
            );
            };

        // ============================================================================
        // DRAW
        // ============================================================================
        fpDraw = []() {
            // Drawing is now handled directly in Core.cpp via DrawCurrentLevel()
            // This prevents double-rendering (once to viewport, once to main window)
            // No-op for Lua-based levels
            };

        // ============================================================================
        // FREE
        // ============================================================================
        fpFree = []() {
            LOG_INFO("GSM", "Cleaning up MainMenu Lua script...");

            extern Framework::CoreEngine* engine;

            // STEP 1: Reset Lua state FIRST (calls OnDestroy which may destroy entities)
            Framework::LevelLoader::GetInstance().ResetLuaState();

            // STEP 2: Clear all entities AFTER Lua cleanup (final cleanup)
            if (engine) {
                if (auto* em = engine->GetEntityManager()) {
                    size_t count = em->GetAllEntities().size();
                    LOG_INFO("GSM", "Final cleanup: Clearing %zu remaining entities...", count);
                    em->ClearAllEntities();
                    LOG_INFO("GSM", "All entities cleared. Remaining: %zu", em->GetAllEntities().size());
                }

                // Clear camera follow
                if (auto* gfx = engine->GetGraphicsSystem()) {
                    gfx->ClearFollowTarget();
                }
            }
            };

        // ============================================================================
        // UNLOAD
        // ============================================================================
        fpUnload = []() {
            LOG_INFO("GSM", "MainMenu Lua script unloaded");
            };
        break;

    case Level_select:
        LOG_INFO("GSM", "Level_select state (Lua-scripted)");

        // ============================================================================
        // LOAD
        // ============================================================================
        fpLoad = []() {
            LOG_INFO("GSM", "Loading LevelSelect Lua script...");

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

        // ============================================================================
        // INITIALIZE
        // ============================================================================
        fpInitialize = []() {
            LOG_INFO("GSM", "LevelSelect ready (Lua-scripted)");
            };

        // ============================================================================
        // UPDATE
        // ============================================================================
        fpUpdate = []() {
            // Get engine pointer
            extern Framework::CoreEngine* engine;

            // F9 Hot Reload Support
            if (engine && engine->GetInputSystem()) {
                auto* input = engine->GetInputSystem();
                if (input->IsKeyPressed(Framework::KEY_F9)) {
                    LOG_INFO("GSM", "F9 pressed - Hot reloading LevelSelect...");
                    Framework::LevelLoader::GetInstance().ReloadCurrentLevel();
                }
            }

            // Call Lua OnUpdate with FIXED_DT from shared constant
            Framework::LevelLoader::GetInstance().UpdateCurrentLevel(
                Framework::Time::FIXED_DT_F
            );
            };

        // ============================================================================
        // DRAW
        // ============================================================================
        fpDraw = []() {
            // Drawing is now handled directly in Core.cpp via DrawCurrentLevel()
            // This prevents double-rendering (once to viewport, once to main window)
            // No-op for Lua-based levels
            };

        // ============================================================================
        // FREE
        // ============================================================================
        fpFree = []() {
            LOG_INFO("GSM", "Cleaning up LevelSelect Lua script...");

            extern Framework::CoreEngine* engine;

            // STEP 1: Reset Lua state FIRST (calls OnDestroy which may destroy entities)
            Framework::LevelLoader::GetInstance().ResetLuaState();

            // STEP 2: Clear all entities AFTER Lua cleanup (final cleanup)
            if (engine) {
                if (auto* em = engine->GetEntityManager()) {
                    size_t count = em->GetAllEntities().size();
                    LOG_INFO("GSM", "Final cleanup: Clearing %zu remaining entities...", count);
                    em->ClearAllEntities();
                    LOG_INFO("GSM", "All entities cleared. Remaining: %zu", em->GetAllEntities().size());
                }

                // Clear camera follow
                if (auto* gfx = engine->GetGraphicsSystem()) {
                    gfx->ClearFollowTarget();
                }
            }
            };

        // ============================================================================
        // UNLOAD
        // ============================================================================
        fpUnload = []() {
            LOG_INFO("GSM", "LevelSelect Lua script unloaded");
            };
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
        fpLoad = level2_Load;
        fpInitialize = level2_Initialize;
        fpUpdate = level2_Update;
        fpDraw = level2_Draw;
        fpFree = level2_Free;
        fpUnload = level2_Unload;
        break;

    case LEVEL_3:
    {
        LOG_INFO("GSM", "Level 3 state (Lua-scripted)");

        // ============================================================================
        // LOAD
        // ============================================================================
        fpLoad = []() {
            LOG_INFO("GSM", "Loading Level3 Lua script...");

            auto& loader = Framework::LevelLoader::GetInstance();
            bool success = loader.LoadLevel("assets/scripts/Level3.lua");

            if (!success) {
                LOG_ERROR("GSM", "CRITICAL: Failed to load Level3 Lua script!");
                LOG_ERROR("GSM", "Check: assets/scripts/Level3.lua exists");
                next = GS_QUIT;
            }
            else {
                LOG_INFO("GSM", "Level3 Lua script loaded successfully");
            }
            };

        // ============================================================================
        // INITIALIZE
        // ============================================================================
        fpInitialize = []() {
            LOG_INFO("GSM", "Level3 ready (Lua-scripted)");
            // Additional C++ initialization (player controller setup)
            extern Framework::CoreEngine* engine;
            if (engine) {
                auto* em = engine->GetEntityManager();
                auto* playerController = engine->GetPlayerController();
                auto* spawner = engine->GetSpawner();
                auto* input = engine->GetInputSystem();

                if (em && playerController && spawner && input) {
                    // Find player entity (spawned by TileMapLoader in Lua)
                    // Player has CircleCollider but NOT EnemyAI (distinguishes from enemies)
                    Framework::Entity player{ Framework::INVALID_ENTITY };
                    for (Framework::Entity e : em->GetAllEntities()) {
                        if (em->HasComponent<Framework::CircleCollider>(e) &&
                            !em->HasComponent<Framework::EnemyAI>(e)) {
                            player = e;
                            break;
                        }
                    }

                    if (player.GetID() != Framework::INVALID_ENTITY) {
                        // DISABLE WASD movement: Remove Movement component to prevent MovementSystem from processing player
                        // Grid movement via arrow keys only (handled by PlayerControllerSystem)
                        if (em->HasComponent<Framework::Movement>(player)) {
                            em->RemoveComponent<Framework::Movement>(player);
                            LOG_INFO("GSM", "Removed Movement component from player - WASD disabled, arrow keys only");
                        }

                        // Add player stats (AP) if not already added
                        if (!em->HasComponent<Framework::AP>(player)) {
                            em->AddComponent<Framework::AP>(player, 5);  // 100 HP, 5 AP
                            LOG_INFO("GSM", "Added AP component to player: 100 HP, 5 AP");
                        }
                        else {
                            auto& ap = em->GetComponent<Framework::AP>(player);
                            LOG_INFO("GSM", "Player already has AP: %d/%d AP",
                                ap.actionPoints, ap.maxActionPoints);
                        }

                        // Configure player controller
                        playerController->SetPlayerEntity(player);
                        playerController->SetEntitySpawner(spawner);
                        playerController->SetEntityManager(em);
                        playerController->SetInputSystem(input);
                        playerController->SetGridMovementEnabled(true);

                        // Set camera follow
                        if (auto* gfx = engine->GetGraphicsSystem()) {
                            gfx->SetFollowTarget(player);
                        }

                        LOG_INFO("GSM", "Player controller configured for entity ID: %u", player.GetID());
                        LOG_INFO("GSM", "Grid movement enabled: TRUE");
                    }
                    else {
                        LOG_ERROR("GSM", "No player entity found!");
                    }

                    // Initialize turn system
                    auto& turn = Framework::Turn();
                    turn.phase = Framework::TurnPhase::Player;
                    turn.busy = false;
                    LOG_INFO("GSM", "Turn system initialized: Phase=Player, Busy=false, TurnIndex=%d", turn.turnIndex);
                }
            }
            };

        // ============================================================================
        // UPDATE
        // ============================================================================
        fpUpdate = []() {
            extern Framework::CoreEngine* engine;
            static Framework::Entity cachedPlayer{ Framework::INVALID_ENTITY };

            // F9 Hot Reload Support
            if (engine && engine->GetInputSystem()) {
                auto* input = engine->GetInputSystem();
                if (input->IsKeyPressed(Framework::KEY_F9)) {
                    LOG_INFO("GSM", "F9 pressed - Hot reloading Level3...");
                    Framework::LevelLoader::GetInstance().ReloadCurrentLevel();
                    cachedPlayer = Framework::Entity{ Framework::INVALID_ENTITY }; // Reset cache
                }
            }

            // C++ system updates (player controller, pathfinding)
            if (engine) {
                auto* pcs = engine->GetPlayerController();
                // Pathfinding commented out for debugging
                // auto* pfs = engine->GetPathfindingSystem();

                // Update player controller
                if (pcs) {
                    pcs->Update(Framework::Time::FIXED_DT_F);
                }

                // Camera follow (MUST be set every frame like original level3_Update)
                if (auto* gfx = engine->GetGraphicsSystem()) {
                    // Set engine to playing state
                    engine->SetPlaying(true);

                    // Find and cache player entity if needed
                    // Player has CircleCollider but NOT EnemyAI (Movement component removed for grid movement)
                    if (cachedPlayer.GetID() == Framework::INVALID_ENTITY) {
                        auto* em = engine->GetEntityManager();
                        if (em) {
                            for (Framework::Entity e : em->GetAllEntities()) {
                                if (em->HasComponent<Framework::CircleCollider>(e) &&
                                    !em->HasComponent<Framework::EnemyAI>(e)) {
                                    cachedPlayer = e;
                                    break;
                                }
                            }
                        }
                    }

                    // Set camera follow target every frame (matches original)
                    if (cachedPlayer.GetID() != Framework::INVALID_ENTITY) {
                        gfx->SetFollowTarget(cachedPlayer);
                    }
                }
            }

            // Call Lua OnUpdate
            Framework::LevelLoader::GetInstance().UpdateCurrentLevel(
                Framework::Time::FIXED_DT_F
            );
            };

        // ============================================================================
        // DRAW
        // ============================================================================
        fpDraw = []() {
            // Drawing is now handled directly in Core.cpp via DrawCurrentLevel()
            // This prevents double-rendering (once to viewport, once to main window)
            // No-op for Lua-based levels
            };

        // ============================================================================
        // FREE
        // ============================================================================
        fpFree = []() {
            LOG_INFO("GSM", "Cleaning up Level3...");

            extern Framework::CoreEngine* engine;

            // STEP 1: Reset Lua state FIRST (calls OnDestroy which may destroy entities)
            Framework::LevelLoader::GetInstance().ResetLuaState();

            // STEP 2: C++ cleanup and final entity clear
            if (engine) {
                // Reset player controller
                if (auto* pcs = engine->GetPlayerController()) {
                    pcs->ResetGridState();
                    pcs->SetGridMovementEnabled(false);
                }

                // Clear camera follow
                if (auto* gfx = engine->GetGraphicsSystem()) {
                    gfx->ClearFollowTarget();
                }

                // Clear all entities AFTER Lua cleanup (final cleanup)
                if (auto* em = engine->GetEntityManager()) {
                    size_t count = em->GetAllEntities().size();
                    LOG_INFO("GSM", "Final cleanup: Clearing %zu remaining entities...", count);
                    em->ClearAllEntities();
                    LOG_INFO("GSM", "All entities cleared. Remaining: %zu", em->GetAllEntities().size());
                }
            }
            };

        // ============================================================================
        // UNLOAD
        // ============================================================================
        fpUnload = []() {
            LOG_INFO("GSM", "Level3 Lua script unloaded");
            };
    }
    break;

    case TUTORIAL:
        LOG_INFO("GSM", "Tutorial state (Lua-scripted)");

        // ============================================================================
        // LOAD
        // ============================================================================
        fpLoad = []() {
            LOG_INFO("GSM", "Loading Tutorial Lua script...");

            auto& loader = Framework::LevelLoader::GetInstance();
            bool success = loader.LoadLevel("assets/scripts/TutorialLevel.lua");

            if (!success) {
                LOG_ERROR("GSM", "CRITICAL: Failed to load Tutorial Lua script!");
                LOG_ERROR("GSM", "Check: assets/scripts/TutorialLevel.lua exists");
                next = GS_QUIT;
            }
            else {
                LOG_INFO("GSM", "Tutorial Lua script loaded successfully");
            }
            };

        // ============================================================================
        // INITIALIZE
        // ============================================================================
        fpInitialize = []() {
            LOG_INFO("GSM", "Tutorial ready (Lua-scripted)");
            };

        // ============================================================================
        // UPDATE
        // ============================================================================
        fpUpdate = []() {
            // Get engine pointer
            extern Framework::CoreEngine* engine;

            // F9 Hot Reload Support
            if (engine && engine->GetInputSystem()) {
                auto* input = engine->GetInputSystem();
                if (input->IsKeyPressed(Framework::KEY_F9)) {
                    LOG_INFO("GSM", "F9 pressed - Hot reloading Tutorial...");
                    Framework::LevelLoader::GetInstance().ReloadCurrentLevel();
                }
            }

            // Call Lua OnUpdate with FIXED_DT from shared constant
            Framework::LevelLoader::GetInstance().UpdateCurrentLevel(
                Framework::Time::FIXED_DT_F
            );
            };

        // ============================================================================
        // DRAW
        // ============================================================================
        fpDraw = []() {
            // Drawing is now handled directly in Core.cpp via DrawCurrentLevel()
            // This prevents double-rendering (once to viewport, once to main window)
            // No-op for Lua-based levels
            };

        // ============================================================================
        // FREE
        // ============================================================================
        fpFree = []() {
            LOG_INFO("GSM", "Cleaning up Tutorial Lua script...");

            extern Framework::CoreEngine* engine;

            // STEP 1: Reset Lua state FIRST (calls OnDestroy which may destroy entities)
            Framework::LevelLoader::GetInstance().ResetLuaState();

            // STEP 2: Clear all entities AFTER Lua cleanup (final cleanup)
            if (engine) {
                if (auto* em = engine->GetEntityManager()) {
                    size_t count = em->GetAllEntities().size();
                    LOG_INFO("GSM", "Final cleanup: Clearing %zu remaining entities...", count);
                    em->ClearAllEntities();
                    LOG_INFO("GSM", "All entities cleared. Remaining: %zu", em->GetAllEntities().size());
                }

                // Clear camera follow
                if (auto* gfx = engine->GetGraphicsSystem()) {
                    gfx->ClearFollowTarget();
                }
            }
            };

        // ============================================================================
        // UNLOAD
        // ============================================================================
        fpUnload = []() {
            LOG_INFO("GSM", "Tutorial Lua script unloaded");
            };
        break;

    case LEVEL_END:
        LOG_INFO("GSM", "Main Menu state (Lua-scripted)");

        // ============================================================================
        // LOAD
        // ============================================================================
        fpLoad = []() {
            LOG_INFO("GSM", "Loading MainMenu Lua script...");

            auto& loader = Framework::LevelLoader::GetInstance();
            bool success = loader.LoadLevel("assets/scripts/EndLevel.lua");

            if (!success) {
                LOG_ERROR("GSM", "CRITICAL: Failed to load MainMenu Lua script!");
                LOG_ERROR("GSM", "Check: assets/scripts/EndLevel.lua exists");
                next = GS_QUIT;
            }
            else {
                LOG_INFO("GSM", "MainMenu Lua script loaded successfully");
            }
            };

        // ============================================================================
        // INITIALIZE
        // ============================================================================
        fpInitialize = []() {
            LOG_INFO("GSM", "MainMenu ready (Lua-scripted)");
            };

        // ============================================================================
        // UPDATE
        // ============================================================================
        fpUpdate = []() {
            // Get engine pointer
            extern Framework::CoreEngine* engine;

            // F9 Hot Reload Support
            if (engine && engine->GetInputSystem()) {
                auto* input = engine->GetInputSystem();
                if (input->IsKeyPressed(Framework::KEY_F9)) {
                    LOG_INFO("GSM", "F9 pressed - Hot reloading MainMenu...");
                    Framework::LevelLoader::GetInstance().ReloadCurrentLevel();
                }
            }

            // Call Lua OnUpdate with FIXED_DT from shared constant
            Framework::LevelLoader::GetInstance().UpdateCurrentLevel(
                Framework::Time::FIXED_DT_F
            );
            };

        // ============================================================================
        // DRAW
        // ============================================================================
        fpDraw = []() {
            // Drawing is now handled directly in Core.cpp via DrawCurrentLevel()
            // This prevents double-rendering (once to viewport, once to main window)
            // No-op for Lua-based levels
            };

        // ============================================================================
        // FREE
        // ============================================================================
        fpFree = []() {
            LOG_INFO("GSM", "Cleaning up MainMenu Lua script...");

            extern Framework::CoreEngine* engine;

            // STEP 1: Reset Lua state FIRST (calls OnDestroy which may destroy entities)
            Framework::LevelLoader::GetInstance().ResetLuaState();

            // STEP 2: Clear all entities AFTER Lua cleanup (final cleanup)
            if (engine) {
                if (auto* em = engine->GetEntityManager()) {
                    size_t count = em->GetAllEntities().size();
                    LOG_INFO("GSM", "Final cleanup: Clearing %zu remaining entities...", count);
                    em->ClearAllEntities();
                    LOG_INFO("GSM", "All entities cleared. Remaining: %zu", em->GetAllEntities().size());
                }

                // Clear camera follow
                if (auto* gfx = engine->GetGraphicsSystem()) {
                    gfx->ClearFollowTarget();
                }
            }
            };

        // ============================================================================
        // UNLOAD
        // ============================================================================
        fpUnload = []() {
            LOG_INFO("GSM", "MainMenu Lua script unloaded");
            };
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