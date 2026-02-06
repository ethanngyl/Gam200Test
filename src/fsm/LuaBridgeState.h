/**
===============================================================================
 File:          LuaBridgeState.h
 Author:        Padilla Carl Jameson Z.
 Email:         c.padilla@digipen.edu
 Date:          2026-02-06
 Contribution:  100%
 ------------------------------------------------------------------------------

 FINITE STATE MACHINE - Lua Bridge State

 Brief:
    A concrete IState that bridges C++ FSM transitions to Lua callbacks.
    When the C++ StateMachine calls Enter/Update/Exit, this class calls
    the corresponding Lua functions by name in the LevelLoader's Lua state.

    This is what connects the C++ FSM (engine-side) to Lua scripts
    (gameplay-side), allowing designers to write state logic in Lua
    while the FSM infrastructure runs in C++.

 Usage:
    // C++ side (or via Lua bridge API):
    auto state = std::make_unique<LuaBridgeState>("Waiting",
        "PlayerWaiting_Enter", "PlayerWaiting_Update", "PlayerWaiting_Exit");
    fsm.AddState("Waiting", std::move(state));

    // Lua side (these global functions get called by the C++ FSM):
    function PlayerWaiting_Enter(entityID) ... end
    function PlayerWaiting_Update(entityID, dt) ... end
    function PlayerWaiting_Exit(entityID) ... end

 Copyright (C) 2026 DigiPen Institute of Technology.
 Reproduction or disclosure of this file or its contents
 without the prior written consent of DigiPen Institute of
 Technology is prohibited.
===============================================================================
*/

#pragma once

#include "IState.h"
#include "LevelLoader.h"
#include <string>
#include <iostream>

extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

namespace Framework {

    /**
     * @class LuaBridgeState
     * @brief An IState implementation that delegates Enter/Update/Exit to Lua functions
     *
     * Each instance stores the names of three Lua functions (enter, update, exit).
     * When the C++ StateMachine triggers a transition, this class calls the
     * corresponding Lua function in the LevelLoader's Lua state, passing
     * the owner entity ID so the Lua side knows which entity is acting.
     */
    class LuaBridgeState : public IState {
    public:
        /**
         * @brief Constructs a LuaBridgeState
         * @param name       State name (for GetName() / debugging)
         * @param enterFunc  Name of the Lua global function to call on Enter
         * @param updateFunc Name of the Lua global function to call on Update
         * @param exitFunc   Name of the Lua global function to call on Exit
         */
        LuaBridgeState(const std::string& name,
            const std::string& enterFunc,
            const std::string& updateFunc,
            const std::string& exitFunc)
            : m_name(name)
            , m_enterFunc(enterFunc)
            , m_updateFunc(updateFunc)
            , m_exitFunc(exitFunc)
        {
        }

        // =================================================================
        // IState INTERFACE
        // =================================================================

        void Enter() override {
            CallLuaFunc(m_enterFunc, false, 0.0f);
        }

        void Update(float dt) override {
            CallLuaFunc(m_updateFunc, true, dt);
        }

        void Exit() override {
            CallLuaFunc(m_exitFunc, false, 0.0f);
        }

        std::string GetName() const override {
            return m_name;
        }

    private:
        std::string m_name;
        std::string m_enterFunc;
        std::string m_updateFunc;
        std::string m_exitFunc;

        /**
         * @brief Calls a named Lua function in the LevelLoader's Lua state
         * @param funcName  Global Lua function name
         * @param passDt    If true, passes dt as second argument
         * @param dt        Delta time (only used if passDt is true)
         *
         * Signature called in Lua:
         *   funcName(entityID)          -- for Enter/Exit
         *   funcName(entityID, dt)      -- for Update
         */
        void CallLuaFunc(const std::string& funcName, bool passDt, float dt) {
            if (funcName.empty()) return;

            // Get the LevelLoader's Lua state
            LevelLoader& loader = LevelLoader::GetInstance();

            // Access Lua state - we use the same pattern as other bridge functions
            // The Lua state is stored in the LevelLoader
            lua_State* L = nullptr;

            // Get L through the global registration (same approach as LevelLoader API)
            // We access it via the loader's public interface
            // NOTE: You may need to add a GetLuaState() accessor to LevelLoader
            L = loader.GetLuaState();

            if (!L) {
                std::cout << "[LuaBridgeState] ERROR: Lua state is null" << std::endl;
                return;
            }

            // Get the global function
            lua_getglobal(L, funcName.c_str());
            if (!lua_isfunction(L, -1)) {
                lua_pop(L, 1);
                // Not an error - function might be optional
                return;
            }

            // Push entity ID as first argument
            uint32_t entityID = GetOwner().GetID();
            lua_pushinteger(L, static_cast<lua_Integer>(entityID));

            int numArgs = 1;

            // Push dt as second argument if this is an Update call
            if (passDt) {
                lua_pushnumber(L, dt);
                numArgs = 2;
            }

            // Call the Lua function
            if (lua_pcall(L, numArgs, 0, 0) != LUA_OK) {
                const char* err = lua_tostring(L, -1);
                std::cout << "[LuaBridgeState] ERROR calling " << funcName
                    << ": " << (err ? err : "unknown error") << std::endl;
                lua_pop(L, 1);
            }
        }
    };

} // namespace Framework