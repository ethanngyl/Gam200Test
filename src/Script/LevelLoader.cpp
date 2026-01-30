/*
===============================================================================
 File:          LevelLoader.cpp
 Author:        GE YONGQI
 Email:         yongqi.ge@digipen.edu
 Date:          2025-11-13
 Contribution:  100%
 ------------------------------------------------------------------------------
  Lua-based Level Loading System Implementation

  Purpose:
  Core implementation of the LevelLoader system that manages Lua-scripted
  game levels and provides C++ API bindings for Lua scripts.

  Key Features:
  - Singleton pattern for global access
  - Lua state lifecycle management (create, reset, destroy)
  - Level lifecycle (Load, Update, Draw, Unload)
  - Hot-reload capability for rapid iteration
  - Editor mode toggle (F1 key) with visual feedback
  - Extensive C API for Lua (60+ functions)

  API Categories:
  - Logging: Log messages to console
  - Camera: Position, zoom, framebuffer queries
  - Engine: Play state, game state transitions
  - ImGui: Enable/disable overlay
  - Pause: Toggle pause, query pause state
  - Audio: Play/stop sounds, volume control
  - UI: Button creation, text rendering
  - Input: Keyboard queries
  - JSON: Configuration file loading
  - Entities: Sprite spawning, manipulation, destruction
  - TileMap: Grid-based level loading
  - Player/Enemy: Grid movement, AP management, combat
  - Scripts: Component management

  Editor Mode:
  - Lua_ToggleEditor(): Toggle editor mode on/off
  - Lua_IsEditorEnabled(): Query current editor mode state
  - Lua_SetEditorMode(): Directly set editor mode
  - When enabled, buttons are grayed out and ImGui is shown
===============================================================================
*/

#include "Precompiled.h"
#include "LevelLoader.h"
#include "Core.h"
#include "UISystem.h"
#include "Audio/AudioSystem.h"
#include "GraphicsSystemV2.h"
#include "ImguiSystem.h"
#include "GameStateList.h"
#include "Pause/GlobalPauseManager.h"

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
        std::string currentPath = lua_tostring(L, -1);
        std::string newPath = currentPath + ";assets/scripts/?.lua";
        lua_pop(L, 1);  // Pop old path
        lua_pushstring(L, newPath.c_str());
        lua_setfield(L, -2, "path");
        lua_pop(L, 1);  // Pop package table

        LOG_INFO("LevelLoader", "Added assets/scripts/ to Lua module path");

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
            } else {
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
            } else {
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
            } else {
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
            auto* em = coreEngine->GetEntityManager();
            if (em)
            {
                em->ClearAllEntities();
                em->ResetEntityIDCounter();
            }
        }

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

    void LevelLoader::UpdateCurrentLevel(float dt) {
        if (!levelLoaded || !L) return;

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

        // Logging
        lua_register(L, "Log", Lua_Log);

        // Camera
        lua_register(L, "SetCameraPosition", Lua_SetCameraPosition);
        lua_register(L, "SetCameraZoom", Lua_SetCameraZoom);
        lua_register(L, "SetCameraFollowTarget", Lua_SetCameraFollowTarget);
        lua_register(L, "GetFramebufferSize", Lua_GetFramebufferSize);

        // Engine control
        lua_register(L, "SetEnginePlayState", Lua_SetEnginePlayState);
        lua_register(L, "SetNextGameState", Lua_SetNextGameState);

        // ImGui
        lua_register(L, "ToggleEditor", Lua_ToggleEditor);
        lua_register(L, "IsEditorEnabled", Lua_IsEditorEnabled);
        lua_register(L, "SetEditorMode", Lua_SetEditorMode);
        lua_register(L, "DisableImGui", Lua_DisableImGui);
        lua_register(L, "EnableImGui", Lua_EnableImGui);

        // Pause control
        lua_register(L, "TogglePause", Lua_TogglePause);
        lua_register(L, "IsPaused", Lua_IsPaused);

        // Audio
        lua_register(L, "PlaySound", Lua_PlaySound);
        lua_register(L, "StopSound", Lua_StopSound);
        lua_register(L, "StopAllSounds", Lua_StopAllSounds);
        lua_register(L, "UpdateAudio", Lua_UpdateAudio);
        lua_register(L, "SetMasterVolume", Lua_SetMasterVolume);

        // UI Buttons
        lua_register(L, "CreateButton", Lua_CreateButton);
        lua_register(L, "ClearAllButtons", Lua_ClearAllButtons);
        lua_register(L, "DrawButtonText", Lua_DrawButtonText);
        lua_register(L, "DrawText", Lua_DrawText);

        // Input
        lua_register(L, "IsKeyDown", Lua_IsKeyDown);

        // JSON and Level Loading
        lua_register(L, "LoadJSON", Lua_LoadJSON);
        lua_register(L, "LoadTileMap", Lua_LoadTileMap);

        // AP Indicator / Entity Management
        lua_register(L, "SpawnSprite", Lua_SpawnSprite);
        lua_register(L, "SpawnAnimatedSprite", Lua_SpawnAnimatedSprite);
        lua_register(L, "SetSpriteAnimationSheet", Lua_SetSpriteAnimationSheet);
        lua_register(L, "SetSpriteColor", Lua_SetSpriteColor);
        lua_register(L, "SetSpriteGray", Lua_SetSpriteGray);
        lua_register(L, "SetSpriteTexture", Lua_SetSpriteTexture);
        lua_register(L, "SetSpritePosition", Lua_SetSpritePosition);
        lua_register(L, "SetSpriteVisibility", Lua_SetSpriteVisibility);
        lua_register(L, "SetSpriteBlendMode", Lua_SetSpriteBlendMode);
        lua_register(L, "SetSpriteFilterMode", Lua_SetSpriteFilterMode);
        lua_register(L, "DestroyEntity", Lua_DestroyEntity);
        lua_register(L, "ClearAllEntities", Lua_ClearAllEntities);

        lua_register(L, "GetPlayerAP", Lua_GetPlayerAP);
        lua_register(L, "GetCameraPosition", Lua_GetCameraPosition);
        lua_register(L, "GetPlayerAttackAP", Lua_GetPlayerAttackAP);
        lua_register(L, "GetPlayerHP", Lua_GetPlayerHP);

        // Enemy AI Configuration
        lua_register(L, "FindPlayer", Lua_FindPlayer);
        lua_register(L, "GetAllPlayers", Lua_GetAllPlayers);
        lua_register(L, "GetAllEnemies", Lua_GetAllEnemies);
        lua_register(L, "SetEnemyTarget", Lua_SetEnemyTarget);
        lua_register(L, "GetCurrentTurn", Lua_GetCurrentTurn);
        lua_register(L, "GetChestProgress", Lua_GetChestProgress);

        // Animation
        lua_register(L, "LoadAnimationConfig", Lua_LoadAnimationConfig);
        lua_register(L, "LoadPlayerAnimation", Lua_LoadPlayerAnimation);

        // Animation Control API
        lua_register(L, "SetAnimationGroup", Lua_SetAnimationGroup);
        lua_register(L, "SetAnimationDirection", Lua_SetAnimationDirection);
        lua_register(L, "SetAnimationFlipX", Lua_SetAnimationFlipX);
        lua_register(L, "SetAnimationPlaying", Lua_SetAnimationPlaying);
        lua_register(L, "SetAnimationLoop", Lua_SetAnimationLoop);
        lua_register(L, "SetAnimationFrameRange", Lua_SetAnimationFrameRange);
        lua_register(L, "GetAnimationGroup", Lua_GetAnimationGroup);
        lua_register(L, "GetEntityMovementDirection", Lua_GetEntityMovementDirection);

        // Party System - Entity-Based APIs
        lua_register(L, "GetEntityAP", Lua_GetEntityAP);
        lua_register(L, "ConsumeEntityAP", Lua_ConsumeEntityAP);
        lua_register(L, "RefillEntityAP", Lua_RefillEntityAP);
        lua_register(L, "GetEntityHP", Lua_GetEntityHP);
        lua_register(L, "SetEntityHP", Lua_SetEntityHP);
        lua_register(L, "IsActiveCharacter", Lua_IsActiveCharacter);

        // Script Component Management
        lua_register(L, "AddScriptComponentToEntity", Lua_AddScriptComponentToEntity);
        lua_register(L, "RemoveScriptComponentFromEntity", Lua_RemoveScriptComponentFromEntity);

        // Player Grid Movement API
        lua_register(L, "GetPlayerGridPosition", Lua_GetPlayerGridPosition);
        lua_register(L, "IsValidGridPosition", Lua_IsValidGridPosition);
        lua_register(L, "IsWalkableTile", Lua_IsWalkableTile);
        lua_register(L, "MovePlayerToTile", Lua_MovePlayerToTile);
        lua_register(L, "SetGridMovementEnabled", Lua_SetGridMovementEnabled);
        lua_register(L, "ShowTileBorder", Lua_ShowTileBorder);
        lua_register(L, "PulseTile", Lua_PulseTile);
        lua_register(L, "ConsumePlayerAP", Lua_ConsumePlayerAP);
        lua_register(L, "RefillPlayerAP", Lua_RefillPlayerAP);
        lua_register(L, "GetTurnIndex", Lua_GetTurnIndex);
        lua_register(L, "EndPlayerTurn", Lua_EndPlayerTurn);
        lua_register(L, "EndEnemyTurn", Lua_EndEnemyTurn);
        lua_register(L, "SetPlayerFlipX", Lua_SetPlayerFlipX);
        lua_register(L, "HasChestAtTile", Lua_HasChestAtTile);
        lua_register(L, "CollectChest", Lua_CollectChest);
        lua_register(L, "HasGoalAtTile", Lua_HasGoalAtTile);

        // Enemy/Entity API
        lua_register(L, "GetEnemyAP", Lua_GetEnemyAP);
        lua_register(L, "RefillEnemyAP", Lua_RefillEnemyAP);
        lua_register(L, "GetEntityGridPosition", Lua_GetEntityGridPosition);
        lua_register(L, "GetEntityWorldPosition", Lua_GetEntityWorldPosition);
        lua_register(L, "MoveEntityToTile", Lua_MoveEntityToTile);
        lua_register(L, "ConsumeEnemyAP", Lua_ConsumeEnemyAP);
        lua_register(L, "DamageEntity", Lua_DamageEntity);
        lua_register(L, "FindPathToTarget", Lua_FindPathToTarget);

        // Grid Conversion API
        lua_register(L, "TileToWorld", Lua_TileToWorld);

        // Entity-specific APIs (proper naming)
        lua_register(L, "GetEntityAP", Lua_GetEntityAP);
        lua_register(L, "GetEntityHP", Lua_GetEntityHP);
        lua_register(L, "SetEntityHP", Lua_SetEntityHP);
        lua_register(L, "RefillEntityAP", Lua_RefillEntityAP);
        lua_register(L, "ConsumeEntityAP", Lua_ConsumeEntityAP);
        lua_register(L, "SetActiveCharacter", Lua_SetActiveCharacter);
        lua_register(L, "IsActiveCharacter", Lua_IsActiveCharacter);

        lua_register(L, "ToggleEditorMode", lua_ToggleEditorMode);
        lua_register(L, "IsEditorMode", lua_IsEditorMode);
        lua_register(L, "ShouldDisableGameplay", Lua_ShouldDisableGameplay);

        // Save/Load API
        lua_register(L, "SaveSceneToJSON", Lua_SaveSceneToJSON);
        lua_register(L, "LoadSceneFromJSON", Lua_LoadSceneFromJSON);
        lua_register(L, "AutoSaveScene", Lua_AutoSaveScene);
        lua_register(L, "LoadAutoSave", Lua_LoadAutoSave);
        lua_register(L, "HasAutoSave", Lua_HasAutoSave);
        lua_register(L, "ClearAutoSave", Lua_ClearAutoSave);

        // Procedural Map API
        lua_register(L, "LoadProceduralMap", Lua_LoadProceduralMap);

        // Entity Spawning API
        lua_register(L, "SpawnPlayerAt", Lua_SpawnPlayerAt);
        lua_register(L, "SpawnEnemyAt", Lua_SpawnEnemyAt);
        lua_register(L, "SpawnChestAt", Lua_SpawnChestAt);
        lua_register(L, "SpawnGoalAt", Lua_SpawnGoalAt);
        lua_register(L, "TileToWorld", Lua_TileToWorld);

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

        // Map state names to enum values
        if (strcmp(stateName, "Level_select") == 0) {
            next = Level_select;
        }
        if (strcmp(stateName, "TUTORIAL") == 0) {
            next = TUTORIAL;
        }
        else if (strcmp(stateName, "LEVEL_2") == 0) {
            next = LEVEL_2;
        }
        else if (strcmp(stateName, "LEVEL_3") == 0) {
            next = LEVEL_3;
        }
        else if (strcmp(stateName, "LEVEL_END") == 0) {
            next = LEVEL_END;
        }
        else if (strcmp(stateName, "mainMenu") == 0) {
            next = mainMenu;
        }
        else if (strcmp(stateName, "GS_QUIT") == 0) {
            next = GS_QUIT;
        }
        else {
            LOG_WARN("LevelLoader", "Unknown game state: %s", stateName);
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

        // Find player entity (CircleCollider)
        Entity player{ INVALID_ENTITY };
        for (Entity e : em->GetAllEntities()) {
            if (em->HasComponent<CircleCollider>(e)) {
                player = e;
                LOG_INFO("LOAD_ANIM", "Found player entity: %u", player.GetID());
                break;
            }
        }

        if (player.GetID() == INVALID_ENTITY) {
            LOG_ERROR("LOAD_ANIM", "Player not found!");
            return 0;
        }

        // Check player's current material
        if (em->HasComponent<MeshRenderer>(player)) {
            auto& mr = em->GetComponent<MeshRenderer>(player);
            LOG_INFO("LOAD_ANIM", "Player material handle: %u", mr.material.GetID());
        }

        // Add SpriteAnimation component if not present
        if (!em->HasComponent<SpriteAnimation>(player)) {
            em->AddComponent<SpriteAnimation>(player);
            LOG_INFO("LOAD_ANIM", " Added SpriteAnimation component");
        }
        else {
            LOG_INFO("LOAD_ANIM", "Player already has SpriteAnimation");
        }

        // Get animation component
        auto& anim = em->GetComponent<SpriteAnimation>(player);
        anim.playing = true;

        // Load animation
        auto* animSys = loader->coreEngine->GetAnimationSystem();
        auto* gfx = loader->graphicsSystem;

        if (animSys && gfx) {
            animSys->LoadAnimation(player, anim, gfx, animName);
            LOG_INFO("LOAD_ANIM", " Animation loaded:");
            LOG_INFO("LOAD_ANIM", "  - Name: '%s'", anim.animName.c_str());
            LOG_INFO("LOAD_ANIM", "  - Grid: %dx%d", anim.rows, anim.columns);
            LOG_INFO("LOAD_ANIM", "  - Frames: %d", anim.frameCount);
            LOG_INFO("LOAD_ANIM", "  - SpriteSheet: %u", anim.spriteSheet.GetID());
            LOG_INFO("LOAD_ANIM", "  - Playing: %d", anim.playing);
        }
        else {
            LOG_ERROR("LOAD_ANIM", "AnimationSystem or GraphicsSystem not available");
        }

        return 0;
    }

} // namespace Framework