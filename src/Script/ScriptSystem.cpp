/**
 * ScriptSystem.cpp
 *
 * Implementation of Lua scripting system
 * Provides C++ <-> Lua bridge for component management
 */

#include "Precompiled.h"
#include "ScriptSystem.h"
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

    ScriptSystem::ScriptSystem() {
        LOG_INFO("ScriptSystem", "Created");
    }

    ScriptSystem::~ScriptSystem() {
        Shutdown();
    }

    // =========================================================================
    // ISYSTEM INTERFACE
    // =========================================================================

    void ScriptSystem::Initialize() {
        LOG_INFO("ScriptSystem", "Initialized");
    }

    void ScriptSystem::Update(float dt) {
        if (!entityManager) return;

        // Update all entities with ScriptComponent
        auto entities = entityManager->GetAllEntities();
        for (auto entity : entities) {
            if (entityManager->HasComponent<ScriptComponent>(entity)) {
                auto& script = entityManager->GetComponent<ScriptComponent>(entity);

                // Initialize script on first update
                if (!script.initialized && script.L) {
                    InitializeScript(entity, script);
                }

                // Call OnUpdate if it exists
                if (script.hasOnUpdate && script.L) {
                    UpdateScript(entity, script, dt);
                }
            }
        }
    }

    void ScriptSystem::Shutdown() {
        if (!entityManager) return;

        // Clean up all script states
        auto entities = entityManager->GetAllEntities();
        for (auto entity : entities) {
            if (entityManager->HasComponent<ScriptComponent>(entity)) {
                UnloadScript(entity);
            }
        }

        LOG_INFO("ScriptSystem", "Shutdown complete");
    }

    void ScriptSystem::SendEngineMessage(Message* message) {
        // Handle messages if needed
    }

    // =========================================================================
    // SCRIPT MANAGEMENT
    // =========================================================================

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

    void ScriptSystem::InitializeScript(Entity entity, ScriptComponent& script) {
        if (!script.hasOnInit || !script.L) return;

        lua_getglobal(script.L, "OnInit");
        if (lua_pcall(script.L, 0, 0, 0) != LUA_OK) {
            const char* error = lua_tostring(script.L, -1);
            LOG_ERROR("ScriptSystem", "OnInit error for entity %u: %s",
                entity.GetID(), error);
            lua_pop(script.L, 1);
            return;
        }

        script.initialized = true;
        LOG_INFO("ScriptSystem", "Initialized script for entity %u", entity.GetID());
    }

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

    lua_State* ScriptSystem::CreateLuaState() {
        lua_State* L = luaL_newstate();
        luaL_openlibs(L);  // Load standard Lua libraries

        RegisterEngineFunctions(L);

        // Store pointer to this ScriptSystem instance
        lua_pushlightuserdata(L, this);
        lua_setglobal(L, "__script_system_ptr");

        return L;
    }

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
    }

    bool ScriptSystem::HasFunction(lua_State* L, const char* funcName) {
        lua_getglobal(L, funcName);
        bool exists = lua_isfunction(L, -1);
        lua_pop(L, 1);
        return exists;
    }

    ScriptSystem* ScriptSystem::GetScriptSystem(lua_State* L) {
        lua_getglobal(L, "__script_system_ptr");
        ScriptSystem* system = static_cast<ScriptSystem*>(lua_touserdata(L, -1));
        lua_pop(L, 1);
        return system;
    }

    // =========================================================================
    // LUA C API FUNCTIONS - Component Management
    // =========================================================================

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