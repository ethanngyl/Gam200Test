/*
===============================================================================
 File:          StateScriptRegistry.cpp
 Author:        ETHAN NG
 Email:         n.ethanyongle@digipen.edu
 Date:          2026-04-05
 Contribution:  100%
 ------------------------------------------------------------------------------
 StateScriptRegistry Implementation

 Overview:
    The StateScriptRegistry module provides runtime functionality for the game engine.

===============================================================================
*/
#include "Precompiled.h"
#include "StateScriptRegistry.h"

#include "ConfigReader.h"
#include "GameStateList.h"

#include <fstream>
#include <nlohmann/json.hpp>
#include <unordered_map>

namespace {
    constexpr const char* kEmptyScriptPath = "";

    std::unordered_map<int, std::string> g_manifestScriptPaths;
    std::unordered_map<int, std::string> g_manifestStateNames;
    bool g_manifestLoaded = false;

    /**
     * @brief Returns a fallback readable state name for unmapped manifest entries.
     * @param state State identifier.
     * @return Static state name string.
     */
    const char* GetFallbackStateName(int state)
    {
        switch (state) {
        case mainMenu: return "mainMenu";
        case settingsMenu: return "settingsMenu";
        case Level_select: return "Level_select";
        case LEVEL_2: return "LEVEL_2";
        case LEVEL_3: return "LEVEL_3";
        case LEVEL_END: return "LEVEL_END";
        case TUTORIAL: return "TUTORIAL";
        case CONTROL: return "CONTROL";
        case CONTROL2: return "CONTROL2";
        case SKILL_SETS: return "SKILL_SETS";
        case WIN_SCREEN: return "WIN_SCREEN";
        case LOSE_SCREEN: return "LOSE_SCREEN";
        case DEMO_BRIDGE: return "DEMO_BRIDGE";
        case GS_QUIT: return "GS_QUIT";
        case GS_RESTART: return "GS_RESTART";
        default: return "UNKNOWN_STATE";
        }
    }

    /**
     * @brief Builds a fallback Lua script path for known scripted states.
     * @param state State identifier.
     * @return Resolved script path or nullptr when unknown.
     */
    const char* GetFallbackLevelScript(int state)
    {
        static std::string fallbackPath;
        const std::string scriptRoot = ConfigReader::GetProjectPath("script_root", "assets/scripts/");
        const auto buildPath = [&](const char* filename) -> const char* {
            fallbackPath = scriptRoot;
            if (!fallbackPath.empty() && fallbackPath.back() != '/') {
                fallbackPath.push_back('/');
            }
            fallbackPath += filename;
            return fallbackPath.c_str();
        };

        switch (state) {
        case mainMenu: return buildPath("MainMenuLevel.lua");
        case settingsMenu: return buildPath("SettingsLevel.lua");
        case Level_select: return buildPath("LevelSelectLevel.lua");
        case LEVEL_2: return buildPath("Level2.lua");
        case LEVEL_3: return buildPath("ProceduralMapLevel.lua");
        case LEVEL_END: return buildPath("EndLevel.lua");
        case TUTORIAL: return buildPath("TutorialLevel.lua");
        case CONTROL: return buildPath("ControlLevel.lua");
        case CONTROL2: return buildPath("Control2Level.lua");
        case SKILL_SETS: return buildPath("SkillSetsLevel.lua");
        case WIN_SCREEN: return buildPath("WinLevel.lua");
        case LOSE_SCREEN: return buildPath("LoseLevel.lua");
        case DEMO_BRIDGE: return buildPath("DemoBridgeLevel.lua");
        default: return nullptr;
        }
    }

    /**
     * @brief Indicates whether a state should have a Lua script mapping.
     * @param state State identifier.
     * @return True if state is scripted; otherwise false.
     */
    bool IsScriptedState(int state)
    {
        switch (state) {
        case mainMenu:
        case settingsMenu:
        case Level_select:
        case LEVEL_2:
        case LEVEL_3:
        case LEVEL_END:
        case TUTORIAL:
        case CONTROL:
        case CONTROL2:
        case SKILL_SETS:
        case WIN_SCREEN:
        case LOSE_SCREEN:
        case DEMO_BRIDGE:
            return true;
        default:
            return false;
        }
    }

    /**
     * @brief Lazily loads state script/name mappings from the registry manifest.
     */
    void LoadStateRegistryIfNeeded()
    {
        if (g_manifestLoaded) return;
        g_manifestLoaded = true;

        try {
            const std::string registryPath =
                ConfigReader::GetProjectPath("state_registry", ConfigReader::STATE_REGISTRY_PATH);
            std::ifstream file(registryPath);
            if (!file.is_open()) {
                LOG_WARN("GSM", "State registry file not found at '%s'; using fallback paths", registryPath.c_str());
                return;
            }

            nlohmann::json doc;
            file >> doc;

            if (!doc.contains("states") || !doc["states"].is_array()) {
                LOG_WARN("GSM", "State registry missing 'states' array; using fallback paths");
                return;
            }

            for (const auto& stateEntry : doc["states"]) {
                if (!stateEntry.is_object()) continue;
                if (!stateEntry.contains("id") || !stateEntry["id"].is_number_integer()) continue;

                const int id = stateEntry["id"].get<int>();
                if (stateEntry.contains("name") && stateEntry["name"].is_string()) {
                    const std::string stateName = stateEntry["name"].get<std::string>();
                    if (!stateName.empty()) {
                        g_manifestStateNames[id] = stateName;
                    }
                }

                if (stateEntry.contains("script") && stateEntry["script"].is_string()) {
                    const std::string scriptPath = stateEntry["script"].get<std::string>();
                    if (!scriptPath.empty()) {
                        g_manifestScriptPaths[id] = scriptPath;
                    }
                }
            }

            LOG_INFO("GSM", "Loaded state registry script paths from %s (%zu entries)",
                registryPath.c_str(), g_manifestScriptPaths.size());
        }
        catch (const nlohmann::json::exception& e) {
            LOG_WARN("GSM", "Failed to parse state registry JSON: %s", e.what());
        }
        catch (const std::exception& e) {
            LOG_WARN("GSM", "Failed to load state registry: %s", e.what());
        }
    }
}

namespace Framework::StateScriptRegistry {

/**
 * @brief Gets the Lua script path for a state id.
 * @param state State identifier.
 * @return Script path string, or empty string when unavailable.
 */
const char* GetLevelScript(int state)
{
    LoadStateRegistryIfNeeded();

    auto it = g_manifestScriptPaths.find(state);
    if (it != g_manifestScriptPaths.end() && !it->second.empty()) {
        return it->second.c_str();
    }

    const char* fallback = GetFallbackLevelScript(state);
    if (fallback && fallback[0] != '\0') {
        return fallback;
    }

    if (IsScriptedState(state)) {
        LOG_ERROR("GSM", "No script mapping found for scripted state id=%d", state);
    }
    return kEmptyScriptPath;
}

/**
 * @brief Gets a stable display/log name for a state id.
 * @param state State identifier.
 * @return State name string from manifest or fallback table.
 */
const char* GetStateName(int state)
{
    LoadStateRegistryIfNeeded();

    auto it = g_manifestStateNames.find(state);
    if (it != g_manifestStateNames.end() && !it->second.empty()) {
        return it->second.c_str();
    }
    return GetFallbackStateName(state);
}

/**
 * @brief Validates that required scripted states resolve to script paths.
 * @return True when all required states are mapped; otherwise false.
 */
bool ValidateScriptMappingsAtStartup()
{
    const int requiredStates[] = {
        mainMenu,
        settingsMenu,
        Level_select,
        LEVEL_2,
        LEVEL_3,
        LEVEL_END,
        TUTORIAL,
        CONTROL,
        CONTROL2,
        SKILL_SETS,
        WIN_SCREEN,
        LOSE_SCREEN
    };

    bool ok = true;
    for (int state : requiredStates) {
        const char* path = GetLevelScript(state);
        if (!path || path[0] == '\0') {
            LOG_ERROR("GSM", "Missing script path for required state id=%d", state);
            ok = false;
        }
    }

    if (ok) {
        LOG_INFO("GSM", "State script mapping validation passed");
    }
    else {
        LOG_ERROR("GSM", "State script mapping validation failed");
    }
    return ok;
}

} // namespace Framework::StateScriptRegistry
