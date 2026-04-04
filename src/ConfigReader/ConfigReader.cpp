/*
===============================================================================
 File:          ConfigReader.cpp
 Author:        GE YONGQI
 Email:         yongqi.ge@digipen.edu
 Date:          2025-10-31
 Contribution:  100%
 ------------------------------------------------------------------------------
  Configuration file reader for the engine (implementation)

  Design notes:
     - Prevents duplicate loading using a cached config path
     - Supports parsing of int, float, bool, and string values
     - Automatically loads the default config if not yet loaded
     - Uses logging macros for diagnostics and error reporting

  Thread-safety:
     - All methods are static and rely on internal global state
     - Not thread-safe by design (single-threaded initialization expected)
===============================================================================
*/


#include "Precompiled.h"

#include <fstream>
#include <nlohmann/json.hpp>
#include <unordered_map>

namespace {
    std::unordered_map<std::string, int> g_stateAliasToEnum;
    bool g_stateRegistryLoaded = false;
    std::unordered_map<std::string, std::string> g_projectPaths;
    bool g_projectPathsLoaded = false;

    std::string ToLowerASCII(const std::string& input)
    {
        std::string out = input;
        std::transform(out.begin(), out.end(), out.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return out;
    }

    void AddAlias(const std::string& alias, int state)
    {
        if (alias.empty()) return;
        g_stateAliasToEnum[ToLowerASCII(alias)] = state;
    }

    void LoadStateRegistryAliases()
    {
        if (g_stateRegistryLoaded) {
            return;
        }

        g_stateRegistryLoaded = true;

        try {
            const std::string stateRegistryPath =
                ConfigReader::GetProjectPath("state_registry", ConfigReader::STATE_REGISTRY_PATH);
            std::ifstream file(stateRegistryPath);
            if (!file.is_open()) {
                LOG_WARN("CONFIG", "State registry not found at '%s'; using fallback aliases",
                    stateRegistryPath.c_str());
                return;
            }

            nlohmann::json doc;
            file >> doc;

            if (!doc.contains("states") || !doc["states"].is_array()) {
                LOG_WARN("CONFIG", "State registry missing 'states' array; using fallback aliases");
                return;
            }

            for (const auto& stateEntry : doc["states"]) {
                if (!stateEntry.is_object()) continue;
                if (!stateEntry.contains("id") || !stateEntry["id"].is_number_integer()) continue;

                const int stateId = stateEntry["id"].get<int>();

                if (stateEntry.contains("name") && stateEntry["name"].is_string()) {
                    AddAlias(stateEntry["name"].get<std::string>(), stateId);
                }

                if (stateEntry.contains("aliases") && stateEntry["aliases"].is_array()) {
                    for (const auto& alias : stateEntry["aliases"]) {
                        if (alias.is_string()) {
                            AddAlias(alias.get<std::string>(), stateId);
                        }
                    }
                }
            }

            LOG_INFO("CONFIG", "Loaded state aliases from %s (%zu entries)",
                stateRegistryPath.c_str(), g_stateAliasToEnum.size());
        }
        catch (const nlohmann::json::exception& e) {
            LOG_WARN("CONFIG", "Failed to parse state registry JSON: %s", e.what());
        }
        catch (const std::exception& e) {
            LOG_WARN("CONFIG", "Failed to load state registry: %s", e.what());
        }
    }

    void LoadProjectPathsIfNeeded()
    {
        if (g_projectPathsLoaded) {
            return;
        }

        g_projectPathsLoaded = true;

        try {
            std::ifstream file(ConfigReader::PROJECT_PATHS_FILE_PATH);
            if (!file.is_open()) {
                LOG_WARN("CONFIG", "Project paths file not found at '%s'",
                    ConfigReader::PROJECT_PATHS_FILE_PATH);
                return;
            }

            nlohmann::json doc;
            file >> doc;

            if (doc.contains("paths") && doc["paths"].is_object()) {
                const auto& paths = doc["paths"];
                for (auto it = paths.begin(); it != paths.end(); ++it) {
                    if (it.value().is_string()) {
                        g_projectPaths[it.key()] = it.value().get<std::string>();
                    }
                }
            }
            else if (doc.is_object()) {
                for (auto it = doc.begin(); it != doc.end(); ++it) {
                    if (it.value().is_string()) {
                        g_projectPaths[it.key()] = it.value().get<std::string>();
                    }
                }
            }

            LOG_INFO("CONFIG", "Loaded project paths from %s (%zu entries)",
                ConfigReader::PROJECT_PATHS_FILE_PATH, g_projectPaths.size());
        }
        catch (const nlohmann::json::exception& e) {
            LOG_WARN("CONFIG", "Failed to parse project paths JSON: %s", e.what());
        }
        catch (const std::exception& e) {
            LOG_WARN("CONFIG", "Failed to load project paths: %s", e.what());
        }
    }
}


 // ============================================================================
 // STATIC MEMBER DEFINITIONS
 // ============================================================================
std::map<std::string, std::string> ConfigReader::configData;
bool ConfigReader::configLoaded = false;
std::string ConfigReader::loadedConfigPath = "";

// ============================================================================
// PUBLIC METHODS
// ============================================================================

bool ConfigReader::LoadConfig(const std::string& filename)
{
    const std::string resolvedPath =
        (filename == CONFIG_FILE_PATH)
        ? GetProjectPath("game_config", CONFIG_FILE_PATH)
        : filename;

    // Check if this file is already loaded
    if (configLoaded && loadedConfigPath == resolvedPath) {
        LOG_INFO("CONFIG", "Configuration already loaded from: %s", resolvedPath.c_str());
        return true;
    }

    // Load the config file
    return LoadConfigInternal(resolvedPath);
}

bool ConfigReader::ReloadConfig(const std::string& filename)
{
    const std::string resolvedPath =
        (filename == CONFIG_FILE_PATH)
        ? GetProjectPath("game_config", CONFIG_FILE_PATH)
        : filename;

    LOG_INFO("CONFIG", "Force reloading configuration...");
    configLoaded = false;
    loadedConfigPath = "";
    return LoadConfigInternal(resolvedPath);
}

bool ConfigReader::IsConfigLoaded()
{
    return configLoaded;
}

int ConfigReader::GetInitialGameState(int defaultState)
{
    // Always reload the game config explicitly.
    // Another config file (e.g. valueloader.txt) may have been loaded after the
    // initial game_config.txt load, which clears configData via LoadConfigInternal.
    // Calling LoadConfig here re-reads game_config.txt when a different file is
    // currently cached, ensuring initial_state is always present.
    LoadConfig(GetProjectPath("game_config", CONFIG_FILE_PATH));

    // Get initial_state value
    std::string stateName = GetString("initial_state", "");

    if (stateName.empty()) {
        LOG_WARN("CONFIG", "No initial_state specified, using default");
        return defaultState;
    }

    // Parse state name to enum
    int state = ResolveStateName(stateName, defaultState);

    LOG_INFO("CONFIG", "Initial game state: %s (enum value: %d)", stateName.c_str(), state);

    return state;
}

int ConfigReader::ResolveStateName(const std::string& stateName, int defaultState)
{
    return ParseStateName(stateName, defaultState);
}

std::string ConfigReader::GetProjectPath(const std::string& key, const std::string& defaultValue)
{
    LoadProjectPathsIfNeeded();

    auto it = g_projectPaths.find(key);
    if (it != g_projectPaths.end() && !it->second.empty()) {
        return it->second;
    }
    return defaultValue;
}

std::string ConfigReader::GetString(const std::string& key, const std::string& defaultValue)
{
    auto it = configData.find(key);
    if (it != configData.end()) {
        return it->second;
    }
    return defaultValue;
}

int ConfigReader::GetInt(const std::string& key, int defaultValue)
{
    auto it = configData.find(key);
    if (it != configData.end()) {
        try {
            return std::stoi(it->second);
        }
        catch (const std::exception& e) {
            LOG_WARN("CONFIG", "Failed to parse int for key '%s': %s", key.c_str(), e.what());
            return defaultValue;
        }
    }
    return defaultValue;
}

bool ConfigReader::GetBool(const std::string& key, bool defaultValue)
{
    auto it = configData.find(key);
    if (it != configData.end()) {
        std::string value = it->second;
        // Convert to lowercase for comparison
        std::transform(value.begin(), value.end(), value.begin(),
            [](unsigned char c) { return (char)std::tolower(c); });
        return (value == "true" || value == "1" || value == "yes" || value == "on");
    }
    return defaultValue;
}

float ConfigReader::GetFloat(const std::string& key, float defaultValue)
{
    auto it = configData.find(key);
    if (it != configData.end()) {
        try {
            return std::stof(it->second);
        }
        catch (const std::exception& e) {
            LOG_WARN("CONFIG", "Failed to parse float for key '%s': %s", key.c_str(), e.what());
            return defaultValue;
        }
    }
    return defaultValue;
}

bool ConfigReader::HasKey(const std::string& key)
{
    return configData.find(key) != configData.end();
}

bool ConfigReader::SetFloat(const std::string& key, float value)
{
    // Convert float to string with precision
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << value;

    configData[key] = oss.str();

    // Save to file
    return SaveConfig();
}

bool ConfigReader::SetString(const std::string& key, const std::string& value)
{
    configData[key] = value;

    // Save to file
    return SaveConfig();
}

bool ConfigReader::SaveConfig(const std::string& filename)
{
    // Use loaded config path if no filename specified
    std::string targetFile = filename.empty() ? loadedConfigPath : filename;

    if (targetFile.empty()) {
        targetFile = GetProjectPath("game_config", CONFIG_FILE_PATH);
    }

    // Read the original file to preserve comments and structure
    std::ifstream inFile(targetFile);
    std::vector<std::string> lines;
    std::map<std::string, bool> keysWritten;

    if (inFile.is_open()) {
        std::string line;
        while (std::getline(inFile, line)) {
            // Check if this line contains a key we have in configData
            size_t pos = line.find('=');
            if (pos != std::string::npos && !line.empty() && line[0] != '#') {
                std::string key = Trim(line.substr(0, pos));

                // If we have this key in our data, update it
                if (configData.find(key) != configData.end()) {
                    lines.push_back(key + " = " + configData[key]);
                    keysWritten[key] = true;
                } else {
                    lines.push_back(line);
                }
            } else {
                // Preserve comments and empty lines
                lines.push_back(line);
            }
        }
        inFile.close();
    }

    // Add any new keys that weren't in the original file
    for (const auto& pair : configData) {
        if (keysWritten.find(pair.first) == keysWritten.end()) {
            lines.push_back(pair.first + " = " + pair.second);
        }
    }

    // Write everything back to file
    std::ofstream outFile(targetFile);
    if (!outFile.is_open()) {
        LOG_ERROR("CONFIG", "Failed to open config file for writing: %s", targetFile.c_str());
        return false;
    }

    for (const auto& line : lines) {
        outFile << line << "\n";
    }

    outFile.close();
    LOG_INFO("CONFIG", "Configuration saved to: %s", targetFile.c_str());
    return true;
}

void ConfigReader::Shutdown()
{
    // Force deallocation using swap trick - don't call clear() first
    std::map<std::string, std::string>().swap(configData);

    // Clear and shrink the string to free its buffer
    loadedConfigPath.clear();
    loadedConfigPath.shrink_to_fit();

    std::unordered_map<std::string, int>().swap(g_stateAliasToEnum);
    std::unordered_map<std::string, std::string>().swap(g_projectPaths);
    g_stateRegistryLoaded = false;
    g_projectPathsLoaded = false;

    configLoaded = false;

    LOG_INFO("CONFIG", "ConfigReader shutdown complete - static resources freed");
}

// ============================================================================
// PRIVATE HELPER METHODS
// ============================================================================

bool ConfigReader::LoadConfigInternal(const std::string& filename)
{
    configData.clear();

    std::ifstream file(filename);
    if (!file.is_open()) {
        LOG_WARN("CONFIG", "Could not open config file: %s", filename.c_str());
        return false;
    }

    LOG_INFO("CONFIG", "Loading configuration from: %s", filename.c_str());

    std::string line;
    int lineNumber = 0;

    while (std::getline(file, line)) {
        lineNumber++;

        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') {
            continue;
        }

        // Find the '=' separator
        size_t pos = line.find('=');
        if (pos == std::string::npos) {
            LOG_WARN("CONFIG", "Invalid config line %d: %s", lineNumber, line.c_str());
            continue;
        }

        // Extract key and value
        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);

        // Trim whitespace
        key = Trim(key);
        value = Trim(value);

        if (key.empty()) {
            LOG_WARN("CONFIG", "Empty key on line %d", lineNumber);
            continue;
        }

        // Store in map
        configData[key] = value;
        LOG_INFO("CONFIG", "  %s = %s", key.c_str(), value.c_str());
    }

    file.close();

    // Mark as loaded
    configLoaded = true;
    loadedConfigPath = filename;

    LOG_INFO("CONFIG", "Configuration loaded successfully (%zu entries)", configData.size());
    return true;
}

std::string ConfigReader::Trim(const std::string& str)
{
    size_t start = 0;
    size_t end = str.length();

    // Trim from start
    while (start < end && std::isspace((unsigned char)str[start])) {
        ++start;
    }

    // Trim from end
    while (end > start && std::isspace((unsigned char)str[end - 1])) {
        --end;
    }

    return str.substr(start, end - start);
}

int ConfigReader::ParseStateName(const std::string& stateName, int defaultState)
{
    LoadStateRegistryAliases();

    const std::string lowerName = ToLowerASCII(stateName);

    auto it = g_stateAliasToEnum.find(lowerName);
    if (it != g_stateAliasToEnum.end()) {
        return it->second;
    }

    // Backward-compatible fallback aliases.
    if (lowerName == "mainmenu" || lowerName == "main_menu") return mainMenu;
    if (lowerName == "level2" || lowerName == "level_2") return LEVEL_2;
    if (lowerName == "quit" || lowerName == "exit") return GS_QUIT;

    LOG_WARN("CONFIG", "Unknown state name '%s', using default", stateName.c_str());
    return defaultState;
}