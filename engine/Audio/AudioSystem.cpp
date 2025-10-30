#include "AudioSystem.h"
#include "Component.h"
#include <fmod.hpp>
#include <fmod_errors.h>
#include <iostream>
#include "Precompiled.h"
#include "ECSEntityManager.h"

namespace Framework {

    // Helper to check FMOD errors
    static void CheckFMODError(FMOD_RESULT result, const char* message) {
        if (result != FMOD_OK) {
            std::cerr << "[FMOD Error] " << message << ": "
                << FMOD_ErrorString(result) << std::endl;
        }
    }

    AudioSystem::AudioSystem() {
        std::cout << "[Audio] AudioSystem created\n";
    }

    AudioSystem::~AudioSystem() {
        std::cout << "[Audio] AudioSystem destroyed\n";
    }

    void AudioSystem::Initialize() {
        std::cout << "[Audio] Initializing FMOD...\n";

        // Create FMOD system
        FMOD_RESULT result = FMOD::System_Create(&fmodSystem);
        CheckFMODError(result, "System_Create");

        if (result != FMOD_OK) {
            std::cerr << "[Audio] Failed to create FMOD system\n";
            return;
        }

        // Initialize FMOD (2D)
        //Max channels is the maximum number of different sounds/audio that can be playing simultaneously
        //FMOD_INIT_NORMAL tells fmod to use the default/standard settings
        result = fmodSystem->init(maxChannels, FMOD_INIT_NORMAL, nullptr);

        //Checks for any errors with fmod after initializing
        CheckFMODError(result, "init");

        if (result != FMOD_OK) {
            std::cerr << "[Audio] Failed to initialize FMOD\n";
            return;
        }

        // Get master channel group
        result = fmodSystem->getMasterChannelGroup(&masterGroup);
        CheckFMODError(result, "getMasterChannelGroup");

        std::cout << "[Audio] FMOD initialized successfully\n";
    }

    void AudioSystem::Update(float dt) {
        if (!fmodSystem) return;

        // Update FMOD
        fmodSystem->update();

        // Update all audio source components
        UpdateAudioSources();
    }

    void AudioSystem::UpdateAudioSources() {
        if (!entityManager) return;

        for (Entity entity : entityManager->GetAllEntities()) {
            if (!entityManager->HasComponent<AudioSource>(entity)) continue;

            auto& audioSource = entityManager->GetComponent<AudioSource>(entity);

            // Play on start
            if (audioSource.playOnStart && !audioSource.isPlaying) {
                if (!audioSource.soundName.empty()) {
                    // Find sound
                    auto it = sounds.find(audioSource.soundName);
                    if (it != sounds.end()) {
                        FMOD::Sound* sound = it->second;
                        FMOD::Channel* channel = nullptr;

                        // Play sound
                        FMOD_RESULT result = fmodSystem->playSound(
                            sound,
                            nullptr,
                            false,
                            &channel
                        );

                        if (result == FMOD_OK && channel) {
                            // Set properties
                            channel->setVolume(audioSource.volume);
                            channel->setPitch(audioSource.pitch);
                            channel->setMode(audioSource.loop ?
                                FMOD_LOOP_NORMAL : FMOD_LOOP_OFF);

                            audioSource.fmodChannel = channel;
                            audioSource.isPlaying = true;
                        }
                    }
                }
                audioSource.playOnStart = false;
            }

            // Update existing sounds
            if (audioSource.isPlaying && audioSource.fmodChannel) {
                FMOD::Channel* channel = (FMOD::Channel*)audioSource.fmodChannel;

                bool isPlaying = false;
                channel->isPlaying(&isPlaying);
                audioSource.isPlaying = isPlaying;

                if (isPlaying) {
                    // Update volume/pitch if changed
                    channel->setVolume(audioSource.volume);
                    channel->setPitch(audioSource.pitch);
                }
            }
        }
    }

    void AudioSystem::SendEngineMessage(Message* message) {
        (void)message;
    }
    void AudioSystem::Shutdown() {
        std::cout << "[Audio] Shutting down FMOD...\n";

        // Unload all sounds
        for (auto& pair : sounds) {
            if (pair.second) {
                pair.second->release();
            }
        }
        sounds.clear();

        // Release FMOD
        if (fmodSystem) {
            fmodSystem->close();
            fmodSystem->release();
            fmodSystem = nullptr;
        }

        std::cout << "[Audio] FMOD shutdown complete\n";
    }

    bool AudioSystem::LoadSound(const std::string& filepath,
        const std::string& name) {
        if (!fmodSystem) {
            std::cerr << "[Audio] FMOD not initialized\n";
            return false;
        }

        // Check if already loaded
        if (sounds.find(name) != sounds.end()) {
            std::cout << "[Audio] Sound '" << name << "' already loaded\n";
            return true;
        }

        FMOD::Sound* sound = nullptr;

        // Load as 2D sound
        FMOD_RESULT result = fmodSystem->createSound(
            filepath.c_str(),
            FMOD_2D | FMOD_DEFAULT,  // 2D only
            nullptr,
            &sound
        );

        if (result != FMOD_OK) {
            CheckFMODError(result, ("Loading sound: " + filepath).c_str());
            return false;
        }

        sounds[name] = sound;
        std::cout << "[Audio] Loaded sound: " << name << " (" << filepath << ")\n";
        return true;
    }

    void AudioSystem::UnloadSound(const std::string& name) {
        auto it = sounds.find(name);
        if (it != sounds.end()) {
            if (it->second) {
                it->second->release();
            }
            sounds.erase(it);
            std::cout << "[Audio] Unloaded sound: " << name << "\n";
        }
    }

    void AudioSystem::PlaySound(const std::string& soundName, bool loop) {
        if (!fmodSystem) return;

        auto it = sounds.find(soundName);
        if (it == sounds.end()) {
            std::cerr << "[Audio] Sound not found: " << soundName << "\n";
            return;
        }

        FMOD::Channel* channel = nullptr;
        FMOD_RESULT result = fmodSystem->playSound(
            it->second,
            nullptr,
            false,
            &channel
        );

        if (result == FMOD_OK && channel) {
            channel->setMode(loop ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF);
        }
    }

    void AudioSystem::StopAllSounds() {
        if (masterGroup) {
            masterGroup->stop();
        }
    }

    void AudioSystem::SetMasterVolume(float volume) {
        if (masterGroup) {
            masterGroup->setVolume(volume);
        }
    }

} // namespace Framework