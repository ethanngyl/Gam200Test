#pragma once
#include "Precompiled.h"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace Framework {

    // Forward declarations
    class AudioSystem;

    /**
     * @brief Audio file configuration structure
     */
    struct AudioConfig {
        std::string name;       // Sound identifier (e.g., "leaves", "bgm")
        std::string filepath;   // Path to audio file
        float volume = 1.0f;    // Volume (0.0 to 1.0)
        bool preload = true;    // Load at startup?
        bool loop = false;      // Loop the sound?
    };

    /**
     * @brief Audio settings structure
     */
    struct AudioSettings {
        float masterVolume = 1.0f;
        float musicVolume = 1.0f;
        float sfxVolume = 1.0f;
    };

    /**
     * @brief Audio configuration loader
     *
     * Deserializes audio configuration from JSON and loads sounds into AudioSystem
     */
    class AudioLoader {
    public:
        /**
         * @brief Load audio configuration from JSON file
         * @param filepath Path to audio config JSON
         * @return true if successful
         */
        static bool LoadAudioConfig(const std::string& filepath, AudioSystem* audioSystem);

        /**
         * @brief Get loaded audio configurations
         */
        static const std::vector<AudioConfig>& GetLoadedConfigs() { return loadedConfigs; }

        /**
         * @brief Get audio settings
         */
        static const AudioSettings& GetSettings() { return settings; }

    private:
        /**
         * @brief Parse JSON into AudioConfig structures
         */
        static bool ParseAudioJSON(const nlohmann::json& json);

        /**
         * @brief Load all configured sounds into AudioSystem
         */
        static bool LoadSoundsIntoSystem(AudioSystem* audioSystem);

        static std::vector<AudioConfig> loadedConfigs;
        static AudioSettings settings;
    };

} // namespace Framework