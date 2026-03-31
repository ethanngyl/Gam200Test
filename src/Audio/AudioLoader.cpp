/*
===============================================================================
 File:          AudioLoader.cpp
 Author:        ETHAN NG
 Email:         n.ethanyongle@digipen.edu
 Date:          2025-10-31
 Contribution:  100%
 ------------------------------------------------------------------------------
  Audio Loader Implementation

 Overview:
    The AudioLoader class is responsible for parsing external configuration files
    (JSON) and loading audio assets into the AudioSystem. It handles the deserialization
    of audio data, volume settings, and manages file I/O operations.

  Design notes:
     - Uses nlohmann::json for robust JSON parsing
     - static member initialization for configs and settings
     - Separates parsing logic from system loading logic
     - Includes exception handling for file I/O and JSON errors
===============================================================================
*/

#include "Precompiled.h"
#include "AudioLoader.h"
#include "AudioSystem.h"
#include <fstream>
#include <algorithm>

namespace Framework {

    // Static member initialization
    std::vector<AudioConfig> AudioLoader::loadedConfigs;
    AudioSettings AudioLoader::settings;

    /**
     * @brief Loads audio configuration from a specified JSON file
     * @params filepath, The path to the JSON configuration file
     * @params audioSystem, Pointer to the AudioSystem instance to populate
     * @return True if loading and parsing were successful, false otherwise
     *
     * Implementation details:
     * - Validates that the AudioSystem pointer is not null before proceeding
     * - Uses std::ifstream to open the file at the specified filepath
     * - Utilizes nlohmann::json library to parse the file content into a JSON object
     * - Calls internal helper ParseAudioJSON to populate data structures
     * - Calls LoadSoundsIntoSystem to register the parsed sounds with the engine
     * - Wrapped in try-catch blocks to handle JSON syntax errors or general exceptions
     */
    bool AudioLoader::LoadAudioConfig(const std::string& filepath, AudioSystem* audioSystem) {
        if (!audioSystem) {
            LOG_ERROR("AudioLoader", "AudioSystem is null!");
            return false;
        }

        LOG_INFO("AudioLoader", "Loading audio configuration from: %s", filepath.c_str());

        // Read JSON file
        std::ifstream file(filepath);
        if (!file.is_open()) {
            LOG_ERROR("AudioLoader", "Failed to open audio config file: %s", filepath.c_str());
            return false;
        }

        try {
            // Parse JSON
            nlohmann::json jsonData;
            file >> jsonData;
            file.close();

            // Parse configuration
            if (!ParseAudioJSON(jsonData)) {
                LOG_ERROR("AudioLoader", "Failed to parse audio JSON");
                return false;
            }

            // Load sounds into AudioSystem
            if (!LoadSoundsIntoSystem(audioSystem)) {
                LOG_ERROR("AudioLoader", "Failed to load sounds into AudioSystem");
                return false;
            }

            LOG_INFO("AudioLoader", "Successfully loaded %zu audio files", loadedConfigs.size());
            return true;
        }
        catch (const nlohmann::json::exception& e) {
            LOG_ERROR("AudioLoader", "JSON parsing error: %s", e.what());
            file.close();
            return false;
        }
        catch (const std::exception& e) {
            LOG_ERROR("AudioLoader", "Error loading audio config: %s", e.what());
            file.close();
            return false;
        }
    }

    /**
     * @brief Parses the raw JSON object into internal audio configuration structures
     * @params json, The loaded nlohmann::json object
     * @return True if configurations were successfully parsed, false if empty
     *
     * Implementation details:
     * - Clears any previously loaded configurations to ensure a clean state
     * - Checks for the existence of the "sounds" array in the JSON object
     * - Iterates through the array to extract "name" and "filepath" (mandatory fields)
     * - Extracts optional fields like "volume", "preload", and "loop" if they exist
     * - Logs warnings if mandatory fields are missing
     * - Separately parses the "settings" object for global volume controls (master, music, sfx)
     */
    bool AudioLoader::ParseAudioJSON(const nlohmann::json& json) {
        loadedConfigs.clear();

        // Parse sounds array
        if (json.contains("sounds") && json["sounds"].is_array()) {
            for (const auto& soundJson : json["sounds"]) {
                AudioConfig config;

                // Required fields
                if (!soundJson.contains("name") || !soundJson.contains("filepath")) {
                    LOG_WARN("AudioLoader", "Sound entry missing 'name' or 'filepath', skipping");
                    continue;
                }

                config.name = soundJson["name"].get<std::string>();
                config.filepath = soundJson["filepath"].get<std::string>();

                // Optional fields
                if (soundJson.contains("volume")) {
                    config.volume = soundJson["volume"].get<float>();
                }
                if (soundJson.contains("preload")) {
                    config.preload = soundJson["preload"].get<bool>();
                }
                if (soundJson.contains("loop")) {
                    config.loop = soundJson["loop"].get<bool>();
                }

                loadedConfigs.push_back(config);
                LOG_INFO("AudioLoader", "Parsed audio: '%s' -> '%s'",
                    config.name.c_str(), config.filepath.c_str());
            }
        }
        else {
            LOG_WARN("AudioLoader", "No 'sounds' array found in JSON");
        }

        // Parse settings
        if (json.contains("settings") && json["settings"].is_object()) {
            const auto& settingsJson = json["settings"];

            if (settingsJson.contains("masterVolume")) {
                settings.masterVolume = settingsJson["masterVolume"].get<float>();
            }
            if (settingsJson.contains("musicVolume")) {
                settings.musicVolume = settingsJson["musicVolume"].get<float>();
            }
            if (settingsJson.contains("sfxVolume")) {
                settings.sfxVolume = settingsJson["sfxVolume"].get<float>();
            }

            LOG_INFO("AudioLoader", "Audio settings: Master=%.2f, Music=%.2f, SFX=%.2f",
                settings.masterVolume, settings.musicVolume, settings.sfxVolume);
        }

        return !loadedConfigs.empty();
    }

    /**
     * @brief Registers the parsed configurations with the main AudioSystem
     * @params audioSystem, Pointer to the system where sounds should be loaded
     * @return True if all attempted loads were successful, false if any failed
     *
     * Implementation details:
     * - Iterates through the static loadedConfigs vector
     * - Checks the 'preload' flag; if false, the sound is skipped for deferred loading
     * - Calls AudioSystem::LoadSound for each valid configuration
     * - Maintains a count of successes and failures for logging purposes
     * - Returns false if failCount is greater than 0, ensuring integrity
     */
    bool AudioLoader::LoadSoundsIntoSystem(AudioSystem* audioSystem) {
        int successCount = 0;
        int failCount = 0;

        for (const auto& config : loadedConfigs) {
            // Only load if preload is true
            if (!config.preload) {
                LOG_INFO("AudioLoader", "Skipping '%s' (preload=false)", config.name.c_str());
                continue;
            }

            LOG_INFO("AudioLoader", "Loading sound: '%s' from '%s'",
                config.name.c_str(), config.filepath.c_str());

            // Load sound into AudioSystem
            bool success = audioSystem->LoadSound(config.filepath, config.name);

            if (success) {
                successCount++;
                LOG_INFO("AudioLoader", "Successfully loaded: '%s'", config.name.c_str());
            }
            else {
                failCount++;
                LOG_ERROR("AudioLoader", "Failed to load: '%s'", config.name.c_str());
            }
        }

        LOG_INFO("AudioLoader", "Audio loading complete: %d succeeded, %d failed",
            successCount, failCount);

        return failCount == 0;
    }

    /**
     * @brief Sets the master volume and updates the persistent configuration file
     * @params volume, The desired volume level (0.0 to 1.0)
     * @params filepath, The path to the config file to update
     * @return True if the configuration was saved successfully
     *
     * Implementation details:
     * - Clamps the input volume between 0.0f and 1.0f using std::max and std::min
     * - Updates the static settings structure with the new volume
     * - Calls SaveAudioConfig to write the changes to disk immediately
     */
    bool AudioLoader::SetMasterVolume(float volume, const std::string& filepath) {
        // Clamp volume to valid range
        settings.masterVolume = (std::max)(0.0f, (std::min)(1.0f, volume));

        // Save to JSON file
        return SaveAudioConfig(filepath);
    }

    bool AudioLoader::SetMusicVolume(float volume, const std::string& filepath) {
        settings.musicVolume = (std::max)(0.0f, (std::min)(1.0f, volume));
        return SaveAudioConfig(filepath);
    }

    bool AudioLoader::SetSfxVolume(float volume, const std::string& filepath) {
        settings.sfxVolume = (std::max)(0.0f, (std::min)(1.0f, volume));
        return SaveAudioConfig(filepath);
    }

    /**
     * @brief Serializes the current audio settings back to the JSON file
     * @params filepath, The path to the JSON file to write
     * @return True if file writing was successful
     *
     * Implementation details:
     * - Reads the existing JSON file first to preserve data not related to settings (like the sounds array)
     * - Checks if the "settings" object exists, creating it if necessary
     * - Updates master, music, and sfx volume fields in the JSON object
     * - Uses std::ofstream to write the modified JSON back to the file
     * - Uses jsonData.dump(2) for pretty-printing with 2-space indentation
     * - Exception handling ensures file corruption is minimized on error
     */
    bool AudioLoader::SaveAudioConfig(const std::string& filepath) {
        try {
            const std::string resolvedPath =
                filepath.empty()
                ? ConfigReader::GetProjectPath("audio_config", "assets/JSON/AudioConfig.json")
                : filepath;

            // Read existing JSON file
            std::ifstream inFile(resolvedPath);
            if (!inFile.is_open()) {
                LOG_ERROR("AudioLoader", "Failed to open audio config file for reading: %s", resolvedPath.c_str());
                return false;
            }

            nlohmann::json jsonData;
            inFile >> jsonData;
            inFile.close();

            // Update settings section
            if (!jsonData.contains("settings")) {
                jsonData["settings"] = nlohmann::json::object();
            }

            jsonData["settings"]["masterVolume"] = settings.masterVolume;
            jsonData["settings"]["musicVolume"] = settings.musicVolume;
            jsonData["settings"]["sfxVolume"] = settings.sfxVolume;

            // Write back to file with pretty formatting
            std::ofstream outFile(resolvedPath);
            if (!outFile.is_open()) {
                LOG_ERROR("AudioLoader", "Failed to open audio config file for writing: %s", resolvedPath.c_str());
                return false;
            }

            outFile << jsonData.dump(2);  // 2-space indentation
            outFile.close();

            LOG_INFO("AudioLoader", "Audio config saved to: %s (Master Volume: %.2f)",
                resolvedPath.c_str(), settings.masterVolume);
            return true;
        }
        catch (const std::exception& e) {
            LOG_ERROR("AudioLoader", "Error saving audio config: %s", e.what());
            return false;
        }
    }

    void AudioLoader::Shutdown() {
        // Force deallocation using swap trick - don't call clear() first
        std::vector<AudioConfig>().swap(loadedConfigs);

        // Reset settings to defaults
        settings = AudioSettings();

        LOG_INFO("AudioLoader", "AudioLoader shutdown complete - static resources freed");
    }

} // namespace Framework