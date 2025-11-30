/**
===============================================================================
 File:           LevelLoader.h
 Author:         GE YONGQI
 Email:          yongqi.ge@digipen.edu
 Date:           2025-11-13
 Contribution:   100%
 ------------------------------------------------------------------------------
  Lua-based Level Loading System

  Purpose:
  Provides a unified system for loading entire game levels/states from Lua
  scripts instead of hardcoding level logic in C++. This enables:
  - Hot-reloadable level configurations
  - Designer-friendly level creation (no C++ compilation needed)
  - Centralized level management
  - Easy A/B testing of different level layouts

  Design:
  Each level is represented by a single Lua script with lifecycle functions:
  - OnInit()     - Setup level (spawn entities, UI, audio)
  - OnUpdate(dt) - Per-frame logic
  - OnDraw()     - Custom rendering (UI text, etc.)
  - OnDestroy()  - Cleanup

  Usage:
  Instead of:
    mainMenu_Load()
    mainMenu_Initialize()
    mainMenu_Update()
    mainMenu_Draw()
    mainMenu_Free()
    mainMenu_Unload()

  Use:
    LevelLoader::LoadLevel("assets/scripts/MainMenuLevel.lua")
    LevelLoader::UpdateCurrentLevel(dt)
    LevelLoader::DrawCurrentLevel()
    LevelLoader::UnloadCurrentLevel()
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

        // UI Button API
        static int Lua_CreateButton(lua_State* L);
        static int Lua_ClearAllButtons(lua_State* L);
        static int Lua_DrawButtonText(lua_State* L);
        static int Lua_DrawText(lua_State* L);

        // Input API
        static int Lua_IsKeyDown(lua_State* L);

        // JSON API
        static int Lua_LoadJSON(lua_State* L);

        // TileMap Loading API
        static int Lua_LoadTileMap(lua_State* L);

        // AP Indicator / Entity Management API
        static int Lua_SpawnSprite(lua_State* L);
        static int Lua_SetSpriteColor(lua_State* L);
        static int Lua_SetSpriteTexture(lua_State* L);
        static int Lua_SetSpritePosition(lua_State* L);
		static int Lua_SetSpriteVisibility(lua_State* L);
        static int Lua_DestroyEntity(lua_State* L);
        static int Lua_GetPlayerAP(lua_State* L);
        static int Lua_GetCameraPosition(lua_State* L);
        static int Lua_FindPlayer(lua_State* L);
        static int Lua_GetAllEnemies(lua_State* L);
        static int Lua_SetEnemyTarget(lua_State* L);
        static int Lua_GetCurrentTurn(lua_State* L);
		static int Lua_GetChestProgress(lua_State* L);
        static int Lua_LoadAnimationConfig(lua_State* L);
        static int Lua_LoadPlayerAnimation(lua_State* L);
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
        static int Lua_SetPlayerFlipX(lua_State* L);
        static int Lua_HasChestAtTile(lua_State* L);
        static int Lua_CollectChest(lua_State* L);
        static int Lua_HasGoalAtTile(lua_State* L);

        // Enemy/Entity API
        static int Lua_GetEnemyAP(lua_State* L);
        static int Lua_RefillEnemyAP(lua_State* L);
        static int Lua_GetEntityGridPosition(lua_State* L);
        static int Lua_MoveEntityToTile(lua_State* L);
        static int Lua_ConsumeEnemyAP(lua_State* L);
        static int Lua_DamageEntity(lua_State* L);
        static int Lua_FindPathToTarget(lua_State* L);

        // Script Component Management API
        static int Lua_AddScriptComponentToEntity(lua_State* L);
        static int Lua_RemoveScriptComponentFromEntity(lua_State* L);

        static int lua_ToggleEditorMode(lua_State* L);
        static int lua_IsEditorMode(lua_State* L);

        // Helper to get LevelLoader instance from Lua state
        static LevelLoader* GetLevelLoader(lua_State* L);
    };

} // namespace Framework