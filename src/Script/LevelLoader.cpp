/*
===============================================================================
File:        LevelLoader.cpp
Author:      GE YONGQI, Sim Kah Yan
Email:       yongqi.ge@digipen.edu; kahyan.sim@digipen.edu
Date:        2026-02-04 (yyyy-mm-dd)
Contribution: GE YONGQI (remaining); Sim Kah Yan 26% (175 lines of 669 total)
-------------------------------------------------------------------------------
Brief:
Lua-based level loading: singleton manages Lua state, level lifecycle
(Load/Update/Draw/Unload), hot-reload, and C++ API bindings for level scripts.
Editor mode (F1) and preserve-playing-state on level transition supported.

Details:
- Initialize(engine) caches UISystem, AudioSystem, GraphicsSystem; CreateLuaState
    adds configured script_root to package.path, RegisterLevelAPI(), stores __level_loader_ptr.
- LoadLevel(scriptPath, isEditorMode): UnloadCurrentLevel if needed; sets
  IS_EDITOR_LOAD, g_preservePlayingState/g_loadAsEditorMode for editor/playing;
  luaL_dofile; enforces editor/ImGui state; checks OnInit/OnUpdate/OnDraw/OnDestroy;
  calls OnInit(); marks levelLoaded, currentLevelPath. UnloadCurrentLevel calls
  OnDestroy, then EntityManager ClearAllEntities/ResetEntityIDCounter.
- UpdateCurrentLevel(dt) / DrawCurrentLevel() call OnUpdate(dt) / OnDraw() when
  present. ReloadCurrentLevel: path save, unload, load. ResetLuaState: unload,
  destroy Lua state, create fresh.
- RegisterLevelAPI() registers 60+ C APIs: Log; Camera; Engine; ImGui; Pause;
  Audio; UI Buttons/Text; Input; JSON; TileMap; SpawnSprite/SpawnAnimatedSprite,
  SetSprite*, DestroyEntity, ClearAllEntities; GetPlayerAP/GetCameraPosition/
  GetPlayerAttackAP/GetPlayerHP; FindPlayer, GetAllEnemies, SetEnemyTarget,
  GetCurrentTurn, GetChestProgress; Enemy turn manager; Animation config/player
  load; PlayAnimationByName, Set/GetAnimationFrame(Count); Animation control;
  Party (GetEntityAP, ConsumeEntityAP, SetActiveCharacter, etc.); Script component;
  Grid movement; Save/Load/Procedural map; Entity spawning; ToggleEditorMode,
  IsEditorMode, ShouldDisableGameplay.
- Lua C implementations: camera, engine, ImGui, LoadAnimationConfig,
  LoadPlayerAnimation (find player by CircleCollider, add SpriteAnimation, load);
  Scroll animation API (PlayAnimationByName, SetAnimationFrame, GetAnimationFrame,
  GetAnimationFrameCount) with special handling for "Scroll" animations.

Notes:
- CallLuaFunction/HasLuaFunction require valid L; errors logged and stack popped.
- GetLevelLoader(L) reads __level_loader_ptr. Windows min/max undefined before
  algorithm include. OnUpdate is always invoked (pause handled inside Lua).

Safety:
- L and loader/subsystem pointers null-checked in API callbacks. lua_pcall
  errors logged; stack cleaned. Unload clears entities and resets ID counter.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
===============================================================================
*/

#include "Precompiled.h"
#include "LevelLoader.h"
#include "Core.h"
#include "UISystem.h"
#include "Audio/AudioSystem.h"
#include "GraphicsSystemV2.h"
#include "Graphics/ParticleSystemManager.h"
#include "ImguiSystem.h"
#include "GameStateList.h"
#include "ConfigReader.h"
#include "ScriptAPIRegistry.h"
#include "Pause/GlobalPauseManager.h"
#include "Component.h"     // CircleCollider, AP components
#include "ECS/TagHelper.h" // FindFirstByTag, FindAllByTag
#include "Grid/GridECS.h"  // WorldToTile, SetOccupant for ProcessDeferredDestructions

// Fix for Windows min/max macro conflicts
#include <algorithm>
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

namespace Framework {

    // ========================================================================
    // SINGLETON
    // ========================================================================

    LevelLoader& LevelLoader::GetInstance() {
        static LevelLoader instance;
        return instance;
    }

    LevelLoader::LevelLoader() {
        LOG_INFO("LevelLoader", "Created");
    }

    LevelLoader::~LevelLoader() {
        Shutdown();
    }

    // ========================================================================
    // INITIALIZATION
    // ========================================================================

    void LevelLoader::Initialize(CoreEngine* engine) {
        LOG_INFO("LevelLoader", "Initializing...");

        coreEngine = engine;

        if (!coreEngine) {
            LOG_ERROR("LevelLoader", "CoreEngine is null!");
            return;
        }

        // Cache subsystem pointers
        uiSystem = coreEngine->GetUISystem();
        audioSystem = coreEngine->GetAudioSystem();
        graphicsSystem = coreEngine->GetGraphicsSystem();

        // Create Lua state
        CreateLuaState();

        LOG_INFO("LevelLoader", "Initialized successfully");
    }

    void LevelLoader::Shutdown() {
        LOG_INFO("LevelLoader", "Shutting down...");

        UnloadCurrentLevel();
        DestroyLuaState();

        coreEngine = nullptr;
        uiSystem = nullptr;
        audioSystem = nullptr;
        graphicsSystem = nullptr;

        LOG_INFO("LevelLoader", "Shutdown complete");
    }

    // ========================================================================
    // LUA STATE MANAGEMENT
    // ========================================================================

    void LevelLoader::CreateLuaState() {
        L = luaL_newstate();
        if (!L) {
            LOG_ERROR("LevelLoader", "Failed to create Lua state!");
            return;
        }

        luaL_openlibs(L);  // Load standard libraries

        // Add assets/scripts to Lua's module search path
        lua_getglobal(L, "package");
        lua_getfield(L, -1, "path");
        const std::string scriptRoot = ConfigReader::GetProjectPath("script_root", "assets/scripts/");
        std::string normalizedScriptRoot = scriptRoot;
        if (!normalizedScriptRoot.empty() && normalizedScriptRoot.back() != '/') {
            normalizedScriptRoot.push_back('/');
        }
        std::string newPath = std::string(lua_tostring(L, -1)) + ";" + normalizedScriptRoot + "?.lua";
        lua_pop(L, 1);  // Pop old path
        lua_pushstring(L, newPath.c_str());
        lua_setfield(L, -2, "path");
        lua_pop(L, 1);  // Pop package table

        LOG_INFO("LevelLoader", "Added %s to Lua module path", normalizedScriptRoot.c_str());

        RegisterLevelAPI();

        // Store pointer to this LevelLoader instance
        lua_pushlightuserdata(L, this);
        lua_setglobal(L, "__level_loader_ptr");

        LOG_INFO("LevelLoader", "Lua state created");
    }

    void LevelLoader::DestroyLuaState() {
        if (L) {
            lua_close(L);
            L = nullptr;
            LOG_INFO("LevelLoader", "Lua state destroyed");
        }
    }

    void LevelLoader::ResetLuaState() {
        LOG_INFO("LevelLoader", "Resetting Lua state for complete reload...");

        // Unload current level if loaded
        if (levelLoaded) {
            UnloadCurrentLevel();
        }

        // Destroy and recreate Lua state
        DestroyLuaState();
        CreateLuaState();

        LOG_INFO("LevelLoader", "Lua state reset complete - fresh VM ready");
    }

    // ========================================================================
    // LEVEL MANAGEMENT
    // ========================================================================

    bool LevelLoader::LoadLevel(const std::string& scriptPath, bool isEditorMode) {
        LOG_INFO("LevelLoader", "================================================");
        LOG_INFO("LevelLoader", "Loading level: %s", scriptPath.c_str());
        LOG_INFO("LevelLoader", "================================================");

        // Unload current level if exists
        if (levelLoaded) {
            UnloadCurrentLevel();
        }

        if (!L) {
            LOG_ERROR("LevelLoader", "Lua state not initialized!");
            return false;
        }

        // When loading a Lua level from the editor, we need to handle two cases:
        // 1. Fresh load from Open Level menu - editor mode ON, simulation OFF
        // 2. Level transition while playing - editor mode ON, simulation ON (preserve playing state)
        extern bool g_preservePlayingState;

        if (isEditorMode && coreEngine)
        {
            coreEngine->SetEditorMode(true);

            // If game was playing when transitioning, keep it playing
            if (g_preservePlayingState) {
                coreEngine->SetPlaying(true);
                LOG_INFO("LevelLoader", "Preserving playing state for level transition");
            }
            else {
                coreEngine->SetPlaying(false);
            }

            GlobalPause::SetPaused(false);
            // Enable ImGui when loading in editor mode
            if (coreEngine->GetImGuiSystem()) {
                coreEngine->GetImGuiSystem()->Enable();
            }
        }

        lua_pushboolean(L, isEditorMode);
        lua_setglobal(L, "IS_EDITOR_LOAD");

        // Load the Lua script
        if (luaL_dofile(L, scriptPath.c_str()) != LUA_OK) {
            const char* error = lua_tostring(L, -1);
            LOG_ERROR("LevelLoader", "Failed to load: %s", error);
            lua_pop(L, 1);
            return false;
        }

        // Re-enforce editor-load state in case the script ran top-level code that changed it.
        if (isEditorMode && coreEngine)
        {
            coreEngine->SetEditorMode(true);

            // Preserve playing state if transitioning between levels while playing
            if (g_preservePlayingState) {
                coreEngine->SetPlaying(true);
            }
            else {
                coreEngine->SetPlaying(false);
            }

            GlobalPause::SetPaused(false);
            // Ensure ImGui stays enabled
            if (coreEngine->GetImGuiSystem()) {
                coreEngine->GetImGuiSystem()->Enable();
            }
        }

        // Check required functions
        bool hasInit = HasLuaFunction("OnInit");
        bool hasUpdate = HasLuaFunction("OnUpdate");
        bool hasDraw = HasLuaFunction("OnDraw");
        bool hasDestroy = HasLuaFunction("OnDestroy");

        LOG_INFO("LevelLoader", "  Functions found:");
        LOG_INFO("LevelLoader", "    OnInit: %s", hasInit ? "Y" : "X");
        LOG_INFO("LevelLoader", "    OnUpdate: %s", hasUpdate ? "Y" : "X");
        LOG_INFO("LevelLoader", "    OnDraw: %s", hasDraw ? "Y" : "X");
        LOG_INFO("LevelLoader", "    OnDestroy: %s", hasDestroy ? "Y" : "X");

        if (!hasInit) {
            LOG_WARN("LevelLoader", "Level missing OnInit() function!");
        }

        // Call OnInit()
        if (hasInit) {
            if (!CallLuaFunction("OnInit")) {
                LOG_ERROR("LevelLoader", "OnInit() failed!");
                return false;
            }
        }


        if (isEditorMode && coreEngine)
        {
            coreEngine->SetEditorMode(true);

            // Preserve playing state if transitioning between levels while playing
            if (g_preservePlayingState) {
                coreEngine->SetPlaying(true);
                LOG_INFO("LevelLoader", "Editor mode with playing state preserved");
            }
            else {
                coreEngine->SetPlaying(false);
                LOG_INFO("LevelLoader", "Editor mode enabled - simulation stopped");
            }

            GlobalPause::SetPaused(false);
            // Final ensure ImGui is enabled after OnInit
            if (coreEngine->GetImGuiSystem()) {
                coreEngine->GetImGuiSystem()->Enable();
            }

            // Reset the preserve playing state flag after use
            g_preservePlayingState = false;
        }

        // Mark as loaded
        levelLoaded = true;
        currentLevelPath = scriptPath;

        LOG_INFO("LevelLoader", "Level loaded successfully");
        return true;
    }

    void LevelLoader::UnloadCurrentLevel() {
        if (!levelLoaded) return;

        LOG_INFO("LevelLoader", "Unloading level: %s", currentLevelPath.c_str());

        // Call OnDestroy() if it exists
        if (HasLuaFunction("OnDestroy")) {
            CallLuaFunction("OnDestroy");
        }

        // NEW: Clear entities that were spawned by the Lua level
        if (coreEngine)
        {
            if (auto* psm = coreEngine->GetParticleSystemManager()) {
                psm->ClearAllEmitters();
            }

            auto* em = coreEngine->GetEntityManager();
            if (em)
            {
                em->ClearAllEntities();
                em->ResetEntityIDCounter();
            }
        }

        deferredEntitiesToDestroy.clear();
        sharedIntStore.clear();
        levelLoaded = false;
        currentLevelPath.clear();

        LOG_INFO("LevelLoader", "Level unloaded");
    }

    void LevelLoader::ReloadCurrentLevel() {
        if (!levelLoaded) {
            LOG_WARN("LevelLoader", "No level loaded to reload");
            return;
        }

        LOG_INFO("LevelLoader", "HOT RELOADING: %s", currentLevelPath.c_str());

        std::string path = currentLevelPath;
        UnloadCurrentLevel();
        LoadLevel(path);

        LOG_INFO("LevelLoader", "Hot reload complete");
    }

    // ========================================================================
    // LIFECYCLE CALLS
    // ========================================================================

    void LevelLoader::DeferEntityDestruction(uint32_t entityID) {
        // Deduplicate: prevent double-destruction crash if called multiple times for same entity
        for (uint32_t existing : deferredEntitiesToDestroy) {
            if (existing == entityID) {
                LOG_WARN("LevelLoader", "DeferEntityDestruction: Entity %u already queued, ignoring duplicate", entityID);
                return;
            }
        }
        deferredEntitiesToDestroy.push_back(entityID);
    }

    void LevelLoader::ProcessDeferredDestructions() {
        if (!coreEngine) return;
        auto* em = coreEngine->GetEntityManager();
        if (!em) return;

        for (uint32_t id : deferredEntitiesToDestroy) {
            Entity entity(id);
            if (em->HasComponent<TagComponent>(entity) &&
                em->GetComponent<TagComponent>(entity).tag == "Enemy" &&
                L && HasLuaFunction("SyncEnemyTurnBeforeEntityDestroyed")) {
                lua_getglobal(L, "SyncEnemyTurnBeforeEntityDestroyed");
                lua_pushinteger(L, static_cast<lua_Integer>(id));
                if (lua_pcall(L, 1, 0, 0) != LUA_OK) {
                    const char* err = lua_tostring(L, -1);
                    LOG_WARN("LevelLoader", "SyncEnemyTurnBeforeEntityDestroyed error: %s", err ? err : "unknown");
                    lua_pop(L, 1);
                }
            }
            if (em->HasComponent<Transform>(entity)) {
                auto& transform = em->GetComponent<Transform>(entity);
                auto tileOpt = Framework::WorldToTile(transform.position);
                if (tileOpt.has_value()) {
                    Framework::SetOccupant(tileOpt.value(), Entity{ INVALID_ENTITY });
                }
            }
            // Call OnDestroy before DestroyEntity (Lua call stack is fully unwound now - safe)
            if (em->HasComponent<ScriptComponent>(entity)) {
                auto& script = em->GetComponent<ScriptComponent>(entity);
                if (script.hasOnDestroy && script.L) {
                    lua_getglobal(script.L, "OnDestroy");
                    if (lua_isfunction(script.L, -1)) {
                        if (lua_pcall(script.L, 0, 0, 0) != LUA_OK) {
                            lua_pop(script.L, 1);
                        }
                    }
                    else {
                        lua_pop(script.L, 1);
                    }
                }
            }
            em->DestroyEntity(entity);
        }
        deferredEntitiesToDestroy.clear();
    }

    void LevelLoader::UpdateCurrentLevel(float dt) {
        if (!levelLoaded || !L) return;

        // Process entities queued for destruction (avoids crash when Parry/Knight Oath kills caller)
        ProcessDeferredDestructions();

        // Update tile tints (restore expired tints)
        UpdateTileTints();

        // NOTE: Don't skip OnUpdate when paused - PauseMenu needs to run to handle unpause!
        // The Lua level can check IsPaused() internally if needed.

        if (HasLuaFunction("OnUpdate")) {
            lua_getglobal(L, "OnUpdate");
            lua_pushnumber(L, dt);

            if (lua_pcall(L, 1, 0, 0) != LUA_OK) {
                const char* error = lua_tostring(L, -1);
                LOG_ERROR("LevelLoader", "OnUpdate() error: %s", error);
                lua_pop(L, 1);
            }
        }
    }

    void LevelLoader::DrawCurrentLevel() {
        if (!levelLoaded || !L) return;

        if (HasLuaFunction("OnDraw")) {
            CallLuaFunction("OnDraw");
        }
    }

    // ========================================================================
    // HELPER FUNCTIONS
    // ========================================================================

    bool LevelLoader::HasLuaFunction(const char* funcName) {
        if (!L) return false;

        lua_getglobal(L, funcName);
        bool exists = lua_isfunction(L, -1);
        lua_pop(L, 1);
        return exists;
    }

    bool LevelLoader::CallLuaFunction(const char* funcName, int numArgs) {
        if (!L) return false;

        lua_getglobal(L, funcName);
        if (!lua_isfunction(L, -1)) {
            lua_pop(L, 1);
            return false;
        }

        if (lua_pcall(L, numArgs, 0, 0) != LUA_OK) {
            const char* error = lua_tostring(L, -1);
            LOG_ERROR("LevelLoader", "%s() error: %s", funcName, error);
            lua_pop(L, 1);
            return false;
        }

        return true;
    }

    bool LevelLoader::ApplyProjectileDamageToEnemy(uint32_t enemyID, int damage, uint32_t attackerID) {
        if (!levelLoaded || !L) return false;

        lua_getglobal(L, "ApplyProjectileDamage");
        if (!lua_isfunction(L, -1)) {
            lua_pop(L, 1);
            lua_getglobal(L, "DamageEntity");
            if (!lua_isfunction(L, -1)) {
                lua_pop(L, 1);
                return false;
            }
            lua_pushinteger(L, static_cast<lua_Integer>(enemyID));
            lua_pushinteger(L, static_cast<lua_Integer>(damage));
            lua_pushinteger(L, static_cast<lua_Integer>(attackerID));
            int result = lua_pcall(L, 3, 1, 0);
            bool success = false;
            if (result == LUA_OK && lua_isboolean(L, -1)) {
                success = lua_toboolean(L, -1) != 0;
            }
            lua_pop(L, 1);
            return success;
        }
        lua_pushinteger(L, static_cast<lua_Integer>(enemyID));
        lua_pushinteger(L, static_cast<lua_Integer>(damage));
        lua_pushinteger(L, static_cast<lua_Integer>(attackerID));
        int result = lua_pcall(L, 3, 1, 0);
        bool success = false;
        if (result == LUA_OK && lua_isboolean(L, -1)) {
            success = lua_toboolean(L, -1) != 0;
        } else if (result != LUA_OK) {
            const char* err = lua_tostring(L, -1);
            LOG_ERROR("LevelLoader", "ApplyProjectileDamage error: %s", err ? err : "unknown");
        }
        lua_pop(L, 1);
        return success;
    }

    bool LevelLoader::ApplyDamageToEntity(uint32_t targetID, int damage, uint32_t attackerID) {
        if (!levelLoaded || !L) return false;

        lua_getglobal(L, "DamageEntity");
        if (!lua_isfunction(L, -1)) {
            lua_pop(L, 1);
            return false;
        }
        lua_pushinteger(L, static_cast<lua_Integer>(targetID));
        lua_pushinteger(L, static_cast<lua_Integer>(damage));
        lua_pushinteger(L, static_cast<lua_Integer>(attackerID));
        int result = lua_pcall(L, 3, 1, 0);
        bool success = false;
        if (result == LUA_OK && lua_isboolean(L, -1)) {
            success = lua_toboolean(L, -1) != 0;
        } else if (result != LUA_OK) {
            const char* err = lua_tostring(L, -1);
            LOG_ERROR("LevelLoader", "DamageEntity error: %s", err ? err : "unknown");
        }
        lua_pop(L, 1);
        return success;
    }

    LevelLoader* LevelLoader::GetLevelLoader(lua_State* L) {
        lua_getglobal(L, "__level_loader_ptr");
        LevelLoader* loader = static_cast<LevelLoader*>(lua_touserdata(L, -1));
        lua_pop(L, 1);
        return loader;
    }

    // ========================================================================
    // API REGISTRATION
    // ========================================================================

    void LevelLoader::RegisterLevelAPI() {
        LOG_INFO("LevelLoader", "Registering Level API...");

        Framework::ScriptAPIRegistry::RegisterAll(L);
        
        LOG_INFO("LevelLoader", "API registered");
    }

    // ========================================================================
    // LUA C API IMPLEMENTATIONS
    // ========================================================================

    // --- Logging ---

    int LevelLoader::Lua_Log(lua_State* L) {
        const char* message = luaL_checkstring(L, 1);
        LOG_INFO("LuaLevel", "%s", message);
        return 0;
    }

    // --- Camera ---

    int LevelLoader::Lua_SetCameraPosition(lua_State* L) {
        LevelLoader* loader = GetLevelLoader(L);
        if (!loader || !loader->graphicsSystem) return 0;

        float x = luaL_checknumber(L, 1);
        float y = luaL_checknumber(L, 2);
        float z = luaL_checknumber(L, 3);

        loader->graphicsSystem->SetCameraPosition(glm::vec3(x, y, z));
        return 0;
    }

    int LevelLoader::Lua_SetCameraZoom(lua_State* L) {
        LevelLoader* loader = GetLevelLoader(L);
        if (!loader || !loader->graphicsSystem) return 0;

        float zoom = luaL_checknumber(L, 1);
        loader->graphicsSystem->SetCameraZoom(zoom);
        return 0;
    }

    int LevelLoader::Lua_SetCameraFollowTarget(lua_State* L) {
        std::cout << "[LevelLoader] SetCameraFollowTarget() called from Lua" << std::endl;

        LevelLoader* loader = GetLevelLoader(L);
        if (!loader || !loader->graphicsSystem) {
            std::cout << "[LevelLoader] ERROR: No graphics system!" << std::endl;
            return 0;
        }

        int entityID = static_cast<int>(luaL_checknumber(L, 1));
        Entity targetEntity(static_cast<uint32_t>(entityID));

        std::cout << "[LevelLoader] Setting camera follow target to Entity " << entityID << std::endl;

        loader->graphicsSystem->SetFollowTarget(targetEntity);

        std::cout << "[LevelLoader] Camera follow target updated successfully!" << std::endl;
        return 0;
    }

    int LevelLoader::Lua_GetFramebufferSize(lua_State* L) {
        LevelLoader* loader = GetLevelLoader(L);
        if (!loader || !loader->coreEngine) {
            lua_pushinteger(L, 0);
            lua_pushinteger(L, 0);
            return 2;
        }

        auto* windowSystem = loader->coreEngine->GetWindowSystem();
        if (!windowSystem) {
            lua_pushinteger(L, 0);
            lua_pushinteger(L, 0);
            return 2;
        }

        int width, height;
        glfwGetFramebufferSize(windowSystem->GetWindow(), &width, &height);

        lua_pushinteger(L, width);
        lua_pushinteger(L, height);
        return 2;
    }

    // --- Engine Control ---

    int LevelLoader::Lua_SetEnginePlayState(lua_State* L) {
        LevelLoader* loader = GetLevelLoader(L);
        if (!loader || !loader->coreEngine) return 0;

        bool playing = lua_toboolean(L, 1);
        loader->coreEngine->SetPlaying(playing);
        return 0;
    }

    int LevelLoader::Lua_SetNextGameState(lua_State* L) {
        const char* stateName = luaL_checkstring(L, 1);

        // Check if ImGui is currently enabled - if so, preserve editor mode for next level
        LevelLoader* loader = GetLevelLoader(L);
        if (loader && loader->coreEngine) {
            auto* imgui = loader->coreEngine->GetImGuiSystem();
            if (imgui && imgui->IsEnabled()) {
                extern bool g_loadAsEditorMode;
                g_loadAsEditorMode = true;
                LOG_INFO("LevelLoader", "ImGui enabled - next level will load in editor mode");
            }
            // Check if game is currently playing - preserve this state for next level
            if (loader->coreEngine->IsPlaying()) {
                extern bool g_preservePlayingState;
                g_preservePlayingState = true;
                LOG_INFO("LevelLoader", "Game is playing - next level will start in playing state");
            }
        }

        const int resolvedState = ConfigReader::ResolveStateName(stateName, -1);
        if (resolvedState < 0) {
            LOG_WARN("LevelLoader", "Unknown game state: %s", stateName);
            return 0;
        }

        // Preserve existing restart behavior for in-place Level 3 transition.
        if (resolvedState == LEVEL_3 && current == LEVEL_3) {
            next = GS_RESTART;
        }
        else {
            next = resolvedState;
        }

        return 0;
    }

    // --- ImGui ---

    int LevelLoader::Lua_DisableImGui(lua_State* L) {
        LevelLoader* loader = GetLevelLoader(L);
        if (!loader || !loader->coreEngine) return 0;

        auto* imgui = loader->coreEngine->GetImGuiSystem();
        if (imgui) {
            imgui->Disable();
        }
        return 0;
    }

    int LevelLoader::Lua_EnableImGui(lua_State* L) {
        LevelLoader* loader = GetLevelLoader(L);
        if (!loader || !loader->coreEngine) return 0;

        auto* imgui = loader->coreEngine->GetImGuiSystem();
        if (imgui) {
            imgui->Enable();
        }
        return 0;
    }

    // --- Animation ---

    int LevelLoader::Lua_LoadAnimationConfig(lua_State* L) {
        LevelLoader* loader = GetLevelLoader(L);
        if (!loader || !loader->coreEngine) {
            LOG_ERROR("LUA_ANIM", "Invalid loader state");
            return 0;
        }

        const char* configPath = luaL_checkstring(L, 1);
        LOG_INFO("LUA_ANIM", "=== LoadAnimationConfig called: '%s' ===", configPath);

        auto* animSys = loader->coreEngine->GetAnimationSystem();
        if (!animSys) {
            LOG_ERROR("LUA_ANIM", "AnimationSystem not available");
            return 0;
        }

        animSys->LoadAnimationConfig(configPath);
        LOG_INFO("LUA_ANIM", " Animation config loaded from '%s'", configPath);

        return 0;
    }

    int LevelLoader::Lua_LoadPlayerAnimation(lua_State* L) {
        LevelLoader* loader = GetLevelLoader(L);
        if (!loader || !loader->coreEngine) {
            LOG_ERROR("LOAD_ANIM", "Invalid loader state");
            return 0;
        }

        auto* em = loader->coreEngine->GetEntityManager();
        if (!em) {
            LOG_ERROR("LOAD_ANIM", "EntityManager not available");
            return 0;
        }

        const char* animName = luaL_checkstring(L, 1);
        LOG_INFO("LOAD_ANIM", "=== LoadPlayerAnimation called: '%s' ===", animName);

        // Find ALL player entities by tag
        auto players = Framework::FindAllByTag(em, "Player");
        for (Entity e : players) {
            LOG_INFO("LOAD_ANIM", "Found player entity: %u", e.GetID());
        }

        if (players.empty()) {
            LOG_ERROR("LOAD_ANIM", "No players found!");
            return 0;
        }

        LOG_INFO("LOAD_ANIM", "Processing animations for %zu players", players.size());

        auto* animSys = loader->coreEngine->GetAnimationSystem();
        auto* gfx = loader->graphicsSystem;

        if (!animSys || !gfx) {
            LOG_ERROR("LOAD_ANIM", "AnimationSystem or GraphicsSystem not available");
            return 0;
        }

        // Apply animation to ALL players
        for (Entity player : players) {
            LOG_INFO("LOAD_ANIM", "--- Processing player %u ---", player.GetID());

            // Check player's current material
            if (em->HasComponent<MeshRenderer>(player)) {
                auto& mr = em->GetComponent<MeshRenderer>(player);
                LOG_INFO("LOAD_ANIM", "  Material handle: %u", mr.material.GetID());
            }

            // Add SpriteAnimation component if not present
            if (!em->HasComponent<SpriteAnimation>(player)) {
                em->AddComponent<SpriteAnimation>(player);
                LOG_INFO("LOAD_ANIM", "  Added SpriteAnimation component");
            }
            else {
                LOG_INFO("LOAD_ANIM", "  Already has SpriteAnimation");
            }

            // Get animation component
            auto& anim = em->GetComponent<SpriteAnimation>(player);
            anim.playing = true;

            // Load animation
            animSys->LoadAnimation(player, anim, gfx, animName);
            LOG_INFO("LOAD_ANIM", "  Animation loaded:");
            LOG_INFO("LOAD_ANIM", "    - Name: '%s'", anim.animName.c_str());
            LOG_INFO("LOAD_ANIM", "    - Grid: %dx%d", anim.rows, anim.columns);
            LOG_INFO("LOAD_ANIM", "    - Frames: %d", anim.frameCount);
            LOG_INFO("LOAD_ANIM", "    - SpriteSheet: %u", anim.spriteSheet.GetID());
            LOG_INFO("LOAD_ANIM", "    - Playing: %d", anim.playing);
        }

        LOG_INFO("LOAD_ANIM", "=== Successfully loaded animations for all %zu players ===", players.size());
        return 0;
    }

    // ========================================================================
    // UNIFIED ANIMATION LOADING API
    // ========================================================================

    /**
     * @brief Load animation for a specific entity
     * Lua usage: LoadAnimationForEntity(entityID, animName)
     * Example: LoadAnimationForEntity(player1, "Warrior")
     */
    int LevelLoader::Lua_LoadAnimationForEntity(lua_State* L) {
        LevelLoader* loader = GetLevelLoader(L);
        if (!loader || !loader->coreEngine) {
            LOG_ERROR("ANIM_API", "Invalid loader state");
            return 0;
        }

        auto* em = loader->coreEngine->GetEntityManager();
        if (!em) {
            LOG_ERROR("ANIM_API", "EntityManager not available");
            return 0;
        }

        // Get parameters
        lua_Integer entityID = luaL_checkinteger(L, 1);
        const char* animName = luaL_checkstring(L, 2);

        Entity entity(static_cast<uint32_t>(entityID));

        LOG_INFO("ANIM_API", "Loading animation '%s' for entity %u", animName, entityID);

        // Add SpriteAnimation component if not present
        if (!em->HasComponent<SpriteAnimation>(entity)) {
            em->AddComponent<SpriteAnimation>(entity);
            LOG_INFO("ANIM_API", "  Added SpriteAnimation component to entity %u", entityID);
        }

        auto& anim = em->GetComponent<SpriteAnimation>(entity);
        anim.playing = true;

        // Load animation
        auto* animSys = loader->coreEngine->GetAnimationSystem();
        auto* gfx = loader->graphicsSystem;

        if (animSys && gfx) {
            animSys->LoadAnimation(entity, anim, gfx, animName);
            LOG_INFO("ANIM_API", "  Animation '%s' loaded successfully for entity %u", animName, entityID);
        }
        else {
            LOG_ERROR("ANIM_API", "AnimationSystem or GraphicsSystem not available");
        }

        return 0;
    }

    /**
     * @brief Load SAME animation for ALL players
     * Lua usage: LoadAnimationForAllPlayers(animName)
     * Example: LoadAnimationForAllPlayers("Warrior") -- All 3 players use Warrior animations
     */
    int LevelLoader::Lua_LoadAnimationForAllPlayers(lua_State* L) {
        LevelLoader* loader = GetLevelLoader(L);
        if (!loader || !loader->coreEngine) {
            LOG_ERROR("ANIM_API", "Invalid loader state");
            return 0;
        }

        auto* em = loader->coreEngine->GetEntityManager();
        if (!em) {
            LOG_ERROR("ANIM_API", "EntityManager not available");
            return 0;
        }

        const char* animName = luaL_checkstring(L, 1);
        LOG_INFO("ANIM_API", "=== LoadAnimationForAllPlayers: '%s' ===", animName);

        // Find ALL players by tag
        auto players = Framework::FindAllByTag(em, "Player");
        for (Entity e : players) {
            LOG_INFO("ANIM_API", "  Found player entity: %u", e.GetID());
        }

        if (players.empty()) {
            LOG_ERROR("ANIM_API", "No players found!");
            return 0;
        }

        auto* animSys = loader->coreEngine->GetAnimationSystem();
        auto* gfx = loader->graphicsSystem;

        if (!animSys || !gfx) {
            LOG_ERROR("ANIM_API", "AnimationSystem or GraphicsSystem not available");
            return 0;
        }

        // Load animation for EACH player
        for (Entity player : players) {
            // Add SpriteAnimation component if not present
            if (!em->HasComponent<SpriteAnimation>(player)) {
                em->AddComponent<SpriteAnimation>(player);
            }

            auto& anim = em->GetComponent<SpriteAnimation>(player);
            anim.playing = true;

            animSys->LoadAnimation(player, anim, gfx, animName);
            LOG_INFO("ANIM_API", "  Animation '%s' loaded for player %u", animName, player.GetID());
        }

        LOG_INFO("ANIM_API", "Successfully loaded animation '%s' for %zu players", animName, players.size());
        return 0;
    }

    /**
     * @brief Load SAME animation for ALL enemies
     * Lua usage: LoadAnimationForAllEnemies(animName)
     * Example: LoadAnimationForAllEnemies("Skeleton") -- All enemies use Skeleton animations
     */
    int LevelLoader::Lua_LoadAnimationForAllEnemies(lua_State* L) {
        LevelLoader* loader = GetLevelLoader(L);
        if (!loader || !loader->coreEngine) {
            LOG_ERROR("ANIM_API", "Invalid loader state");
            return 0;
        }

        auto* em = loader->coreEngine->GetEntityManager();
        if (!em) {
            LOG_ERROR("ANIM_API", "EntityManager not available");
            return 0;
        }

        const char* animName = luaL_checkstring(L, 1);
        LOG_INFO("ANIM_API", "=== LoadAnimationForAllEnemies: '%s' ===", animName);

        // Find ALL enemies by tag
        auto enemies = Framework::FindAllByTag(em, "Enemy");
        for (Entity e : enemies) {
            LOG_INFO("ANIM_API", "  Found enemy entity: %u", e.GetID());
        }

        if (enemies.empty()) {
            LOG_WARN("ANIM_API", "No enemies found (this is okay if level has no enemies)");
            return 0;
        }

        auto* animSys = loader->coreEngine->GetAnimationSystem();
        auto* gfx = loader->graphicsSystem;

        if (!animSys || !gfx) {
            LOG_ERROR("ANIM_API", "AnimationSystem or GraphicsSystem not available");
            return 0;
        }

        // Load animation for EACH enemy
        for (Entity enemy : enemies) {
            // Add SpriteAnimation component if not present
            if (!em->HasComponent<SpriteAnimation>(enemy)) {
                em->AddComponent<SpriteAnimation>(enemy);
            }

            auto& anim = em->GetComponent<SpriteAnimation>(enemy);
            anim.playing = true;

            animSys->LoadAnimation(enemy, anim, gfx, animName);
            LOG_INFO("ANIM_API", "  Animation '%s' loaded for enemy %u", animName, enemy.GetID());
        }

        LOG_INFO("ANIM_API", "Successfully loaded animation '%s' for %zu enemies", animName, enemies.size());
        return 0;
    }

    // ========================================================================
    // SCROLL ANIMATION API (for TurnScrollUI)
    // ========================================================================

    /**
     * @brief Play animation by name on an entity
     * Lua usage: PlayAnimationByName(entityID, animName, loop)
     */
    int LevelLoader::Lua_PlayAnimationByName(lua_State* L) {
        LevelLoader* loader = GetLevelLoader(L);
        if (!loader || !loader->coreEngine) {
            return 0;
        }

        auto* em = loader->coreEngine->GetEntityManager();
        auto* animSys = loader->coreEngine->GetAnimationSystem();
        auto* gfx = loader->graphicsSystem;
        if (!em || !animSys || !gfx) {
            return 0;
        }

        lua_Integer entityID = luaL_checkinteger(L, 1);
        const char* animName = luaL_checkstring(L, 2);
        const std::string animNameStr = animName ? std::string(animName) : std::string();
        const bool isScrollAnim = animNameStr.rfind("Scroll", 0) == 0;
        const bool hasLoopArg = (lua_gettop(L) >= 3) && !lua_isnil(L, 3);
        const bool loop = hasLoopArg ? lua_toboolean(L, 3) != 0 : false;

        Entity e(static_cast<EntityID>(entityID));
        if (!e.IsValid()) {
            return 0;
        }

        if (!em->HasComponent<SpriteAnimation>(e)) {
            em->AddComponent<SpriteAnimation>(e);
        }

        auto& anim = em->GetComponent<SpriteAnimation>(e);
        anim.animName = animName;
        anim.playing = true;

        animSys->LoadAnimation(e, anim, gfx, animName);

        if (hasLoopArg) {
            anim.loop = loop;
        }

        anim.currentFrame = 0;
        anim.elapsedTime = 0.0f;

        // Special handling for scroll animations - keep manual control
        if (isScrollAnim) {
            anim.group = AnimGroup::Idle;
            anim.direction = AnimDirection::None;
            anim.playing = false;
            anim.loop = false;
            anim.currentFrame = 0;
            anim.elapsedTime = 0.0f;
        }

        return 0;
    }

    /**
     * @brief Set current animation frame
     * Lua usage: SetAnimationFrame(entityID, frame)
     */
    int LevelLoader::Lua_SetAnimationFrame(lua_State* L) {
        LevelLoader* loader = GetLevelLoader(L);
        if (!loader || !loader->coreEngine) {
            return 0;
        }

        auto* em = loader->coreEngine->GetEntityManager();
        if (!em) return 0;

        lua_Integer entityID = luaL_checkinteger(L, 1);
        lua_Integer frame = luaL_checkinteger(L, 2);

        Entity e(static_cast<EntityID>(entityID));
        if (!e.IsValid() || !em->HasComponent<SpriteAnimation>(e)) {
            return 0;
        }

        auto& anim = em->GetComponent<SpriteAnimation>(e);
        int maxFrame = anim.frameCount > 0 ? (anim.frameCount - 1) : 0;
        int clamped = static_cast<int>(frame);
        if (clamped < 0) clamped = 0;
        if (clamped > maxFrame) clamped = maxFrame;

        anim.currentFrame = clamped;
        anim.elapsedTime = 0.0f;
        return 0;
    }

    /**
     * @brief Get current animation frame
     * Lua usage: frame = GetAnimationFrame(entityID)
     */
    int LevelLoader::Lua_GetAnimationFrame(lua_State* L) {
        LevelLoader* loader = GetLevelLoader(L);
        if (!loader || !loader->coreEngine) {
            lua_pushinteger(L, 0);
            return 1;
        }

        auto* em = loader->coreEngine->GetEntityManager();
        if (!em) {
            lua_pushinteger(L, 0);
            return 1;
        }

        lua_Integer entityID = luaL_checkinteger(L, 1);
        Entity e(static_cast<EntityID>(entityID));
        if (!e.IsValid() || !em->HasComponent<SpriteAnimation>(e)) {
            lua_pushinteger(L, 0);
            return 1;
        }

        auto& anim = em->GetComponent<SpriteAnimation>(e);
        lua_pushinteger(L, anim.currentFrame);
        return 1;
    }

    /**
     * @brief Get total frame count for animation
     * Lua usage: count = GetAnimationFrameCount(entityID)
     */
    int LevelLoader::Lua_GetAnimationFrameCount(lua_State* L) {
        LevelLoader* loader = GetLevelLoader(L);
        if (!loader || !loader->coreEngine) {
            lua_pushinteger(L, 0);
            return 1;
        }

        auto* em = loader->coreEngine->GetEntityManager();
        if (!em) {
            lua_pushinteger(L, 0);
            return 1;
        }

        lua_Integer entityID = luaL_checkinteger(L, 1);
        Entity e(static_cast<EntityID>(entityID));
        if (!e.IsValid() || !em->HasComponent<SpriteAnimation>(e)) {
            lua_pushinteger(L, 0);
            return 1;
        }

        auto& anim = em->GetComponent<SpriteAnimation>(e);
        lua_pushinteger(L, anim.frameCount);
        return 1;
    }

} // namespace Framework
