/**
===============================================================================
 File:           LevelLoader.cpp
 Author:         GE YONGQI
 Email:          yongqi.ge@digipen.edu
 Date:           2025-11-13
 ------------------------------------------------------------------------------
  Implementation of Lua-based level loading system
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

    bool LevelLoader::LoadLevel(const std::string& scriptPath) {
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

        // Load the Lua script
        if (luaL_dofile(L, scriptPath.c_str()) != LUA_OK) {
            const char* error = lua_tostring(L, -1);
            LOG_ERROR("LevelLoader", "Failed to load: %s", error);
            lua_pop(L, 1);
            return false;
        }

        // Check required functions
        bool hasInit = HasLuaFunction("OnInit");
        bool hasUpdate = HasLuaFunction("OnUpdate");
        bool hasDraw = HasLuaFunction("OnDraw");
        bool hasDestroy = HasLuaFunction("OnDestroy");

        LOG_INFO("LevelLoader", "  Functions found:");
        LOG_INFO("LevelLoader", "    OnInit: %s", hasInit ? "✓" : "✗");
        LOG_INFO("LevelLoader", "    OnUpdate: %s", hasUpdate ? "✓" : "✗");
        LOG_INFO("LevelLoader", "    OnDraw: %s", hasDraw ? "✓" : "✗");
        LOG_INFO("LevelLoader", "    OnDestroy: %s", hasDestroy ? "✓" : "✗");

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

        // Check global pause state - skip update if paused
        if (GlobalPause::IsPaused()) {
            return;
        }

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

        // UI Buttons
        lua_register(L, "CreateButton", Lua_CreateButton);
        lua_register(L, "ClearAllButtons", Lua_ClearAllButtons);
        lua_register(L, "DrawButtonText", Lua_DrawButtonText);

        // Input
        lua_register(L, "IsKeyDown", Lua_IsKeyDown);

        // JSON and Level Loading
        lua_register(L, "LoadJSON", Lua_LoadJSON);
        lua_register(L, "LoadTileMap", Lua_LoadTileMap);

        // AP Indicator / Entity Management
        lua_register(L, "SpawnSprite", Lua_SpawnSprite);
        lua_register(L, "SetSpriteColor", Lua_SetSpriteColor);
        lua_register(L, "SetSpriteTexture", Lua_SetSpriteTexture);
        lua_register(L, "SetSpritePosition", Lua_SetSpritePosition);
        lua_register(L, "DestroyEntity", Lua_DestroyEntity);
        lua_register(L, "GetPlayerAP", Lua_GetPlayerAP);
        lua_register(L, "GetCameraPosition", Lua_GetCameraPosition);
		lua_register(L, "GetPlayerAttackAP", Lua_GetPlayerAttackAP);
		lua_register(L, "GetPlayerHP", Lua_GetPlayerHP);

        // Enemy AI Configuration
        lua_register(L, "FindPlayer", Lua_FindPlayer);
        lua_register(L, "GetAllEnemies", Lua_GetAllEnemies);
        lua_register(L, "SetEnemyTarget", Lua_SetEnemyTarget);
        lua_register(L, "GetCurrentTurn", Lua_GetCurrentTurn);
        lua_register(L, "GetChestProgress", Lua_GetChestProgress);

        // Animation
        lua_register(L, "LoadAnimationConfig", Lua_LoadAnimationConfig);
        lua_register(L, "LoadPlayerAnimation", Lua_LoadPlayerAnimation);

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

        // Map state names to enum values
        if (strcmp(stateName, "Level_select") == 0) {
            next = Level_select;
        }
        else if (strcmp(stateName, "LEVEL_1") == 0) {
            next = LEVEL_1;
        }
        else if (strcmp(stateName, "LEVEL_2") == 0) {
            next = LEVEL_2;
        }
        else if (strcmp(stateName, "LEVEL_3") == 0) {
            next = LEVEL_3;
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
        LOG_INFO("LUA_ANIM", "✅ Animation config loaded from '%s'", configPath);

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
            LOG_INFO("LOAD_ANIM", "✅ Added SpriteAnimation component");
        } else {
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
            LOG_INFO("LOAD_ANIM", "✅ Animation loaded:");
            LOG_INFO("LOAD_ANIM", "  - Name: '%s'", anim.animName.c_str());
            LOG_INFO("LOAD_ANIM", "  - Grid: %dx%d", anim.rows, anim.columns);
            LOG_INFO("LOAD_ANIM", "  - Frames: %d", anim.frameCount);
            LOG_INFO("LOAD_ANIM", "  - SpriteSheet: %u", anim.spriteSheet.GetID());
            LOG_INFO("LOAD_ANIM", "  - Playing: %d", anim.playing);
        } else {
            LOG_ERROR("LOAD_ANIM", "AnimationSystem or GraphicsSystem not available");
        }

        return 0;
    }

    // [CONTINUED IN NEXT PART...]

} // namespace Framework