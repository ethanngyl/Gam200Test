/*
===============================================================================
 File:          ConfigReader.h
 Author:        GE YONGQI
 Email:         yongqi.ge@digipen.edu
 Date:          2025-10-31
 Contribution:  100%
 ------------------------------------------------------------------------------
  Configuration file reader for game settings (header)

  Features:
     - Prevents duplicate loading of configuration file
     - Unified config file path constant
     - Supports multiple data types (int, float, bool, string)
     - Automatic game state parsing

  Format:
     - Lines starting with '#' are comments
     - key = value format
     - Whitespace is trimmed

  Example:
     fullscreen = true
     initial_state = Level1
===============================================================================
*/

#pragma once
#include <string>
#include <map>

 /**
  * @class ConfigReader
  * @brief Reads and parses configuration files for game settings
  *
  * Features:
  * - Prevents duplicate loading of config file
  * - Unified config file path constant
  * - Supports multiple data types (int, float, bool, string)
  * - Automatic game state parsing
  *
  * Format:
  * - Lines starting with # are comments
  * - key = value format
  * - Whitespace is trimmed
  */
class ConfigReader
{
public:
    // ========================================================================
    // CONFIGURATION PATH
    // ========================================================================
    // Unified config file path - change this to match your project structure
    static constexpr const char* CONFIG_FILE_PATH = "assets/game_config.txt";

    // ========================================================================
    // PUBLIC INTERFACE
    // ========================================================================

    /**
     * @brief Load and parse a configuration file
     * @param filename Path to the configuration file
     * @return true if file was successfully loaded
     * @note Calling this multiple times with the same file is safe (won't reload)
     */
    static bool LoadConfig(const std::string& filename = CONFIG_FILE_PATH);

    /**
     * @brief Force reload the configuration file
     * @param filename Path to the configuration file
     * @return true if file was successfully loaded
     */
    static bool ReloadConfig(const std::string& filename = CONFIG_FILE_PATH);

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

    /**
     * @brief Check if configuration has been loaded
     * @return true if config is loaded
     */
    static bool IsConfigLoaded();

    /**
     * @brief Set a float value in config and save to file
     * @param key The configuration key
     * @param value The value to set
     * @return true if successfully saved
     */
    static bool SetFloat(const std::string& key, float value);

    /**
     * @brief Set a string value in config and save to file
     * @param key The configuration key
     * @param value The value to set
     * @return true if successfully saved
     */
    static bool SetString(const std::string& key, const std::string& value);

    /**
     * @brief Save current config data to file
     * @param filename Path to save the config file (defaults to loaded config path)
     * @return true if successfully saved
     */
    static bool SaveConfig(const std::string& filename = "");

private:
    // ========================================================================
    // PRIVATE MEMBERS
    // ========================================================================
    static std::map<std::string, std::string> configData;
    static bool configLoaded;
    static std::string loadedConfigPath;

    /**
     * @brief Trim whitespace from both ends of a string
     */
    static std::string Trim(const std::string& str);

    /**
     * @brief Parse state name string to enum value
     */
    static int ParseStateName(const std::string& stateName, int defaultState);

    /**
     * @brief Internal load function (without duplicate check)
     */
    static bool LoadConfigInternal(const std::string& filename);
};