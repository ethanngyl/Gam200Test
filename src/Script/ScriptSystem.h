#pragma once

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

#include "ECSEntity.h"
#include "Interface.h"
#include <string>
#include <unordered_map>
#include "Component.h"
namespace Framework {

    // Forward declarations
    class EntityManager;
    class CoreEngine;

    /**
     * @brief System that manages Lua scripts attached to entities
     */
    class ScriptSystem : public EngineSystem {
    public:
        ScriptSystem();
        virtual ~ScriptSystem();

        // ISystem interface
        void Initialize() override;
        void Update(float dt) override;
        void Shutdown();
        void SendEngineMessage(Message* message) override;

        // Script management
        void LoadScript(Entity entity, const std::string& scriptPath);
        void ReloadScript(Entity entity);
        void UnloadScript(Entity entity);
        void ReloadAllScripts();

        // Dependencies
        void SetEntityManager(EntityManager* em) { entityManager = em; }
        void SetCoreEngine(CoreEngine* core) { coreEngine = core; }

    private:
        // Script execution
        void InitializeScript(Entity entity, ScriptComponent& script);
        void UpdateScript(Entity entity, ScriptComponent& script, float dt);
        void DestroyScript(Entity entity, ScriptComponent& script);

        // Lua state management
        lua_State* CreateLuaState();
        void RegisterEngineFunctions(lua_State* L);

        // Helper to check if function exists in script
        bool HasFunction(lua_State* L, const char* funcName);

        EntityManager* entityManager = nullptr;
        CoreEngine* coreEngine = nullptr;

        // ===== LUA C API FUNCTIONS (called from Lua scripts) =====
        // These are static because Lua C API requires C-style function pointers

        // Component Management
        static int Lua_AddTransform(lua_State* L);
        static int Lua_AddSprite(lua_State* L);
        static int Lua_AddRenderable(lua_State* L);

        static int Lua_RemoveTransform(lua_State* L);
        static int Lua_RemoveSprite(lua_State* L);
        static int Lua_RemoveRenderable(lua_State* L);

        static int Lua_HasComponent(lua_State* L);

        // Component Access (Get/Set)
        static int Lua_GetPosition(lua_State* L);
        static int Lua_SetPosition(lua_State* L);
        static int Lua_GetScale(lua_State* L);
        static int Lua_SetScale(lua_State* L);
        static int Lua_GetRotation(lua_State* L);
        static int Lua_SetRotation(lua_State* L);

        // Entity Management
        static int Lua_CreateEntity(lua_State* L);
        static int Lua_DestroyEntity(lua_State* L);
        static int Lua_IsEntityValid(lua_State* L);

        // Utility
        static int Lua_Log(lua_State* L);
        static int Lua_GetDeltaTime(lua_State* L);

        // Helper to get ScriptSystem instance from Lua state
        static ScriptSystem* GetScriptSystem(lua_State* L);
    };

} // namespace Framework