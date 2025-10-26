/**
===============================================================================
 File:           ConfigReader.h
 Description:    Configuration file reader with game state support
===============================================================================
 */

#pragma once
#include <string>
#include <map>

 /**
  * @class ConfigReader
  * @brief Reads and parses configuration files for game settings
  *
  * Format:
  * - Lines starting with # are comments
  * - key = value format
  * - Whitespace is trimmed
  */
class ConfigReader
{
public:
    /**
     * @brief Load and parse a configuration file
     * @param filename Path to the configuration file
     * @return true if file was successfully loaded
     */
    static bool LoadConfig(const std::string& filename);

    /**
     * @brief Get the initial game state from config
     * @param defaultState Default state if config not found or invalid
     * @return The game state enum value
     */
    static int GetInitialGameState(int defaultState);

    /**
     * @brief Get a string value from config
     * @param key The configuration key
     * @param defaultValue Default value if key not found
     * @return The configuration value or default
     */
    static std::string GetString(const std::string& key, const std::string& defaultValue = "");

    /**
     * @brief Get an integer value from config
     * @param key The configuration key
     * @param defaultValue Default value if key not found
     * @return The configuration value or default
     */
    static int GetInt(const std::string& key, int defaultValue = 0);

    /**
     * @brief Get a boolean value from config
     * @param key The configuration key
     * @param defaultValue Default value if key not found
     * @return The configuration value or default
     */
    static bool GetBool(const std::string& key, bool defaultValue = false);

    /**
     * @brief Get a float value from config
     * @param key The configuration key
     * @param defaultValue Default value if key not found
     * @return The configuration value or default
     */
    static float GetFloat(const std::string& key, float defaultValue = 0.0f);

    /**
     * @brief Check if a key exists in the config
     * @param key The configuration key
     * @return true if key exists
     */
    static bool HasKey(const std::string& key);

private:
    static std::map<std::string, std::string> configData;

    /**
     * @brief Trim whitespace from both ends of a string
     */
    static std::string Trim(const std::string& str);

    /**
     * @brief Parse state name string to enum value
     */
    static int ParseStateName(const std::string& stateName, int defaultState);
};