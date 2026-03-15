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
#include "TagHelper.h"

#include <fstream>
#include <nlohmann/json.hpp>
#include <unordered_map>

// ============================================================================
// LEVEL SCRIPT LOOKUP
// ============================================================================

namespace {
    constexpr const char* kEmptyScriptPath = "";

    std::unordered_map<int, std::string> g_manifestScriptPaths;
    bool g_manifestLoaded = false;

    const char* GetFallbackLevelScript(int state)
    {
        static std::string fallbackPath;
        const std::string scriptRoot = ConfigReader::GetProjectPath("script_root", "assets/scripts/");
        const auto buildPath = [&](const char* filename) -> const char* {
            fallbackPath = scriptRoot;
            if (!fallbackPath.empty() && fallbackPath.back() != '/') {
                fallbackPath.push_back('/');
            }
            fallbackPath += filename;
            return fallbackPath.c_str();
        };

        switch (state) {
        case mainMenu: return buildPath("MainMenuLevel.lua");
        case settingsMenu: return buildPath("SettingsLevel.lua");
        case Level_select: return buildPath("LevelSelectLevel.lua");
        case LEVEL_2: return buildPath("Level2.lua");
        case LEVEL_3: return buildPath("ProceduralMapLevel.lua");
        case LEVEL_END: return buildPath("EndLevel.lua");
        case TUTORIAL: return buildPath("TutorialLevel.lua");
        case CONTROL: return buildPath("ControlLevel.lua");
        case CONTROL2: return buildPath("Control2Level.lua");
        case SKILL_SETS: return buildPath("SkillSetsLevel.lua");
        case WIN_SCREEN: return buildPath("WinLevel.lua");
        case LOSE_SCREEN: return buildPath("LoseLevel.lua");
        default: return nullptr;
        }
    }

    bool IsScriptedState(int state)
    {
        switch (state) {
        case mainMenu:
        case settingsMenu:
        case Level_select:
        case LEVEL_2:
        case LEVEL_3:
        case LEVEL_END:
        case TUTORIAL:
        case CONTROL:
        case CONTROL2:
        case SKILL_SETS:
        case WIN_SCREEN:
        case LOSE_SCREEN:
            return true;
        default:
            return false;
        }
    }

    void LoadStateRegistryIfNeeded()
    {
        if (g_manifestLoaded) return;
        g_manifestLoaded = true;

        try {
            const std::string registryPath =
                ConfigReader::GetProjectPath("state_registry", ConfigReader::STATE_REGISTRY_PATH);
            std::ifstream file(registryPath);
            if (!file.is_open()) {
                LOG_WARN("GSM", "State registry file not found at '%s'; using fallback paths", registryPath.c_str());
                return;
            }

            nlohmann::json doc;
            file >> doc;

            if (!doc.contains("states") || !doc["states"].is_array()) {
                LOG_WARN("GSM", "State registry missing 'states' array; using fallback paths");
                return;
            }

            for (const auto& stateEntry : doc["states"]) {
                if (!stateEntry.is_object()) continue;
                if (!stateEntry.contains("id") || !stateEntry["id"].is_number_integer()) continue;
                if (!stateEntry.contains("script") || !stateEntry["script"].is_string()) continue;

                const int id = stateEntry["id"].get<int>();
                const std::string scriptPath = stateEntry["script"].get<std::string>();
                if (!scriptPath.empty()) {
                    g_manifestScriptPaths[id] = scriptPath;
                }
            }

            LOG_INFO("GSM", "Loaded state registry script paths from %s (%zu entries)",
                registryPath.c_str(), g_manifestScriptPaths.size());
        }
        catch (const nlohmann::json::exception& e) {
            LOG_WARN("GSM", "Failed to parse state registry JSON: %s", e.what());
        }
        catch (const std::exception& e) {
            LOG_WARN("GSM", "Failed to load state registry: %s", e.what());
        }
    }
}

/// Returns the Lua script path for a game state, or nullptr if none.
static const char* GetLevelScript(int state)
{
    LoadStateRegistryIfNeeded();

    auto it = g_manifestScriptPaths.find(state);
    if (it != g_manifestScriptPaths.end() && !it->second.empty()) {
        return it->second.c_str();
    }

    const char* fallback = GetFallbackLevelScript(state);
    if (fallback && fallback[0] != '\0') {
        return fallback;
    }

    if (IsScriptedState(state)) {
        LOG_ERROR("GSM", "No script mapping found for scripted state id=%d", state);
    }
    return kEmptyScriptPath;
}

static bool ValidateScriptMappingsAtStartup()
{
    const int requiredStates[] = {
        mainMenu,
        settingsMenu,
        Level_select,
        LEVEL_2,
        LEVEL_3,
        LEVEL_END,
        TUTORIAL,
        CONTROL,
        CONTROL2,
        SKILL_SETS,
        WIN_SCREEN,
        LOSE_SCREEN
    };

    bool ok = true;
    for (int state : requiredStates) {
        const char* path = GetLevelScript(state);
        if (!path || path[0] == '\0') {
            LOG_ERROR("GSM", "Missing script path for required state id=%d", state);
            ok = false;
        }
    }

    if (ok) {
        LOG_INFO("GSM", "State script mapping validation passed");
    }
    else {
        LOG_ERROR("GSM", "State script mapping validation failed");
    }
    return ok;
}

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

    const bool mappingOk = ValidateScriptMappingsAtStartup();
    LOG_WARN("GSM", "[PHASE1] State registry validation result: %s",
        mappingOk ? "PASS" : "FAIL");
    std::cout << "[GSM][PHASE1] State registry validation result: "
        << (mappingOk ? "PASS" : "FAIL") << std::endl;

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
            bool success = loader.LoadLevel(GetLevelScript(mainMenu), g_loadAsEditorMode);
            g_loadAsEditorMode = false;  // Reset flag after use

            if (!success) {
                LOG_ERROR("GSM", "CRITICAL: Failed to load MainMenu Lua script!");
                LOG_ERROR("GSM", "Check: %s exists", GetLevelScript(mainMenu));
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

    case settingsMenu:
        LOG_INFO("GSM", "Settings Menu state (Lua-scripted)");

        // ============================================================================
        // LOAD
        // ============================================================================
        fpLoad = []() {
            LOG_INFO("GSM", "Loading Settings Lua script...");

            auto& loader = Framework::LevelLoader::GetInstance();
            bool success = loader.LoadLevel(GetLevelScript(settingsMenu), g_loadAsEditorMode);
            g_loadAsEditorMode = false;  // Reset flag after use

            if (!success) {
                LOG_ERROR("GSM", "CRITICAL: Failed to load Settings Lua script!");
                LOG_ERROR("GSM", "Check: %s exists", GetLevelScript(settingsMenu));
                next = GS_QUIT;
            }
            else {
                LOG_INFO("GSM", "Settings Lua script loaded successfully");
            }
            };

        // ============================================================================
        // INITIALIZE
        // ============================================================================
        fpInitialize = []() {
            LOG_INFO("GSM", "Settings ready (Lua-scripted)");
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
                    LOG_INFO("GSM", "F9 pressed - Hot reloading Settings...");
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
            LOG_INFO("GSM", "Cleaning up Settings Lua script...");

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
            LOG_INFO("GSM", "Settings Lua script unloaded");
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
            bool success = loader.LoadLevel(GetLevelScript(Level_select), g_loadAsEditorMode);
            g_loadAsEditorMode = false;  // Reset flag after use

            if (!success) {
                LOG_ERROR("GSM", "CRITICAL: Failed to load LevelSelect Lua script!");
                LOG_ERROR("GSM", "Check: %s exists", GetLevelScript(Level_select));
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
                GetLevelScript(LEVEL_2),
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
            bool success = loader.LoadLevel(GetLevelScript(LEVEL_3), g_loadAsEditorMode);
            g_loadAsEditorMode = false;  // Reset flag after use

            if (!success) {
                LOG_ERROR("GSM", "CRITICAL: Failed to load Level3 Lua script!");
                LOG_ERROR("GSM", "Check: %s exists", GetLevelScript(LEVEL_3));
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
            // Player setup (Movement removal, camera follow, turn system init)
            // is now handled by ProceduralMapLevel.lua OnInit via:
            //   RemoveMovementComponent(), SetCameraFollowTarget(), InitializeTurnSystem()

            // Configure C++ player controller with minimal plumbing so its
            // Update() can run (even though gridMovement is disabled by Lua).
            extern Framework::CoreEngine* engine;
            if (engine) {
                auto* em = engine->GetEntityManager();
                auto* pc = engine->GetPlayerController();
                auto* spawner = engine->GetSpawner();
                auto* input = engine->GetInputSystem();

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
                    if (cachedPlayer.GetID() == Framework::INVALID_ENTITY) {
                        auto* em = engine->GetEntityManager();
                        if (em) {
                            cachedPlayer = Framework::FindFirstByTag(em, "Player");
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
            bool success = loader.LoadLevel(GetLevelScript(TUTORIAL), g_loadAsEditorMode);
            g_loadAsEditorMode = false;  // Reset flag after use

            if (!success) {
                LOG_ERROR("GSM", "CRITICAL: Failed to load Tutorial Lua script!");
                LOG_ERROR("GSM", "Check: %s exists", GetLevelScript(TUTORIAL));
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

    case CONTROL:
        LOG_INFO("GSM", "Control state (Lua-scripted)");

        // ============================================================================
        // LOAD
        // ============================================================================
        fpLoad = []() {
            LOG_INFO("GSM", "Loading Control Lua script...");

            auto& loader = Framework::LevelLoader::GetInstance();
            bool success = loader.LoadLevel(GetLevelScript(CONTROL), g_loadAsEditorMode);
            g_loadAsEditorMode = false;  // Reset flag after use

            if (!success) {
                LOG_ERROR("GSM", "CRITICAL: Failed to load Control Lua script!");
                LOG_ERROR("GSM", "Check: %s exists", GetLevelScript(CONTROL));
                next = GS_QUIT;
            }
            else {
                LOG_INFO("GSM", "Control Lua script loaded successfully");
            }
            };

        // ============================================================================
        // INITIALIZE
        // ============================================================================
        fpInitialize = []() {
            LOG_INFO("GSM", "Control page ready (Lua-scripted)");
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
                    LOG_INFO("GSM", "F9 pressed - Hot reloading Control...");
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
            LOG_INFO("GSM", "Cleaning up Control Lua script...");

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
            LOG_INFO("GSM", "Control Lua script unloaded");
            };
        break;

    case CONTROL2:
        LOG_INFO("GSM", "Control2 state (Lua-scripted)");

        // ============================================================================
        // LOAD
        // ============================================================================
        fpLoad = []() {
            LOG_INFO("GSM", "Loading Control2 Lua script...");

            auto& loader = Framework::LevelLoader::GetInstance();
            bool success = loader.LoadLevel(GetLevelScript(CONTROL2), g_loadAsEditorMode);
            g_loadAsEditorMode = false;  // Reset flag after use

            if (!success) {
                LOG_ERROR("GSM", "CRITICAL: Failed to load Control2 Lua script!");
                LOG_ERROR("GSM", "Check: %s exists", GetLevelScript(CONTROL2));
                next = GS_QUIT;
            }
            else {
                LOG_INFO("GSM", "Control2 Lua script loaded successfully");
            }
            };

        // ============================================================================
        // INITIALIZE
        // ============================================================================
        fpInitialize = []() {
            LOG_INFO("GSM", "Control2 page ready (Lua-scripted)");
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
                    LOG_INFO("GSM", "F9 pressed - Hot reloading Control2...");
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
            LOG_INFO("GSM", "Cleaning up Control2 Lua script...");

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
            LOG_INFO("GSM", "Control2 Lua script unloaded");
            };
        break;

    case SKILL_SETS:
        LOG_INFO("GSM", "Skill sets state (Lua-scripted)");

        // ============================================================================
        // LOAD
        // ============================================================================
        fpLoad = []() {
            LOG_INFO("GSM", "Loading SkillSets Lua script...");

            auto& loader = Framework::LevelLoader::GetInstance();
            bool success = loader.LoadLevel(GetLevelScript(SKILL_SETS), g_loadAsEditorMode);
            g_loadAsEditorMode = false;  // Reset flag after use

            if (!success) {
                LOG_ERROR("GSM", "CRITICAL: Failed to load SkillSets Lua script!");
                LOG_ERROR("GSM", "Check: %s exists", GetLevelScript(SKILL_SETS));
                next = GS_QUIT;
            }
            else {
                LOG_INFO("GSM", "SkillSets Lua script loaded successfully");
            }
            };

        // ============================================================================
        // INITIALIZE
        // ============================================================================
        fpInitialize = []() {
            LOG_INFO("GSM", "Skill sets page ready (Lua-scripted)");
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
                    LOG_INFO("GSM", "F9 pressed - Hot reloading SkillSets...");
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
            LOG_INFO("GSM", "Cleaning up SkillSets Lua script...");

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
            LOG_INFO("GSM", "SkillSets Lua script unloaded");
            };
        break;

    case WIN_SCREEN:
        LOG_INFO("GSM", "Win screen state (Lua-scripted)");

        // ============================================================================
        // LOAD
        // ============================================================================
        fpLoad = []() {
            LOG_INFO("GSM", "Loading WinLevel Lua script...");

            auto& loader = Framework::LevelLoader::GetInstance();
            bool success = loader.LoadLevel(GetLevelScript(WIN_SCREEN), g_loadAsEditorMode);
            g_loadAsEditorMode = false;  // Reset flag after use

            if (!success) {
                LOG_ERROR("GSM", "CRITICAL: Failed to load WinLevel Lua script!");
                LOG_ERROR("GSM", "Check: %s exists", GetLevelScript(WIN_SCREEN));
                next = GS_QUIT;
            }
            else {
                LOG_INFO("GSM", "WinLevel Lua script loaded successfully");
            }
            };

        // ============================================================================
        // INITIALIZE
        // ============================================================================
        fpInitialize = []() {
            LOG_INFO("GSM", "Win screen ready (Lua-scripted)");
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
                    LOG_INFO("GSM", "F9 pressed - Hot reloading WinLevel...");
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
            LOG_INFO("GSM", "Cleaning up WinLevel Lua script...");

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
            LOG_INFO("GSM", "WinLevel Lua script unloaded");
            };
        break;

    case LOSE_SCREEN:
        LOG_INFO("GSM", "Lose screen state (Lua-scripted)");

        // ============================================================================
        // LOAD
        // ============================================================================
        fpLoad = []() {
            LOG_INFO("GSM", "Loading LoseLevel Lua script...");

            auto& loader = Framework::LevelLoader::GetInstance();
            bool success = loader.LoadLevel(GetLevelScript(LOSE_SCREEN), g_loadAsEditorMode);
            g_loadAsEditorMode = false;  // Reset flag after use

            if (!success) {
                LOG_ERROR("GSM", "CRITICAL: Failed to load LoseLevel Lua script!");
                LOG_ERROR("GSM", "Check: %s exists", GetLevelScript(LOSE_SCREEN));
                next = GS_QUIT;
            }
            else {
                LOG_INFO("GSM", "LoseLevel Lua script loaded successfully");
            }
            };

        // ============================================================================
        // INITIALIZE
        // ============================================================================
        fpInitialize = []() {
            LOG_INFO("GSM", "Lose screen ready (Lua-scripted)");
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
                    LOG_INFO("GSM", "F9 pressed - Hot reloading LoseLevel...");
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
            LOG_INFO("GSM", "Cleaning up LoseLevel Lua script...");

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
            LOG_INFO("GSM", "LoseLevel Lua script unloaded");
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
            bool success = loader.LoadLevel(GetLevelScript(LEVEL_END), g_loadAsEditorMode);
            g_loadAsEditorMode = false;  // Reset flag after use

            if (!success) {
                LOG_ERROR("GSM", "CRITICAL: Failed to load EndLevel Lua script!");
                LOG_ERROR("GSM", "Check: %s exists", GetLevelScript(LEVEL_END));
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