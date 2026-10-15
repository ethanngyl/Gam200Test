/*
===============================================================================
 File:          ScriptSystem.h
 Author:        ETHAN NG
 Email:         n.ethanyongle@digipen.edu
 Date:          2025-10-31
 Contribution:  100%
 ------------------------------------------------------------------------------
 Script System Header

 Overview:
    The ScriptSystem class serves as the integration layer between the C++ engine
    and the Lua scripting language. It allows entities to have unique behavior
    defined in external .lua files without recompiling the engine.

 Key Features:
    - Per-Entity Lua States (Sandbox environment for each script)
    - Lifecycle Hooks: OnInit, OnUpdate, OnDestroy
    - Hot-Reloading support for rapid iteration
    - Static C-API bindings for Component and Entity manipulation
    - Bridge for accessing Transform, Sprite, and Rendering data


 Copyright (C) 2025 DigiPen Institute of Technology.
 Reproduction or disclosure of this file or its contents
 without the prior written consent of DigiPen Institute of
 Technology is prohibited.
===============================================================================
*/

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
     * @class ScriptSystem
     * @brief Manages the lifecycle and execution of Lua scripts attached to entities
     *
     * The ScriptSystem iterates over all entities with a ScriptComponent.
     * It manages the creation of Lua states, registration of C++ functions
     * into those states, and the execution of script lifecycle events.
     *
     * Usage Pattern:
     * @code
     * ScriptSystem scripts;
     * scripts.SetEntityManager(entityManager);
     * scripts.Initialize();
     * * // Load a script onto an entity
    * scripts.LoadScript(myEntity, "<script_root>/enemy_ai.lua");
     *
     * // Main Loop
     * scripts.Update(deltaTime); // Calls 'OnUpdate(dt)' in Lua
     * @endcode
     */
    class ScriptSystem : public EngineSystem {
    public:
        /**
         * @brief Default constructor
         */
        ScriptSystem();

        /**
         * @brief Destructor - Ensures all Lua states are closed
         */
        virtual ~ScriptSystem();

        // ====================================================================
        // ISystem Interface
        // ====================================================================

        /**
         * @brief Initializes the system
         */
        void Initialize() override;

        /**
         * @brief Main update loop for scripts
         * @param dt Delta time
         *
         * Implementation Details:
         * - Iterates over all entities with ScriptComponent
         * - Calls 'OnInit' if the script is running for the first time
         * - Calls 'OnUpdate(dt)' for all active scripts
         */
        void Update(float dt) override;

        /**
         * @brief Shuts down system and unloads all scripts
         */
        void Shutdown();

        /**
         * @brief Handles engine messages (Currently unused)
         */
        void SendEngineMessage(Message* message) override;

        // ====================================================================
        // Script Management
        // ====================================================================

        /**
         * @brief Attaches a specific Lua script file to an entity
         * @param entity Target entity
         * @param scriptPath Path to the .lua file
         *
         * Implementation Details:
         * - Creates a new lua_State for the entity
         * - Loads the file and checks for syntax errors
         * - Caches the existence of lifecycle functions (HasOnInit, etc.)
         */
        void LoadScript(Entity entity, const std::string& scriptPath);

        /**
         * @brief Hot-reloads a script for an entity
         * @param entity Target entity
         *
         * Useful for live-coding. Unloads the current state and re-reads
         * the file from disk without destroying the entity.
         */
        void ReloadScript(Entity entity);

        /**
         * @brief Removes a script from an entity
         * @param entity Target entity
         *
         * Triggers 'OnDestroy' in Lua before closing the state.
         */
        void UnloadScript(Entity entity);

        /**
         * @brief Hot-reloads all active scripts in the scene
         */
        void ReloadAllScripts();

        // ====================================================================
        // Dependencies
        // ====================================================================
        void SetEntityManager(EntityManager* em) { entityManager = em; }
        void SetCoreEngine(CoreEngine* core) { coreEngine = core; }

    private:
        // ====================================================================
        // Internal Helpers
        // ====================================================================

        bool isShutdown = false;


        // Executes "OnInit" Lua function
        void InitializeScript(Entity entity, ScriptComponent& script);

        // Executes "OnUpdate" Lua function
        void UpdateScript(Entity entity, ScriptComponent& script, float dt);

        // Executes "OnDestroy" Lua function
        void DestroyScript(Entity entity, ScriptComponent& script);

        // Creates a new lua_State and loads libraries
        lua_State* CreateLuaState();

        // Binds C++ static functions to Lua global names
        void RegisterEngineFunctions(lua_State* L);

        // Checks if a global function exists in the Lua state
        bool HasFunction(lua_State* L, const char* funcName);

        EntityManager* entityManager = nullptr;
        CoreEngine* coreEngine = nullptr;

        // ====================================================================
        // LUA C API FUNCTIONS (Bindings)
        // ====================================================================
        // Note: These must be static to comply with Lua's C function signature:
        // int function_name(lua_State* L)

        // --- Component Management ---
        static int Lua_AddTransform(lua_State* L);   // AddTransform(entityID?, x, y)
        static int Lua_AddSprite(lua_State* L);      // AddSprite(entityID?, path)
        static int Lua_AddRenderable(lua_State* L);  // AddRenderable(entityID?)

        static int Lua_RemoveTransform(lua_State* L);
        static int Lua_RemoveSprite(lua_State* L);
        static int Lua_RemoveRenderable(lua_State* L);

        static int Lua_HasComponent(lua_State* L);   // HasComponent(entityID, "Type")

        // --- Component Access (Get/Set) ---
        static int Lua_GetPosition(lua_State* L);    // x, y = GetPosition(entityID?)
        static int Lua_SetPosition(lua_State* L);    // SetPosition(entityID?, x, y)
        static int Lua_GetScale(lua_State* L);       // x, y = GetScale(entityID?)
        static int Lua_SetScale(lua_State* L);       // SetScale(entityID?, x, y)
        static int Lua_GetRotation(lua_State* L);    // r = GetRotation(entityID?)
        static int Lua_SetRotation(lua_State* L);    // SetRotation(entityID?, r)

        // --- Entity Management ---
        static int Lua_CreateEntity(lua_State* L);   // id = CreateEntity()
        static int Lua_DestroyEntity(lua_State* L);  // DestroyEntity(id)
        static int Lua_IsEntityValid(lua_State* L);  // bool = IsEntityValid(id)

        // --- Utility ---
        static int Lua_Log(lua_State* L);            // Log(string)
        static int Lua_GetDeltaTime(lua_State* L);   // dt = GetDeltaTime()

        /**
         * @brief Helper to retrieve the ScriptSystem instance inside a static Lua callback
         * @param L Current Lua state
         * @return Pointer to the ScriptSystem instance
         *
         * Implementation Details:
         * - Retrieves the 'this' pointer stored in the Lua global registry
         * - Key: "__script_system_ptr"
         */
        static ScriptSystem* GetScriptSystem(lua_State* L);
    };

} // namespace Framework