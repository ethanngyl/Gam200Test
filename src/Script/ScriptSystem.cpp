/*
===============================================================================
 File:          ScriptSystem.cpp
 Author:        ETHAN NG
 Email:         n.ethanyongle@digipen.edu
 Date:          2025-10-31
 Contribution:  100%
 ------------------------------------------------------------------------------
  Script System Implementation

 Overview:
    The ScriptSystem class serves as the bridge between the C++ engine core and
    the Lua scripting layer. It manages the lifecycle of script components,
    handles the execution of Lua logic (OnInit, OnUpdate, OnDestroy), and
    exposes a C-API for scripts to manipulate ECS entities and components.

  Design notes:
     - Each ScriptComponent maintains its own independent lua_State
     - Implements a C++ <-> Lua bridge for Entity Component System management
     - Exposes engine functionality via static callback functions
     - Supports hot-reloading of scripts at runtime

 Copyright (C) 2025 DigiPen Institute of Technology.
 Reproduction or disclosure of this file or its contents
 without the prior written consent of DigiPen Institute of
 Technology is prohibited.
===============================================================================
*/

#include "Precompiled.h"
#include "ScriptSystem.h"
#include "LevelLoader.h"
#include "ECSEntityManager.h"
#include "ECSEntity.h"
#include "Component.h"
#include "Core.h"
#include "RenderComponents.h"
#include "Graphics/RenderLayers.h"
#include <fstream>

namespace Framework {

    // =========================================================================
    // CONSTRUCTOR / DESTRUCTOR
    // =========================================================================

    /**
     * @brief Default constructor for ScriptSystem
     * - Logs creation message
     */
    ScriptSystem::ScriptSystem() {
        LOG_INFO("ScriptSystem", "Created");
    }

    /**
     * @brief Destructor for ScriptSystem
     * - Calls Shutdown() to ensure all Lua states are cleaned up
     */
    ScriptSystem::~ScriptSystem() {
        Shutdown();
    }

    // =========================================================================
    // ISYSTEM INTERFACE
    // =========================================================================

    /**
     * @brief Initializes the script system
     */
    void ScriptSystem::Initialize() {
        LOG_INFO("ScriptSystem", "Initialized");
    }

    /**
     * @brief Updates the script system and all active script components
     * @param dt - Delta time since last frame
     *
     * Implementation details:
     * - Validates the entity manager exists
     * - Iterates through all entities to find those with ScriptComponents
     * - Detects first-time execution to run the 'OnInit' Lua function
     * - Calls 'OnUpdate' for initialized scripts, passing delta time
     */
    void ScriptSystem::Update(float dt) {
        if (!entityManager) {
            LOG_WARN("ScriptSystem", "Update() called but entityManager is null!");
            return;
        }

        // Update all entities with ScriptComponent
        auto entities = entityManager->GetAllEntities();

        static bool hasLoggedUpdate = false;
        if (!hasLoggedUpdate) {
            LOG_INFO("ScriptSystem", "========================================");
            LOG_INFO("ScriptSystem", "Update() CALLED - Checking %zu entities", entities.size());
            hasLoggedUpdate = true;
        }

        int scriptCount = 0;
        for (auto entity : entities) {
            if (entityManager->HasComponent<ScriptComponent>(entity)) {
                scriptCount++;
                auto& script = entityManager->GetComponent<ScriptComponent>(entity);

                LOG_INFO("ScriptSystem", "Found ScriptComponent on Entity %u:", entity.GetID());
                LOG_INFO("ScriptSystem", "  scriptPath: %s", script.scriptPath.empty() ? "(empty)" : script.scriptPath.c_str());
                LOG_INFO("ScriptSystem", "  L (Lua state): %s", script.L ? "EXISTS" : "NULL");
                LOG_INFO("ScriptSystem", "  initialized: %s", script.initialized ? "true" : "false");
                LOG_INFO("ScriptSystem", "  hasOnUpdate: %s", script.hasOnUpdate ? "true" : "false");

                // Auto-load script if it has a path but no Lua state
                if (!script.L && !script.scriptPath.empty()) {
                    LOG_INFO("ScriptSystem", "  AUTO-LOADING script for Entity %u...", entity.GetID());
                    LoadScript(entity, script.scriptPath);
                }

                // Initialize script on first update
                if (!script.initialized && script.L) {
                    LOG_INFO("ScriptSystem", "  INITIALIZING script for Entity %u...", entity.GetID());
                    InitializeScript(entity, script);
                }

                // Call OnUpdate if it exists
                if (script.hasOnUpdate && script.L) {
                    UpdateScript(entity, script, dt);
                }
            }
        }

        if (!hasLoggedUpdate) {
            LOG_INFO("ScriptSystem", "Found %d entities with ScriptComponent", scriptCount);
            LOG_INFO("ScriptSystem", "========================================");
        }
    }

    /**
     * @brief Shuts down the script system
     *
     * Implementation details:
     * - Iterates through all entities with ScriptComponents
     * - Calls UnloadScript for each to ensure 'OnDestroy' is called and memory freed
     */
    void ScriptSystem::Shutdown() {
        if (isShutdown) {
            LOG_DEBUG("ScriptSystem", "Already shutdown, skipping");
            return;
        }

        if (!entityManager) {
            LOG_DEBUG("ScriptSystem", "EntityManager null, marking as shutdown");
            isShutdown = true;
            return;
        }

        LOG_INFO("ScriptSystem", "Shutting down...");

        try {
            // Clean up all script states
            auto entities = entityManager->GetAllEntities();
            for (auto entity : entities) {
                if (entityManager->HasComponent<ScriptComponent>(entity)) {
                    auto& script = entityManager->GetComponent<ScriptComponent>(entity);

                    // Call OnDestroy if exists
                    if (script.hasOnDestroy && script.L) {
                        lua_getglobal(script.L, "OnDestroy");
                        if (lua_isfunction(script.L, -1)) {
                            if (lua_pcall(script.L, 0, 0, 0) != LUA_OK) {
                                const char* error = lua_tostring(script.L, -1);
                                LOG_WARN("ScriptSystem", "OnDestroy error: %s", error);
                                lua_pop(script.L, 1);
                            }
                        }
                    }

                    // Close Lua state safely
                    if (script.L) {
                        lua_close(script.L);
                        script.L = nullptr;
                    }

                    script.initialized = false;
                }
            }
        }
        catch (const std::exception& e) {
            LOG_ERROR("ScriptSystem", "Exception during shutdown: %s", e.what());
        }

        isShutdown = true; 
        LOG_INFO("ScriptSystem", "Shutdown complete");
    }

    void ScriptSystem::SendEngineMessage(Message* message) {
        (void)message;
        // Handle messages if needed
    }

    // =========================================================================
    // SCRIPT MANAGEMENT
    // =========================================================================

    /**
     * @brief Loads and attaches a Lua script to an entity
     * @param entity - The target entity
     * @param scriptPath - File path to the .lua script
     *
     * Implementation details:
     * - Adds a ScriptComponent to the entity if one doesn't exist
     * - If one exists, closes the old Lua state before creating a new one
     * - Creates a new lua_State via CreateLuaState()
     * - Sets the global 'self' variable in Lua to the Entity ID
     * - Loads the file using luaL_dofile and checks for syntax errors
     * - Caches the existence of lifecycle functions (OnInit, OnUpdate, OnDestroy) to avoid expensive lookups later
     */
    void ScriptSystem::LoadScript(Entity entity, const std::string& scriptPath) {
        if (!entityManager) {
            LOG_ERROR("ScriptSystem", "EntityManager is null!");
            return;
        }

        // Add or get ScriptComponent
        ScriptComponent* script = nullptr;
        if (entityManager->HasComponent<ScriptComponent>(entity)) {
            script = &entityManager->GetComponent<ScriptComponent>(entity);
            // Clean up old state if exists
            if (script->L) {
                lua_close(script->L);
            }
        }
        else {
            script = &entityManager->AddComponent<ScriptComponent>(entity);
        }

        script->scriptPath = scriptPath;
        script->L = CreateLuaState();
        script->initialized = false;
        lua_pushinteger(script->L, entity.GetID());
        lua_setglobal(script->L, "self");
        // Load the script file
        if (luaL_dofile(script->L, scriptPath.c_str()) != LUA_OK) {
            const char* error = lua_tostring(script->L, -1);
            LOG_ERROR("ScriptSystem", "Failed to load script '%s': %s",
                scriptPath.c_str(), error);
            lua_close(script->L);
            script->L = nullptr;
            return;
        }

        // Check which functions exist
        script->hasOnInit = HasFunction(script->L, "OnInit");
        script->hasOnUpdate = HasFunction(script->L, "OnUpdate");
        script->hasOnDestroy = HasFunction(script->L, "OnDestroy");

        // Store entity ID in Lua global for scripts to access
        lua_pushinteger(script->L, entity.GetID());
        lua_setglobal(script->L, "self");

        LOG_INFO("ScriptSystem", "Loaded script '%s' for entity %u",
            scriptPath.c_str(), entity.GetID());
    }

    /**
     * @brief Reloads the script currently attached to an entity
     * @param entity - The target entity
     *
     * Implementation details:
     * - Retrieves the current script path
     * - Unloads the current script (triggering OnDestroy)
     * - Loads the script again from the file (triggering OnInit on next update)
     */
    void ScriptSystem::ReloadScript(Entity entity) {
        if (!entityManager || !entityManager->HasComponent<ScriptComponent>(entity)) {
            return;
        }

        auto& script = entityManager->GetComponent<ScriptComponent>(entity);
        std::string path = script.scriptPath;

        UnloadScript(entity);
        LoadScript(entity, path);

        LOG_INFO("ScriptSystem", "Reloaded script for entity %u", entity.GetID());
    }

    /**
     * @brief Unloads a script from an entity and cleans up resources
     * @param entity - The target entity
     *
     * Implementation details:
     * - Triggers the Lua 'OnDestroy' function if it exists
     * - Closes the lua_State to free memory
     * - Removes the ScriptComponent from the entity manager
     */
    void ScriptSystem::UnloadScript(Entity entity) {
        if (!entityManager || !entityManager->HasComponent<ScriptComponent>(entity)) {
            return;
        }

        auto& script = entityManager->GetComponent<ScriptComponent>(entity);

        // Call OnDestroy if it exists
        if (script.hasOnDestroy && script.L) {
            DestroyScript(entity, script);
        }

        // Close Lua state
        if (script.L) {
            lua_close(script.L);
            script.L = nullptr;
        }

        // Remove component
        entityManager->RemoveComponent<ScriptComponent>(entity);

        LOG_INFO("ScriptSystem", "Unloaded script for entity %u", entity.GetID());
    }

    /**
     * @brief Triggers a reload for every entity with a script component
     */
    void ScriptSystem::ReloadAllScripts() {
        if (!entityManager) return;

        auto entities = entityManager->GetAllEntities();
        for (auto entity : entities) {
            if (entityManager->HasComponent<ScriptComponent>(entity)) {
                ReloadScript(entity);
            }
        }

        LOG_INFO("ScriptSystem", "Reloaded all scripts");
    }

    // =========================================================================
    // SCRIPT EXECUTION
    // =========================================================================

    /**
     * @brief calls the 'OnInit' function in the Lua script
     * @param entity - The entity context
     * @param script - Reference to the component data
     *
     * Implementation details:
     * - Uses lua_pcall to execute 'OnInit' safely
     * - Logs errors to console if execution fails
     * - Marks the script as initialized upon success
     */
    void ScriptSystem::InitializeScript(Entity entity, ScriptComponent& script) {
        if (!script.hasOnInit || !script.L) return;

        lua_getglobal(script.L, "OnInit");
        lua_pushinteger(script.L, entity.GetID());  // Push entity ID as argument
        if (lua_pcall(script.L, 1, 0, 0) != LUA_OK) {  // Changed from 0 to 1 argument
            const char* error = lua_tostring(script.L, -1);
            LOG_ERROR("ScriptSystem", "OnInit error for entity %u: %s",
                entity.GetID(), error);
            lua_pop(script.L, 1);
            return;
        }

        script.initialized = true;
        LOG_INFO("ScriptSystem", "Initialized script for entity %u", entity.GetID());
    }

    /**
     * @brief calls the 'OnUpdate' function in the Lua script
     * @param entity - The entity context
     * @param script - Reference to the component data
     * @param dt - Delta time to pass to Lua
     *
     * Implementation details:
     * - Pushes 'OnUpdate' function to stack
     * - Pushes delta time as a number argument
     * - Calls function with 1 argument and 0 returns
     */
    void ScriptSystem::UpdateScript(Entity entity, ScriptComponent& script, float dt) {
        if (!script.L) return;

        lua_getglobal(script.L, "OnUpdate");
        lua_pushnumber(script.L, dt);  // Pass deltaTime as parameter

        if (lua_pcall(script.L, 1, 0, 0) != LUA_OK) {
            const char* error = lua_tostring(script.L, -1);
            LOG_ERROR("ScriptSystem", "OnUpdate error for entity %u: %s",
                entity.GetID(), error);
            lua_pop(script.L, 1);
        }
    }

    /**
     * @brief calls the 'OnDestroy' function in the Lua script
     * @param entity - The entity context
     * @param script - Reference to the component data
     */
    void ScriptSystem::DestroyScript(Entity entity, ScriptComponent& script) {
        if (!script.L) return;

        lua_getglobal(script.L, "OnDestroy");
        if (lua_pcall(script.L, 0, 0, 0) != LUA_OK) {
            const char* error = lua_tostring(script.L, -1);
            LOG_ERROR("ScriptSystem", "OnDestroy error for entity %u: %s",
                entity.GetID(), error);
            lua_pop(script.L, 1);
        }
    }

    // =========================================================================
    // LUA STATE MANAGEMENT
    // =========================================================================

    /**
     * @brief Creates and configures a new Lua state
     * @return pointer to the new lua_State
     *
     * Implementation details:
     * - Initializes standard Lua libraries
     * - Registers engine-specific C API functions
     * - Stores a pointer to the ScriptSystem instance in a global Lua variable
     * ('__script_system_ptr') so static callback functions can access member data
     */
    lua_State* ScriptSystem::CreateLuaState() {
        lua_State* L = luaL_newstate();
        luaL_openlibs(L);  // Load standard Lua libraries

        RegisterEngineFunctions(L);

        // Store pointer to this ScriptSystem instance
        lua_pushlightuserdata(L, this);
        lua_setglobal(L, "__script_system_ptr");

        // Store pointer to LevelLoader for entity scripts to access game API
        if (coreEngine) {
            std::cout << "[ScriptSystem] Registering LevelLoader pointer in entity Lua state..." << std::endl;
            LevelLoader* levelLoader = &(coreEngine->GetLevelLoader());
            lua_pushlightuserdata(L, levelLoader);
            lua_setglobal(L, "__level_loader_ptr");
            std::cout << "[ScriptSystem] LevelLoader pointer registered successfully!" << std::endl;
        } else {
            std::cout << "[ScriptSystem] WARNING: coreEngine is NULL, cannot register LevelLoader!" << std::endl;
        }

        return L;
    }

    /**
     * @brief Registers C++ functions to the Lua environment
     * @param L - The Lua state
     */
    void ScriptSystem::RegisterEngineFunctions(lua_State* L) {
        // Component Management
        lua_register(L, "AddTransform", Lua_AddTransform);
        lua_register(L, "AddSprite", Lua_AddSprite);
        lua_register(L, "AddRenderable", Lua_AddRenderable);

        lua_register(L, "RemoveTransform", Lua_RemoveTransform);
        lua_register(L, "RemoveSprite", Lua_RemoveSprite);
        lua_register(L, "RemoveRenderable", Lua_RemoveRenderable);

        lua_register(L, "HasComponent", Lua_HasComponent);

        // Component Access
        lua_register(L, "GetPosition", Lua_GetPosition);
        lua_register(L, "SetPosition", Lua_SetPosition);
        lua_register(L, "GetScale", Lua_GetScale);
        lua_register(L, "SetScale", Lua_SetScale);
        lua_register(L, "GetRotation", Lua_GetRotation);
        lua_register(L, "SetRotation", Lua_SetRotation);

        // Entity Management
        lua_register(L, "CreateEntity", Lua_CreateEntity);
        lua_register(L, "DestroyEntity", Lua_DestroyEntity);
        lua_register(L, "IsEntityValid", Lua_IsEntityValid);

        // Utility
        lua_register(L, "Log", Lua_Log);
        lua_register(L, "GetDeltaTime", Lua_GetDeltaTime);

        // Game API - Input
        lua_register(L, "IsKeyDown", LevelLoader::Lua_IsKeyDown);
        lua_register(L, "IsMouseButtonDown", LevelLoader::Lua_IsMouseButtonDown);
        lua_register(L, "IsMouseButtonPressed", LevelLoader::Lua_IsMouseButtonPressed);
        lua_register(L, "GetMousePosition", LevelLoader::Lua_GetMousePosition);

        // Game API - Turn System
        lua_register(L, "GetCurrentTurn", LevelLoader::Lua_GetCurrentTurn);
        lua_register(L, "GetTurnIndex", LevelLoader::Lua_GetTurnIndex);
        lua_register(L, "EndPlayerTurn", LevelLoader::Lua_EndPlayerTurn);
        lua_register(L, "EndEnemyTurn", LevelLoader::Lua_EndEnemyTurn);
        lua_register(L, "CallLevelFunction", LevelLoader::Lua_CallLevelFunction);  // Generic entity→level bridge
        lua_register(L, "EndCharacterTurn", LevelLoader::Lua_EndCharacterTurn);  // Party system
        lua_register(L, "IsUIAnimating", LevelLoader::Lua_IsUIAnimating);  // Check UI animation state
        lua_register(L, "IsInTurnTransition", LevelLoader::Lua_IsInTurnTransition);  // Check turn transition cooldown
        lua_register(L, "IsTurnScrollPlaying", LevelLoader::Lua_IsTurnScrollPlaying);  // Check turn scroll animation
        lua_register(L, "TriggerAttackAPAnimation", LevelLoader::Lua_TriggerAttackAPAnimation);  // Trigger attack AP consume animation

        // Game API - Player
        lua_register(L, "GetPlayerAP", LevelLoader::Lua_GetPlayerAP);
        lua_register(L, "GetPlayerAttackAP", LevelLoader::Lua_GetPlayerAttackAP);
        lua_register(L, "GetPlayerGridPosition", LevelLoader::Lua_GetPlayerGridPosition);
        lua_register(L, "ConsumePlayerAP", LevelLoader::Lua_ConsumePlayerAP);
        lua_register(L, "RefillPlayerAP", LevelLoader::Lua_RefillPlayerAP);
        lua_register(L, "SetPlayerFlipX", LevelLoader::Lua_SetPlayerFlipX);
        lua_register(L, "LoadPlayerAnimation", LevelLoader::Lua_LoadPlayerAnimation);

        // Game API - Grid/Tiles
        lua_register(L, "IsValidGridPosition", LevelLoader::Lua_IsValidGridPosition);
        lua_register(L, "IsWalkableTile", LevelLoader::Lua_IsWalkableTile);
        lua_register(L, "MovePlayerToTile", LevelLoader::Lua_MovePlayerToTile);
        lua_register(L, "SetGridMovementEnabled", LevelLoader::Lua_SetGridMovementEnabled);
        lua_register(L, "ShowTileBorder", LevelLoader::Lua_ShowTileBorder);
        lua_register(L, "PulseTile", LevelLoader::Lua_PulseTile);
        lua_register(L, "TintTile", LevelLoader::Lua_TintTile);  // Tile tinting for attack preview
        lua_register(L, "HasChestAtTile", LevelLoader::Lua_HasChestAtTile);
        lua_register(L, "CollectChest", LevelLoader::Lua_CollectChest);
        lua_register(L, "HasGoalAtTile", LevelLoader::Lua_HasGoalAtTile);
        lua_register(L, "IsTileOccupied", LevelLoader::Lua_IsTileOccupied);  // Check if tile has an entity

        // Game API - Enemy/Entity
        lua_register(L, "FindPlayer", LevelLoader::Lua_FindPlayer);
        lua_register(L, "GetEnemyAP", LevelLoader::Lua_GetEnemyAP);
        lua_register(L, "RefillEnemyAP", LevelLoader::Lua_RefillEnemyAP);
        lua_register(L, "GetEntityGridPosition", LevelLoader::Lua_GetEntityGridPosition);
        lua_register(L, "GetEntityWorldPosition", LevelLoader::Lua_GetEntityWorldPosition);
        lua_register(L, "MoveEntityToTile", LevelLoader::Lua_MoveEntityToTile);
        lua_register(L, "ConsumeEnemyAP", LevelLoader::Lua_ConsumeEnemyAP);
        lua_register(L, "DamageEntity", LevelLoader::Lua_DamageEntity);
        lua_register(L, "FindPathToTarget", LevelLoader::Lua_FindPathToTarget);
        lua_register(L, "GetAllPlayers", LevelLoader::Lua_GetAllPlayers);
        lua_register(L, "GetAllEnemies", LevelLoader::Lua_GetAllEnemies);

        // Enemy Turn Management System
        lua_register(L, "InitializeEnemyTurn", LevelLoader::Lua_InitializeEnemyTurn);
        lua_register(L, "IsActiveEnemy", LevelLoader::Lua_IsActiveEnemy);
        lua_register(L, "IsEnemyActionReady", LevelLoader::Lua_IsEnemyActionReady);
        lua_register(L, "MarkEnemyActionComplete", LevelLoader::Lua_MarkEnemyActionComplete);
        lua_register(L, "UpdateEnemyTurnManager", LevelLoader::Lua_UpdateEnemyTurnManager);

        // Grid Conversion API
        lua_register(L, "TileToWorld", LevelLoader::Lua_TileToWorld);

        // Game API - Audio
        lua_register(L, "PlaySound", LevelLoader::Lua_PlaySound);

        // Game API - Animation Control
        lua_register(L, "SetAnimationGroup", LevelLoader::Lua_SetAnimationGroup);
        lua_register(L, "SetAnimationDirection", LevelLoader::Lua_SetAnimationDirection);
        lua_register(L, "SetAnimationFlipX", LevelLoader::Lua_SetAnimationFlipX);
        lua_register(L, "SetAnimationPlaying", LevelLoader::Lua_SetAnimationPlaying);
        lua_register(L, "SetAnimationLoop", LevelLoader::Lua_SetAnimationLoop);
        lua_register(L, "SetAnimationFrameRange", LevelLoader::Lua_SetAnimationFrameRange);  // Set startFrame and frameCount for sprite sheet animations
        lua_register(L, "GetAnimationGroup", LevelLoader::Lua_GetAnimationGroup);
        lua_register(L, "GetEntityMovementDirection", LevelLoader::Lua_GetEntityMovementDirection);
        lua_register(L, "SetSpriteAnimationSheet", LevelLoader::Lua_SetSpriteAnimationSheet);
        
        // Game API - Sprite Control
        lua_register(L, "SpawnSprite", LevelLoader::Lua_SpawnSprite);
        lua_register(L, "SpawnAnimatedSprite", LevelLoader::Lua_SpawnAnimatedSprite);
        lua_register(L, "SetSpriteVisibility", LevelLoader::Lua_SetSpriteVisibility);
        lua_register(L, "SetSpritePosition", LevelLoader::Lua_SetSpritePosition);
        lua_register(L, "SetSpriteColor", LevelLoader::Lua_SetSpriteColor);
        lua_register(L, "SetSpriteTexture", LevelLoader::Lua_SetSpriteTexture);

        // Game API - Party System (Entity-Based APIs)
        lua_register(L, "GetEntityAP", LevelLoader::Lua_GetEntityAP);
        lua_register(L, "GetEntityAttackAP", LevelLoader::Lua_GetEntityAttackAP);
        lua_register(L, "ConsumeEntityAP", LevelLoader::Lua_ConsumeEntityAP);
        lua_register(L, "ConsumeEntityAttackAP", LevelLoader::Lua_ConsumeEntityAttackAP);
        lua_register(L, "RefillEntityAP", LevelLoader::Lua_RefillEntityAP);
        lua_register(L, "RefillEntityAttackAP", LevelLoader::Lua_RefillEntityAttackAP);
        lua_register(L, "GetEntityHP", LevelLoader::Lua_GetEntityHP);
        lua_register(L, "SetEntityHP", LevelLoader::Lua_SetEntityHP);
        lua_register(L, "IsActiveCharacter", LevelLoader::Lua_IsActiveCharacter);

        // Game API - Projectile Skills
        lua_register(L, "SpawnSkillProjectile", LevelLoader::Lua_SpawnSkillProjectile);

        // JSON Loading (shared with LevelLoader)
        lua_register(L, "LoadJSON", LevelLoader::Lua_LoadJSON);
    }

    /**
     * @brief Helper to check if a specific global function exists in Lua
     */
    bool ScriptSystem::HasFunction(lua_State* L, const char* funcName) {
        lua_getglobal(L, funcName);
        bool exists = lua_isfunction(L, -1);
        lua_pop(L, 1);
        return exists;
    }

    /**
     * @brief Retrieves the ScriptSystem instance from the Lua state
     * @param L - The Lua state
     * @return Pointer to ScriptSystem
     *
     * Implementation details:
     * - Retrieves the lightuserdata stored at '__script_system_ptr'
     * - Casts it back to ScriptSystem*
     */
    ScriptSystem* ScriptSystem::GetScriptSystem(lua_State* L) {
        lua_getglobal(L, "__script_system_ptr");
        ScriptSystem* system = static_cast<ScriptSystem*>(lua_touserdata(L, -1));
        lua_pop(L, 1);
        return system;
    }

    // =========================================================================
    // LUA C API FUNCTIONS - Component Management
    // =========================================================================

    /**
     * @brief Lua API: Adds a Transform component to an entity
     * @params entityID (optional), x (optional), y (optional)
     *
     * Implementation details:
     * - If entityID is not provided, defaults to 'self'
     * - Checks if component already exists
     * - Adds component and initializes position/scale/rotation
     */
    int ScriptSystem::Lua_AddTransform(lua_State* L) {
        ScriptSystem* system = GetScriptSystem(L);
        if (!system || !system->entityManager) return 0;

        // Get entity ID (defaults to 'self' if not provided)
        uint32_t entityID = luaL_optinteger(L, 1, 0);
        if (entityID == 0) {
            lua_getglobal(L, "self");
            entityID = lua_tointeger(L, -1);
            lua_pop(L, 1);
        }

        Entity entity(entityID);

        if (system->entityManager->HasComponent<Transform>(entity)) {
            LOG_WARN("ScriptSystem", "Entity %u already has Transform", entityID);
            lua_pushboolean(L, false);
            return 1;
        }

        // Get optional position parameters (x, y)
        float x = luaL_optnumber(L, 2, 0.0f);
        float y = luaL_optnumber(L, 3, 0.0f);

        auto& transform = system->entityManager->AddComponent<Transform>(entity);
        transform.position = Vector2D(x, y);
        transform.scale = Vector2D(1.0f, 1.0f);
        transform.rotation = 0.0f;

        LOG_INFO("ScriptSystem", "Added Transform to entity %u", entityID);
        lua_pushboolean(L, true);
        return 1;
    }

    int ScriptSystem::Lua_AddSprite(lua_State* L) {
        ScriptSystem* system = GetScriptSystem(L);
        if (!system || !system->entityManager) return 0;

        uint32_t entityID = luaL_optinteger(L, 1, 0);
        if (entityID == 0) {
            lua_getglobal(L, "self");
            entityID = lua_tointeger(L, -1);
            lua_pop(L, 1);
        }

        Entity entity(entityID);

        if (system->entityManager->HasComponent<Sprite>(entity)) {
            LOG_WARN("ScriptSystem", "Entity %u already has Sprite", entityID);
            lua_pushboolean(L, false);
            return 1;
        }

        // Get texture path parameter
        const char* texturePath = luaL_optstring(L, 2, "quad");

        auto& sprite = system->entityManager->AddComponent<Sprite>(entity);
        sprite.texturePath = texturePath;

        LOG_INFO("ScriptSystem", "Added Sprite to entity %u with texture '%s'",
            entityID, texturePath);
        lua_pushboolean(L, true);
        return 1;
    }

    int ScriptSystem::Lua_AddRenderable(lua_State* L) {
        ScriptSystem* system = GetScriptSystem(L);
        if (!system || !system->entityManager) return 0;

        uint32_t entityID = luaL_optinteger(L, 1, 0);
        if (entityID == 0) {
            lua_getglobal(L, "self");
            entityID = lua_tointeger(L, -1);
            lua_pop(L, 1);
        }

        Entity entity(entityID);

        if (system->entityManager->HasComponent<Renderable>(entity)) {
            LOG_WARN("ScriptSystem", "Entity %u already has Renderable", entityID);
            lua_pushboolean(L, false);
            return 1;
        }

        auto& renderable = system->entityManager->AddComponent<Renderable>(entity);
        renderable.visible = true;
        renderable.layer = RenderLayers::Ground;  // Default to ground layer

        LOG_INFO("ScriptSystem", "Added Renderable to entity %u", entityID);
        lua_pushboolean(L, true);
        return 1;
    }

    // =========================================================================
    // LUA C API FUNCTIONS - Remove Components
    // =========================================================================

    int ScriptSystem::Lua_RemoveTransform(lua_State* L) {
        ScriptSystem* system = GetScriptSystem(L);
        if (!system || !system->entityManager) return 0;

        uint32_t entityID = luaL_optinteger(L, 1, 0);
        if (entityID == 0) {
            lua_getglobal(L, "self");
            entityID = lua_tointeger(L, -1);
            lua_pop(L, 1);
        }

        Entity entity(entityID);

        if (!system->entityManager->HasComponent<Transform>(entity)) {
            lua_pushboolean(L, false);
            return 1;
        }

        system->entityManager->RemoveComponent<Transform>(entity);
        LOG_INFO("ScriptSystem", "Removed Transform from entity %u", entityID);
        lua_pushboolean(L, true);
        return 1;
    }

    int ScriptSystem::Lua_RemoveSprite(lua_State* L) {
        ScriptSystem* system = GetScriptSystem(L);
        if (!system || !system->entityManager) return 0;

        uint32_t entityID = luaL_optinteger(L, 1, 0);
        if (entityID == 0) {
            lua_getglobal(L, "self");
            entityID = lua_tointeger(L, -1);
            lua_pop(L, 1);
        }

        Entity entity(entityID);

        if (!system->entityManager->HasComponent<Sprite>(entity)) {
            lua_pushboolean(L, false);
            return 1;
        }

        system->entityManager->RemoveComponent<Sprite>(entity);
        LOG_INFO("ScriptSystem", "Removed Sprite from entity %u", entityID);
        lua_pushboolean(L, true);
        return 1;
    }

    int ScriptSystem::Lua_RemoveRenderable(lua_State* L) {
        ScriptSystem* system = GetScriptSystem(L);
        if (!system || !system->entityManager) return 0;

        uint32_t entityID = luaL_optinteger(L, 1, 0);
        if (entityID == 0) {
            lua_getglobal(L, "self");
            entityID = lua_tointeger(L, -1);
            lua_pop(L, 1);
        }

        Entity entity(entityID);

        if (!system->entityManager->HasComponent<Renderable>(entity)) {
            lua_pushboolean(L, false);
            return 1;
        }

        system->entityManager->RemoveComponent<Renderable>(entity);
        LOG_INFO("ScriptSystem", "Removed Renderable from entity %u", entityID);
        lua_pushboolean(L, true);
        return 1;
    }

    // =========================================================================
    // LUA C API FUNCTIONS - Component Queries
    // =========================================================================

    /**
     * @brief Lua API: Checks if an entity has a specific component
     * @params entityID, componentName (string)
     */
    int ScriptSystem::Lua_HasComponent(lua_State* L) {
        ScriptSystem* system = GetScriptSystem(L);
        if (!system || !system->entityManager) return 0;

        uint32_t entityID = luaL_checkinteger(L, 1);
        const char* componentName = luaL_checkstring(L, 2);

        Entity entity(entityID);
        bool hasComponent = false;

        if (strcmp(componentName, "Transform") == 0) {
            hasComponent = system->entityManager->HasComponent<Transform>(entity);
        }
        else if (strcmp(componentName, "Sprite") == 0) {
            hasComponent = system->entityManager->HasComponent<Sprite>(entity);
        }
        else if (strcmp(componentName, "Renderable") == 0) {
            hasComponent = system->entityManager->HasComponent<Renderable>(entity);
        }

        lua_pushboolean(L, hasComponent);
        return 1;
    }

    // =========================================================================
    // LUA C API FUNCTIONS - Component Access
    // =========================================================================

    /**
     * @brief Lua API: Gets transform position
     * @return x, y
     */
    int ScriptSystem::Lua_GetPosition(lua_State* L) {
        ScriptSystem* system = GetScriptSystem(L);
        if (!system || !system->entityManager) return 0;

        uint32_t entityID = luaL_optinteger(L, 1, 0);
        if (entityID == 0) {
            lua_getglobal(L, "self");
            entityID = lua_tointeger(L, -1);
            lua_pop(L, 1);
        }

        Entity entity(entityID);

        if (!system->entityManager->HasComponent<Transform>(entity)) {
            lua_pushnil(L);
            lua_pushnil(L);
            return 2;
        }

        auto& transform = system->entityManager->GetComponent<Transform>(entity);
        lua_pushnumber(L, transform.position.x);
        lua_pushnumber(L, transform.position.y);
        return 2;
    }

    int ScriptSystem::Lua_SetPosition(lua_State* L) {
        ScriptSystem* system = GetScriptSystem(L);
        if (!system || !system->entityManager) return 0;

        uint32_t entityID = luaL_optinteger(L, 1, 0);
        float x = luaL_checknumber(L, 2);
        float y = luaL_checknumber(L, 3);

        if (entityID == 0) {
            lua_getglobal(L, "self");
            entityID = lua_tointeger(L, -1);
            lua_pop(L, 1);
        }

        Entity entity(entityID);

        if (!system->entityManager->HasComponent<Transform>(entity)) {
            lua_pushboolean(L, false);
            return 1;
        }

        auto& transform = system->entityManager->GetComponent<Transform>(entity);
        transform.position.x = x;
        transform.position.y = y;

        lua_pushboolean(L, true);
        return 1;
    }

    int ScriptSystem::Lua_GetScale(lua_State* L) {
        ScriptSystem* system = GetScriptSystem(L);
        if (!system || !system->entityManager) return 0;

        uint32_t entityID = luaL_optinteger(L, 1, 0);
        if (entityID == 0) {
            lua_getglobal(L, "self");
            entityID = lua_tointeger(L, -1);
            lua_pop(L, 1);
        }

        Entity entity(entityID);

        if (!system->entityManager->HasComponent<Transform>(entity)) {
            lua_pushnil(L);
            lua_pushnil(L);
            return 2;
        }

        auto& transform = system->entityManager->GetComponent<Transform>(entity);
        lua_pushnumber(L, transform.scale.x);
        lua_pushnumber(L, transform.scale.y);
        return 2;
    }

    int ScriptSystem::Lua_SetScale(lua_State* L) {
        ScriptSystem* system = GetScriptSystem(L);
        if (!system || !system->entityManager) return 0;

        uint32_t entityID = luaL_optinteger(L, 1, 0);
        float x = luaL_checknumber(L, 2);
        float y = luaL_checknumber(L, 3);

        if (entityID == 0) {
            lua_getglobal(L, "self");
            entityID = lua_tointeger(L, -1);
            lua_pop(L, 1);
        }

        Entity entity(entityID);

        if (!system->entityManager->HasComponent<Transform>(entity)) {
            lua_pushboolean(L, false);
            return 1;
        }

        auto& transform = system->entityManager->GetComponent<Transform>(entity);
        transform.scale.x = x;
        transform.scale.y = y;

        lua_pushboolean(L, true);
        return 1;
    }

    int ScriptSystem::Lua_GetRotation(lua_State* L) {
        ScriptSystem* system = GetScriptSystem(L);
        if (!system || !system->entityManager) return 0;

        uint32_t entityID = luaL_optinteger(L, 1, 0);
        if (entityID == 0) {
            lua_getglobal(L, "self");
            entityID = lua_tointeger(L, -1);
            lua_pop(L, 1);
        }

        Entity entity(entityID);

        if (!system->entityManager->HasComponent<Transform>(entity)) {
            lua_pushnil(L);
            return 1;
        }

        auto& transform = system->entityManager->GetComponent<Transform>(entity);
        lua_pushnumber(L, transform.rotation);
        return 1;
    }

    int ScriptSystem::Lua_SetRotation(lua_State* L) {
        ScriptSystem* system = GetScriptSystem(L);
        if (!system || !system->entityManager) return 0;

        uint32_t entityID = luaL_optinteger(L, 1, 0);
        float rotation = luaL_checknumber(L, 2);

        if (entityID == 0) {
            lua_getglobal(L, "self");
            entityID = lua_tointeger(L, -1);
            lua_pop(L, 1);
        }

        Entity entity(entityID);

        if (!system->entityManager->HasComponent<Transform>(entity)) {
            lua_pushboolean(L, false);
            return 1;
        }

        auto& transform = system->entityManager->GetComponent<Transform>(entity);
        transform.rotation = rotation;

        lua_pushboolean(L, true);
        return 1;
    }

    // =========================================================================
    // LUA C API FUNCTIONS - Entity Management
    // =========================================================================

    /**
     * @brief Lua API: Creates a new empty entity
     * @return The new Entity ID (integer)
     */
    int ScriptSystem::Lua_CreateEntity(lua_State* L) {
        ScriptSystem* system = GetScriptSystem(L);
        if (!system || !system->entityManager) return 0;

        Entity newEntity = system->entityManager->CreateEntity();
        lua_pushinteger(L, newEntity.GetID());

        LOG_INFO("ScriptSystem", "Created entity %u from script", newEntity.GetID());
        return 1;
    }

    int ScriptSystem::Lua_DestroyEntity(lua_State* L) {
        ScriptSystem* system = GetScriptSystem(L);
        if (!system || !system->entityManager) return 0;

        uint32_t entityID = luaL_checkinteger(L, 1);
        Entity entity(entityID);

        system->entityManager->DestroyEntity(entity);
        LOG_INFO("ScriptSystem", "Destroyed entity %u from script", entityID);

        lua_pushboolean(L, true);
        return 1;
    }

    int ScriptSystem::Lua_IsEntityValid(lua_State* L) {
        ScriptSystem* system = GetScriptSystem(L);
        if (!system || !system->entityManager) return 0;

        uint32_t entityID = luaL_checkinteger(L, 1);
        Entity entity(entityID);

        bool valid = entity.IsValid();
        lua_pushboolean(L, valid);
        return 1;
    }

    // =========================================================================
    // LUA C API FUNCTIONS - Utility
    // =========================================================================

    int ScriptSystem::Lua_Log(lua_State* L) {
        const char* message = luaL_checkstring(L, 1);
        LOG_INFO("LuaScript", "%s", message);
        return 0;
    }

    int ScriptSystem::Lua_GetDeltaTime(lua_State* L) {
        ScriptSystem* system = GetScriptSystem(L);
        if (!system || !system->coreEngine) {
            lua_pushnumber(L, 0.016f);  // Default ~60 FPS
            return 1;
        }

        // You'll need to add GetDeltaTime() to CoreEngine if it doesn't exist
        // For now, return a default value
        lua_pushnumber(L, 0.016f);
        return 1;
    }

} // namespace Framework