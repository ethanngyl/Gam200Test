#include "Precompiled.h"
#include "AudioLoader.h"
#include "Audio/AudioSystem.h"
#include <fstream>

namespace Framework {

    // Static member initialization
    std::vector<AudioConfig> AudioLoader::loadedConfigs;
    AudioSettings AudioLoader::settings;

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
                LOG_INFO("AudioLoader", "✓ Successfully loaded: '%s'", config.name.c_str());
            }
            else {
                failCount++;
                LOG_ERROR("AudioLoader", "✗ Failed to load: '%s'", config.name.c_str());
            }
        }

        LOG_INFO("AudioLoader", "Audio loading complete: %d succeeded, %d failed",
            successCount, failCount);

        return failCount == 0;
    }

    bool AudioLoader::SetMasterVolume(float volume, const std::string& filepath) {
        // Clamp volume to valid range
        settings.masterVolume = std::max(0.0f, std::min(1.0f, volume));

        // Save to JSON file
        return SaveAudioConfig(filepath);
    }

    bool AudioLoader::SaveAudioConfig(const std::string& filepath) {
        try {
            // Read existing JSON file
            std::ifstream inFile(filepath);
            if (!inFile.is_open()) {
                LOG_ERROR("AudioLoader", "Failed to open audio config file for reading: %s", filepath.c_str());
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
            std::ofstream outFile(filepath);
            if (!outFile.is_open()) {
                LOG_ERROR("AudioLoader", "Failed to open audio config file for writing: %s", filepath.c_str());
                return false;
            }

            outFile << jsonData.dump(2);  // 2-space indentation
            outFile.close();

            LOG_INFO("AudioLoader", "Audio config saved to: %s (Master Volume: %.2f)",
                filepath.c_str(), settings.masterVolume);
            return true;
        }
        catch (const std::exception& e) {
            LOG_ERROR("AudioLoader", "Error saving audio config: %s", e.what());
            return false;
        }
    }

} // namespace Framework