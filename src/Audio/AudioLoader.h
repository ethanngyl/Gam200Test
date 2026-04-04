/*
===============================================================================
 File:          AudioLoader.h
 Author:        ETHAN NG
 Email:         n.ethanyongle@digipen.edu
 Date:          2025-10-31
 Contribution:  100%
 ------------------------------------------------------------------------------
 Audio Loader Header

 Overview:
    The AudioLoader class serves as a static utility system responsible for
    parsing external configuration files (JSON) and populating the AudioSystem.
    It acts as the bridge between data files and the runtime audio engine.

 Key Features:
    - JSON-based configuration parsing (using nlohmann::json)
    - Automated batch loading of sound assets
    - Persistent settings management (Master/Music/SFX volume)
    - Serialization of runtime volume changes back to disk
    - Support for sound properties (looping, preloading, individual volume)

===============================================================================
*/

#pragma once
#include "Precompiled.h"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace Framework {

    // Forward declarations
    class AudioSystem;

    /**
     * @struct AudioConfig
     * @brief Data structure representing a single sound entry parsed from JSON
     */
    struct AudioConfig {
        std::string name;       // Sound identifier (e.g., "leaves", "bgm")
        std::string filepath;   // Path to audio file relative to executable
        float volume = 1.0f;    // Default playback volume (0.0 to 1.0)
        bool preload = true;    // If true, load on startup; if false, load on demand
        bool loop = false;      // If true, sound loops by default
    };

    /**
     * @struct AudioSettings
     * @brief Data structure representing global audio volume settings
     */
    struct AudioSettings {
        float masterVolume = 1.0f;  // Global scalar for all audio
        float musicVolume = 1.0f;   // Scalar for BGM category
        float sfxVolume = 1.0f;     // Scalar for SFX category
    };

    /**
     * @class AudioLoader
     * @brief Static utility class for serializing and deserializing audio data
     *
     * The AudioLoader abstracts the complexity of file I/O and JSON parsing
     * away from the main AudioSystem. It handles:
     * - Reading 'AudioConfig.json'
     * - Parsing sound definitions and global settings
     * - Instructing the AudioSystem to load specific assets
     * - Saving runtime volume changes back to disk
     *
     * Usage Pattern:
     * @code
     * // Init
    * AudioLoader::LoadAudioConfig(ConfigReader::GetProjectPath("audio_config", "<default>"), audioSystem);
     * * // Runtime Volume Change (Auto-saves)
     * AudioLoader::SetMasterVolume(0.5f);
     * @endcode
     */
    class AudioLoader {
    public:
        /**
         * @brief Parses a JSON config file and loads sounds into the AudioSystem
         * @param filepath Path to the JSON configuration file
         * @param audioSystem Pointer to the AudioSystem instance to populate
         * @return true if parsing and loading were successful, false otherwise
         *
         * Loading Process:
         * 1. Validates the AudioSystem pointer
         * 2. Opens and deserializes the JSON file via std::ifstream
         * 3. Parses "sounds" array into AudioConfig structs
         * 4. Parses "settings" object into AudioSettings struct
         * 5. Calls LoadSoundsIntoSystem to register assets with the engine
         */
        static bool LoadAudioConfig(const std::string& filepath, AudioSystem* audioSystem);

        /**
         * @brief Retrieves the list of currently loaded sound configurations
         * @return Const reference to the vector of AudioConfig
         */
        static const std::vector<AudioConfig>& GetLoadedConfigs() { return loadedConfigs; }

        /**
         * @brief Retrieves the current global audio settings
         * @return Const reference to the AudioSettings struct
         */
        static const AudioSettings& GetSettings() { return settings; }

        /**
         * @brief Sets the master volume and immediately saves it to the config file
         * @param volume Desired master volume (clamped 0.0 to 1.0)
         * @param filepath Path to audio config JSON (empty uses project path manifest)
         * @return true if the configuration was successfully saved to disk
         *
         * Implementation Details:
         * - Clamps input volume to valid range
         * - Updates internal settings state
         * - Triggers SaveAudioConfig to persist changes
         */
        static bool SetMasterVolume(float volume, const std::string& filepath = "");

        /**
         * @brief Serializes the current audio settings back to the JSON file
         * @param filepath Path to audio config JSON (empty uses project path manifest)
         * @return true if file writing was successful
         *
         * Save Process:
         * 1. Reads existing JSON (to preserve non-settings data like sound lists)
         * 2. Updates the "settings" object with current volume values
         * 3. Writes the modified JSON back to disk with pretty-printing
         */
        static bool SaveAudioConfig(const std::string& filepath = "");

        /**
         * @brief Clean up static resources and free memory
         * @note Call this before program exit to prevent memory leak reports
         */
        static void Shutdown();

    private:
        /**
         * @brief Internal helper to parse raw JSON into internal structures
         * @param json The nlohmann::json object
         * @return true if parsing was successful and configs are not empty
         *
         * Implementation Details:
         * - Extracts "name" and "filepath" (Required)
         * - Extracts "volume", "preload", "loop" (Optional)
         * - Populates the static loadedConfigs vector
         */
        static bool ParseAudioJSON(const nlohmann::json& json);

        /**
         * @brief Internal helper to register parsed configs with the AudioSystem
         * @param audioSystem Target system
         * @return true if all preload sounds loaded successfully
         *
         * Implementation Details:
         * - Iterates through loadedConfigs
         * - Checks 'preload' flag; skips deferred sounds
         * - Calls AudioSystem::LoadSound for valid entries
         */
        static bool LoadSoundsIntoSystem(AudioSystem* audioSystem);

        // Static storage for loaded configurations to avoid reloading
        static std::vector<AudioConfig> loadedConfigs;

        // Static storage for global settings
        static AudioSettings settings;
    };

} // namespace Framework