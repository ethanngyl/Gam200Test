/*
===============================================================================
 File:          LevelLoader_JSON.h
 Author:        GE YONGQI
 Email:         yongqi.ge@digipen.edu
 Date:          2025-11-13
 Contribution:  100%
 ------------------------------------------------------------------------------
  JSON Configuration Loading for Lua Scripts

  Purpose:
  This extension adds JSON loading capabilities to the LevelLoader system,
  allowing Lua scripts to load UI configurations from JSON files.

  Features:
  - Parses JSON files using nlohmann/json library
  - Converts JSON objects to Lua tables automatically
  - Supports nested objects and arrays
  - Handles all JSON data types (null, boolean, integer, float, string)

  Usage in Lua:
    config = LoadJSON("assets/JSON/mainmenu_config.json")
    buttonText = config.menu.buttons[1].text.content
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