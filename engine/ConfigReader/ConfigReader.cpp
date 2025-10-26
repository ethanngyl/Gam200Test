/**
===============================================================================
 File:           ConfigReader.cpp (Optimized Version)
 Description:    Configuration file reader - Implementation with duplicate
                 loading prevention
===============================================================================
 */

#include "Precompiled.h"
#include "ConfigReader.h"
#include "GSM/GameStateList.h"
#include <fstream>
#include <sstream>
#include <algorithm>

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
    // Check if this file is already loaded
    if (configLoaded && loadedConfigPath == filename) {
        LOG_INFO("CONFIG", "Configuration already loaded from: %s", filename.c_str());
        return true;
    }

    // Load the config file
    return LoadConfigInternal(filename);
}

bool ConfigReader::ReloadConfig(const std::string& filename)
{
    LOG_INFO("CONFIG", "Force reloading configuration...");
    configLoaded = false;
    loadedConfigPath = "";
    return LoadConfigInternal(filename);
}

bool ConfigReader::IsConfigLoaded()
{
    return configLoaded;
}

int ConfigReader::GetInitialGameState(int defaultState)
{
    // Ensure config is loaded
    if (!configLoaded) {
        LoadConfig(CONFIG_FILE_PATH);
    }

    // Get initial_state value
    std::string stateName = GetString("initial_state", "");

    if (stateName.empty()) {
        LOG_WARN("CONFIG", "No initial_state specified, using default");
        return defaultState;
    }

    // Parse state name to enum
    int state = ParseStateName(stateName, defaultState);

    LOG_INFO("CONFIG", "Initial game state: %s (enum value: %d)", stateName.c_str(), state);

    return state;
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
    // Convert to lowercase for case-insensitive comparison
    std::string lowerName = stateName;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(),
        [](unsigned char c) { return (char)std::tolower(c); });

    // Parse state names
    if (lowerName == "mainmenu" || lowerName == "main_menu") {
        return mainMenu;
    }
    else if (lowerName == "level1" || lowerName == "level_1") {
        return LEVEL_1;
    }
    else if (lowerName == "level2" || lowerName == "level_2") {
        return LEVEL_2;
    }
    else if (lowerName == "quit" || lowerName == "exit") {
        return GS_QUIT;
    }
    else {
        LOG_WARN("CONFIG", "Unknown state name '%s', using default", stateName.c_str());
        return defaultState;
    }
}