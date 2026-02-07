/*
===============================================================================
File:        LevelLoader.h
Author:      GE YONGQI, Sim Kah Yan
Email:       yongqi.ge@digipen.edu; kahyan.sim@digipen.edu
Date:        2026-02-04 (yyyy-mm-dd)
Contribution: GE YONGQI (remaining); Sim Kah Yan 3% (8 lines of 309 total)
-------------------------------------------------------------------------------
Brief:
Declaration of LevelLoader: singleton for loading game levels from Lua scripts
(OnInit/OnUpdate/OnDraw/OnDestroy), hot-reload, and F1 editor mode support.

Purpose (GE YONGQI):
  Unified system for loading entire game levels/states from Lua instead of
  hardcoding in C++. Enables: hot-reloadable level configs, designer-friendly
  level creation (no C++ compile), centralized level management, A/B testing
  of layouts, F1 editor mode toggle.

Design:
  Each level = one Lua script with lifecycle: OnInit() (setup entities, UI,
  audio), OnUpdate(dt), OnDraw(), OnDestroy() (cleanup). Singleton GetInstance();
  Initialize(engine), Shutdown; LoadLevel(scriptPath, isEditorMode),
  UnloadCurrentLevel, ReloadCurrentLevel, ResetLuaState; UpdateCurrentLevel(dt),
  DrawCurrentLevel; IsLevelLoaded, GetCurrentLevelPath. Lua state, RegisterLevelAPI,
  60+ static C API declarations (Log, Camera, Engine, ImGui, Pause, Audio, UI,
  Input, JSON, TileMap, Sprites, Player/Enemy, Animation, Party, Grid, Save/Load,
  Entity spawning, Editor). GetLevelLoader(L) helper.

Editor Mode:
  F1 toggles game/editor; in editor, buttons grayed out/disabled, ImGui overlay
  enabled for debugging; game state can be paused/unpaused independently.

Usage:
  Instead of: mainMenu_Load(), mainMenu_Initialize(), mainMenu_Update(),
  mainMenu_Draw(), mainMenu_Free(), mainMenu_Unload()
  Use: LevelLoader::LoadLevel("assets/scripts/MainMenuLevel.lua"),
  UpdateCurrentLevel(dt), DrawCurrentLevel(), UnloadCurrentLevel()

Safety:
  No ownership of engine/subsystems; pointers cached for API callbacks.
  Lua state created/destroyed in Initialize/Shutdown.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
===============================================================================
*/

#pragma once
#include "Precompiled.h"

extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

namespace Framework {

    // Forward declarations
    class CoreEngine;
    class UISystem;
    class AudioSystem;
    class GraphicsSystemV2;

    /**
     * @class LevelLoader
     * @brief Manages Lua-scripted game levels/states
     *
     * Singleton pattern for convenient global access.
     * Handles level lifecycle and provides C++ API bindings to Lua.
     */
    class LevelLoader {
    public:
        // Singleton access
        static LevelLoader& GetInstance();

        // Initialization
        void Initialize(CoreEngine* engine);
        void Shutdown();

        // Level management
        bool LoadLevel(const std::string& scriptPath, bool isEditorMode = false);
        void UnloadCurrentLevel();
        void ReloadCurrentLevel();  // Hot reload
        void ResetLuaState();       // Complete Lua state reset

        // Lifecycle calls (to be called from GSM or main loop)
        void UpdateCurrentLevel(float dt);
        void DrawCurrentLevel();

        // State queries
        bool IsLevelLoaded() const { return levelLoaded; }
        std::string GetCurrentLevelPath() const { return currentLevelPath; }
        static int Lua_ToggleEditor(lua_State* L);
        static int Lua_IsEditorEnabled(lua_State* L);
        static int Lua_SetEditorMode(lua_State* L);
    private:
        // Singleton (private constructor)
        LevelLoader();
        ~LevelLoader();
        LevelLoader(const LevelLoader&) = delete;
        LevelLoader& operator=(const LevelLoader&) = delete;

        // Lua state management
        lua_State* L = nullptr;
        void CreateLuaState();
        void DestroyLuaState();
        static int Lua_ClearAllEntities(lua_State* L);
        // API registration
        void RegisterLevelAPI();

        // Helper: check if Lua function exists
        bool HasLuaFunction(const char* funcName);

        // Helper: call Lua function safely
        bool CallLuaFunction(const char* funcName, int numArgs = 0);

        // State
        CoreEngine* coreEngine = nullptr;
        bool levelLoaded = false;
        std::string currentLevelPath;

        // Cached subsystem pointers (for fast access in API)
        UISystem* uiSystem = nullptr;
        AudioSystem* audioSystem = nullptr;
        GraphicsSystemV2* graphicsSystem = nullptr;

    public:
        // ====================================================================
        // LUA C API FUNCTIONS
        // ====================================================================
        // These are static because Lua requires C-style function pointers

        // Logging
        static int Lua_Log(lua_State* L);

        // Camera control
        static int Lua_SetCameraPosition(lua_State* L);
        static int Lua_SetCameraZoom(lua_State* L);
        static int Lua_SetCameraFollowTarget(lua_State* L);  // Set which entity camera follows
        static int Lua_GetFramebufferSize(lua_State* L);

        // Engine control
        static int Lua_SetEnginePlayState(lua_State* L);
        static int Lua_SetNextGameState(lua_State* L);

        // ImGui control
        static int Lua_DisableImGui(lua_State* L);
        static int Lua_EnableImGui(lua_State* L);

        // Pause control
        static int Lua_TogglePause(lua_State* L);
        static int Lua_IsPaused(lua_State* L);

        // Audio API
        static int Lua_PlaySound(lua_State* L);
        static int Lua_StopSound(lua_State* L);
        static int Lua_StopAllSounds(lua_State* L);
        static int Lua_UpdateAudio(lua_State* L);
        static int Lua_SetMasterVolume(lua_State* L);
        static int Lua_GetMasterVolume(lua_State* L);
        static int Lua_SaveMasterVolume(lua_State* L);

        // UI Button API
        static int Lua_CreateButton(lua_State* L);
        static int Lua_ClearAllButtons(lua_State* L);
        static int Lua_DrawButtonText(lua_State* L);
        static int Lua_DrawText(lua_State* L);

        // Input API
        static int Lua_IsKeyDown(lua_State* L);
        static int Lua_IsMouseButtonDown(lua_State* L);
        static int Lua_IsMouseButtonPressed(lua_State* L);
        static int Lua_GetMousePosition(lua_State* L);

        // JSON API
        static int Lua_LoadJSON(lua_State* L);

        // TileMap Loading API
        static int Lua_LoadTileMap(lua_State* L);

        // AP Indicator / Entity Management API
        static int Lua_SpawnSprite(lua_State* L);
        static int Lua_SpawnAnimatedSprite(lua_State* L);  // Spawn sprite with animation sheet
        static int Lua_SetSpriteAnimationSheet(lua_State* L);  // Add animation to existing sprite
        static int Lua_SetSpriteColor(lua_State* L);
        static int Lua_TintTile(lua_State* L);  // Tint tile at grid coordinates
        static int Lua_SetSpriteGray(lua_State* L);
        static int Lua_SetSpriteTexture(lua_State* L);
        static int Lua_SetSpritePosition(lua_State* L);
        static int Lua_SetSpriteVisibility(lua_State* L);
        static int Lua_SetSpriteBlendMode(lua_State* L);
        static int Lua_SetSpriteFilterMode(lua_State* L);
        static int Lua_DestroyEntity(lua_State* L);
        static int Lua_GetPlayerAP(lua_State* L);
        static int Lua_GetCameraPosition(lua_State* L);
        static int Lua_FindPlayer(lua_State* L);
        static int Lua_GetAllPlayers(lua_State* L);
        static int Lua_GetAllEnemies(lua_State* L);
        static int Lua_SetEnemyTarget(lua_State* L);
        static int Lua_GetCurrentTurn(lua_State* L);
        static int Lua_GetChestProgress(lua_State* L);
        static int Lua_LoadAnimationConfig(lua_State* L);
        static int Lua_LoadPlayerAnimation(lua_State* L);

        // NEW: Unified Animation Loading API
        static int Lua_LoadAnimationForEntity(lua_State* L);       // Load animation for ONE specific entity
        static int Lua_LoadAnimationForAllPlayers(lua_State* L);   // Load SAME animation for ALL players
        static int Lua_LoadAnimationForAllEnemies(lua_State* L);   // Load SAME animation for ALL enemies

        // Scroll Animation API (for TurnScrollUI)
        static int Lua_PlayAnimationByName(lua_State* L);
        static int Lua_SetAnimationFrame(lua_State* L);
        static int Lua_GetAnimationFrame(lua_State* L);
        static int Lua_GetAnimationFrameCount(lua_State* L);

        // Animation Control API
        static int Lua_SetAnimationGroup(lua_State* L);
        static int Lua_SetAnimationDirection(lua_State* L);
        static int Lua_SetAnimationFlipX(lua_State* L);
        static int Lua_SetAnimationPlaying(lua_State* L);
        static int Lua_SetAnimationLoop(lua_State* L);
        static int Lua_SetAnimationFrameRange(lua_State* L);  // Set animation frame range (startFrame, frameCount)
        static int Lua_GetAnimationGroup(lua_State* L);
        static int Lua_GetEntityMovementDirection(lua_State* L);

        // Party System - Entity-Based APIs
        static int Lua_GetEntityAP(lua_State* L);
        static int Lua_GetEntityAttackAP(lua_State* L);
        static int Lua_ConsumeEntityAP(lua_State* L);
        static int Lua_ConsumeEntityAttackAP(lua_State* L);
        static int Lua_RefillEntityAP(lua_State* L);
        static int Lua_RefillEntityAttackAP(lua_State* L);
        static int Lua_GetEntityHP(lua_State* L);
        static int Lua_SetEntityHP(lua_State* L);
        static int Lua_SetActiveCharacter(lua_State* L);
        static int Lua_IsActiveCharacter(lua_State* L);

        static int Lua_GetPlayerAttackAP(lua_State* L);
        static int Lua_GetPlayerHP(lua_State* L);

        // Player Grid Movement API
        static int Lua_GetPlayerGridPosition(lua_State* L);
        static int Lua_IsValidGridPosition(lua_State* L);
        static int Lua_IsWalkableTile(lua_State* L);
        static int Lua_MovePlayerToTile(lua_State* L);
        static int Lua_ShowTileBorder(lua_State* L);
        static int Lua_PulseTile(lua_State* L);
        static int Lua_ConsumePlayerAP(lua_State* L);
        static int Lua_RefillPlayerAP(lua_State* L);
        static int Lua_GetTurnIndex(lua_State* L);
        static int Lua_EndPlayerTurn(lua_State* L);
        static int Lua_EndEnemyTurn(lua_State* L);
        static int Lua_EndCharacterTurn(lua_State* L);  // Party turn system - bridge to level Lua state
        static int Lua_IsUIAnimating(lua_State* L);     // Check if UI is animating - bridge to level Lua state
        static int Lua_IsInTurnTransition(lua_State* L); // Check if in turn transition cooldown - bridge to level Lua state
        static int Lua_IsTurnScrollPlaying(lua_State* L); // Check if turn scroll animation is playing - bridge to level Lua state
        static int Lua_TriggerAttackAPAnimation(lua_State* L); // Trigger attack AP consume animation - bridge to UIManager
        static int Lua_RestoreAllAttackAPCrystals(lua_State* L); // Restore all attack AP crystals - bridge to UIManager
        static int Lua_SetPlayerFlipX(lua_State* L);
        static int Lua_SetGridMovementEnabled(lua_State* L);
        static int Lua_HasChestAtTile(lua_State* L);
        static int Lua_CollectChest(lua_State* L);
        static int Lua_HasGoalAtTile(lua_State* L);

        // Enemy/Entity API
        static int Lua_GetEnemyAP(lua_State* L);
        static int Lua_RefillEnemyAP(lua_State* L);
        static int Lua_GetEntityGridPosition(lua_State* L);
        static int Lua_GetEntityWorldPosition(lua_State* L);  // Get world coords from Transform
        static int Lua_MoveEntityToTile(lua_State* L);
        static int Lua_ConsumeEnemyAP(lua_State* L);
        static int Lua_DamageEntity(lua_State* L);
        static int Lua_FindPathToTarget(lua_State* L);
        
        // Tile Occupancy API
        static int Lua_SetTileOccupant(lua_State* L);  // Set entity occupying a tile
        static int Lua_GetTileOccupant(lua_State* L);  // Get entity at tile (or 0 if empty)
        static int Lua_IsTileOccupied(lua_State* L);   // Check if tile has an occupant

        // Enemy Turn Management System (C++ Implementation)
        static int Lua_InitializeEnemyTurn(lua_State* L);
        static int Lua_IsActiveEnemy(lua_State* L);
        static int Lua_IsEnemyActionReady(lua_State* L);
        static int Lua_MarkEnemyActionComplete(lua_State* L);
        static int Lua_UpdateEnemyTurnManager(lua_State* L);

        // Grid Conversion API
        static int Lua_TileToWorld(lua_State* L);

        // Script Component Management API
        static int Lua_AddScriptComponentToEntity(lua_State* L);
        static int Lua_RemoveScriptComponentFromEntity(lua_State* L);

        static int lua_ToggleEditorMode(lua_State* L);
        static int lua_IsEditorMode(lua_State* L);
        static int Lua_ShouldDisableGameplay(lua_State* L);

        // Save/Load API
        static int Lua_SaveSceneToJSON(lua_State* L);
        static int Lua_LoadSceneFromJSON(lua_State* L);
        static int Lua_AutoSaveScene(lua_State* L);
        static int Lua_LoadAutoSave(lua_State* L);
        static int Lua_HasAutoSave(lua_State* L);
        static int Lua_ClearAutoSave(lua_State* L);

        // Procedural Map API
        static int Lua_LoadProceduralMap(lua_State* L);

        // Entity Spawning API
        static int Lua_SpawnPlayerAt(lua_State* L);
        static int Lua_SpawnEnemyAt(lua_State* L);
        static int Lua_SpawnChestAt(lua_State* L);
        static int Lua_SpawnGoalAt(lua_State* L);


        // Helper to get LevelLoader instance from Lua state
        static LevelLoader* GetLevelLoader(lua_State* L);
    };

} // namespace Framework