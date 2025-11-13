/**
===============================================================================
 File:           LevelLoader_JSON.cpp
 Author:         GE YONGQI
 Date:           2025-11-13
 ------------------------------------------------------------------------------
  JSON Configuration Loading Implementation
===============================================================================
*/

#include "Precompiled.h"
#include "LevelLoader_JSON.h"
#include <fstream>
#include <sstream>

namespace Framework {

    // ========================================================================
    // JSON TO LUA CONVERSION
    // ========================================================================

    bool LevelLoaderJSON::LoadJSONToLua(lua_State* L, const std::string& filepath) {
        if (!L) {
            LOG_ERROR("LevelLoaderJSON", "Lua state is null!");
            return false;
        }

        // Read JSON file
        std::ifstream file(filepath);
        if (!file.is_open()) {
            LOG_ERROR("LevelLoaderJSON", "Failed to open JSON file: %s", filepath.c_str());
            return false;
        }

        try {
            // Parse JSON
            nlohmann::json jsonData;
            file >> jsonData;
            file.close();

            LOG_INFO("LevelLoaderJSON", "Successfully parsed JSON: %s", filepath.c_str());

            // Convert to Lua table
            JSONToLuaTable(L, jsonData);

            LOG_INFO("LevelLoaderJSON", "JSON loaded and converted to Lua table");
            return true;
        }
        catch (const nlohmann::json::exception& e) {
            LOG_ERROR("LevelLoaderJSON", "JSON parsing error: %s", e.what());
            file.close();
            return false;
        }
        catch (const std::exception& e) {
            LOG_ERROR("LevelLoaderJSON", "Error loading JSON: %s", e.what());
            file.close();
            return false;
        }
    }

    void LevelLoaderJSON::JSONToLuaTable(lua_State* L, const nlohmann::json& json) {
        lua_newtable(L);  // Create new Lua table

        if (json.is_object()) {
            // Iterate through JSON object
            for (auto it = json.begin(); it != json.end(); ++it) {
                // Push key
                lua_pushstring(L, it.key().c_str());

                // Push value (recursive for nested objects/arrays)
                PushJSONValue(L, it.value());

                // Set table[key] = value
                lua_settable(L, -3);
            }
        }
        else if (json.is_array()) {
            // JSON array to Lua array (1-indexed)
            int index = 1;
            for (const auto& element : json) {
                lua_pushinteger(L, index++);
                PushJSONValue(L, element);
                lua_settable(L, -3);
            }
        }
    }

    void LevelLoaderJSON::PushJSONValue(lua_State* L, const nlohmann::json& value) {
        if (value.is_null()) {
            lua_pushnil(L);
        }
        else if (value.is_boolean()) {
            lua_pushboolean(L, value.get<bool>());
        }
        else if (value.is_number_integer()) {
            lua_pushinteger(L, value.get<int>());
        }
        else if (value.is_number_float()) {
            lua_pushnumber(L, value.get<double>());
        }
        else if (value.is_string()) {
            lua_pushstring(L, value.get<std::string>().c_str());
        }
        else if (value.is_object() || value.is_array()) {
            // Recursively convert nested objects/arrays
            JSONToLuaTable(L, value);
        }
        else {
            // Unknown type - push nil
            LOG_WARN("LevelLoaderJSON", "Unknown JSON type, pushing nil");
            lua_pushnil(L);
        }
    }

} // namespace Framework