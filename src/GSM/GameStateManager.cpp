/**
===============================================================================
 File:          GameStateManager.cpp
 Author:        Padilla Carl Jameson Z.
 Email:         c.padilla@digipen.edu
 Date:          2025-11-30
 Contribution:  100%
 ------------------------------------------------------------------------------

 GAME STATE MANAGER - State Lifecycle & Hot Reload

 Brief:
    Manages game state transitions and lifecycle for all game levels and menus.
    Supports Lua-scripted states with hot reload capability via F9 key. Each
    state defines load, initialize, update, draw, free, and unload function
    pointers. Integrates with LevelLoader for Lua script management and handles
    proper cleanup order (Lua first, then C++ systems). Uses fixed delta time
    for consistent gameplay updates across all states.

 Usage:
    // Register a new game state
    GSM::AddState("MainMenu", &MainMenuLoad, &MainMenuInit,
                  &MainMenuUpdate, &MainMenuDraw,
                  &MainMenuFree, &MainMenuUnload);

    // Transition to a state
    GSM::SetNextState("MainMenu");

    // Hot reload current Lua state (F9)
    if (Input::IsKeyTriggered(KEY_F9)) {
        GSM::ReloadCurrentState();
    }


 Copyright (C) 2026 DigiPen Institute of Technology.
 Reproduction or disclosure of this file or its contents
 without the prior written consent of DigiPen Institute of
 Technology is prohibited.
===============================================================================
*/


#include "Precompiled.h"
#include "LevelLoader.h"
#include "TimeConstants.h"
#include "ImguiSystem.h"  // For GSM_SetNextState to check ImGui state

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

// Flag to indicate if the next level should be loaded in editor mode
bool g_loadAsEditorMode = false;

// Flag to indicate if the game was playing when transitioning to next level
bool g_preservePlayingState = false;

// ============================================================================
// GSM FUNCTIONS
// ============================================================================

/** @brief Initializes the game state manager with a starting state. */
void GSM_Initialize(int startingState)
{
    current = previous = next = startingState;
    LOG_INFO("GSM", "Game State Manager initialized with state: %d", startingState);
    LOG_INFO("GSM", "Using LUA-SCRIPTED MainMenu (Pure Lua mode)");
    LOG_INFO("GSM", "Fixed DT: %.4f seconds", Framework::Time::FIXED_DT);
    LOG_INFO("GSM", "Press F9 to hot reload Lua scripts");
}

/** @brief Updates function pointers based on current game state. */
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
            bool success = loader.LoadLevel("assets/scripts/MainMenuLevel.lua", g_loadAsEditorMode);
            g_loadAsEditorMode = false;  // Reset flag after use

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
            bool success = loader.LoadLevel("assets/scripts/LevelSelectLevel.lua", g_loadAsEditorMode);
            g_loadAsEditorMode = false;  // Reset flag after use

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

    case LEVEL_2:
        // Use LevelLoader to load Level2.lua
        fpLoad = []() {
                Framework::LevelLoader::GetInstance().LoadLevel(
                "assets/scripts/Level2.lua",
                g_loadAsEditorMode
            );
            g_loadAsEditorMode = false;  // Reset flag after use
            };
        fpInitialize = []() {}; // LevelLoader handles init via OnInit()
        fpUpdate = []() {
            Framework::LevelLoader::GetInstance().UpdateCurrentLevel(
                Framework::Time::FIXED_DT
            );
            };
        fpDraw = []() {
            Framework::LevelLoader::GetInstance().DrawCurrentLevel();
            };
        fpFree = []() {
            Framework::LevelLoader::GetInstance().UnloadCurrentLevel();
            };
        fpUnload = []() {}; // Cleanup handled by LevelLoader
        LOG_INFO("GSM", "State changed to: LEVEL_2 (Lua)");
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
            bool success = loader.LoadLevel("assets/scripts/ProceduralMapLevel.lua", g_loadAsEditorMode);
            g_loadAsEditorMode = false;  // Reset flag after use

            if (!success) {
                LOG_ERROR("GSM", "CRITICAL: Failed to load Level3 Lua script!");
                LOG_ERROR("GSM", "Check: assets/scripts/Level3Clean.lua exists");
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
                        // DISABLED: Don't force grid movement ON - let Lua scripts control it
                        // playerController->SetGridMovementEnabled(true);

                        // Set camera follow
                        if (auto* gfx = engine->GetGraphicsSystem()) {
                            gfx->SetFollowTarget(player);
                        }

                        LOG_INFO("GSM", "Player controller configured for entity ID: %u", player.GetID());
                        // LOG_INFO("GSM", "Grid movement enabled: TRUE");
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
            /*if (engine) {
                auto* pcs = engine->GetPlayerController();
                if (pcs) {
                    pcs->Update(Framework::Time::FIXED_DT_F);
                }

                auto* pfs = engine->GetPathfindingSystem();
                if (pfs) {
                    pfs->Update(Framework::Time::FIXED_DT_F);
                }

                // Camera follow (MUST be set every frame like original level3_Update)
                if (auto* gfx = engine->GetGraphicsSystem()) {
                    // Set engine to playing state
                    //engine->SetPlaying(true);

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
            }*/

            if (engine) {

                // STOP must actually stop: if not playing, do not run demo updates
                if (!engine->IsPlaying()) {
                    if (auto* gfx = engine->GetGraphicsSystem()) {
                        gfx->ClearFollowTarget();
                    }
                    return;
                }

                auto* pcs = engine->GetPlayerController();
                if (pcs) {
                    pcs->Update(Framework::Time::FIXED_DT_F);
                }

                auto* pfs = engine->GetPathfindingSystem();
                if (pfs) {
                    pfs->Update(Framework::Time::FIXED_DT_F);
                }

                // Camera follow (set every frame like original)
                if (auto* gfx = engine->GetGraphicsSystem()) {

                    // REMOVE THIS LINE:
                    // engine->SetPlaying(true);

                    // ... keep the cachedPlayer logic and SetFollowTarget exactly as-is ...
                    // Find and cache player entity if needed
                    // ...
                    // Set camera follow target every frame
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
            bool success = loader.LoadLevel("assets/scripts/TutorialLevel.lua", g_loadAsEditorMode);
            g_loadAsEditorMode = false;  // Reset flag after use

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
            bool success = loader.LoadLevel("assets/scripts/EndLevel.lua", g_loadAsEditorMode);
            g_loadAsEditorMode = false;  // Reset flag after use

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

/**
 * @brief Set the next game state, preserving editor mode if ImGui is enabled
 */
void GSM_SetNextState(int nextState) {
    // Check if ImGui is currently enabled
    if (Framework::CORE) {
        auto* imgui = Framework::CORE->GetImGuiSystem();
        if (imgui && imgui->IsEnabled()) {
            g_loadAsEditorMode = true;
            LOG_INFO("GSM", "ImGui enabled - next level will load in editor mode");
        }
        // Check if game is currently playing - preserve this state for next level
        if (Framework::CORE->IsPlaying()) {
            g_preservePlayingState = true;
            LOG_INFO("GSM", "Game is playing - next level will start in playing state");
        }
    }
    next = nextState;
}