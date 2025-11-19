/**
===============================================================================
 File:           LevelLoader_JSON.h
 Author:         GE YONGQI
 Date:           2025-11-13
 ------------------------------------------------------------------------------
  JSON Configuration Loading for Lua Scripts

  This extension adds JSON loading capabilities to the LevelLoader system,
  allowing Lua scripts to load UI configurations from JSON files.
===============================================================================
*/

#pragma once
#include "Precompiled.h"
#include <nlohmann/json.hpp>

extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

namespace Framework {

    /**
     * @brief JSON Helper functions for LevelLoader
     *
     * Provides utilities to load JSON files and convert them to Lua tables
     */
    class LevelLoaderJSON {
    public:
        /**
         * @brief Load JSON file and push as Lua table
         * @param L Lua state
         * @param filepath Path to JSON file
         * @return true if successful, false otherwise
         */
        static bool LoadJSONToLua(lua_State* L, const std::string& filepath);

        /**
         * @brief Convert JSON object to Lua table (recursive)
         * @param L Lua state
         * @param json JSON object
         */
        static void JSONToLuaTable(lua_State* L, const nlohmann::json& json);

    private:
        /**
         * @brief Push JSON value to Lua stack
         * @param L Lua state
         * @param value JSON value
         */
        static void PushJSONValue(lua_State* L, const nlohmann::json& value);
    };

} // namespace Framework