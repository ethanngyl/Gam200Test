/*
===============================================================================
 File:          AudioSystem.cpp
 Author:        ETHAN NG
 Email:         n.ethanyongle@digipen.edu
 Date:          2025-10-31
 Contribution:  100%
 ------------------------------------------------------------------------------
  Audio System Implementation

 Overview:
    The AudioSystem class provides audio functioanlities for the game engine
    via use of the FMOD audio library and also is integrated with the current
    ECS system in place.

  Design notes:
     - Inherits from the engineSystem class
     - ECS Compatible
     - Currently only supports 2D audio
     - Multiple validation checks to prevent faulty/uninitialized entities from running
===============================================================================
*/
#include "AudioSystem.h"
#include "Component.h"
#include <fmod.hpp>
#include <fmod_errors.h>
#include <iostream>
#include "Precompiled.h"
#include "ECSEntityManager.h"

namespace Framework {

    /**
     * @brief To check for FMOD Errors 
     * @params result, Takes in the FMOD error code
     * @params message, Takes in a message, can be used to specify where exactly the error occurs
     * @return the type of fmod error
     *
     * Implementation details:
     * - This function is restricted to this file
     * - FMOD_Result result contains the error code
     * - By using FMOD_ErrorString we are able to retrieve the string description of the error code
     */
    static void CheckFMODError(FMOD_RESULT result, const char* message) {
        if (result != FMOD_OK) {
            std::cerr << "[FMOD Error] " << message << ": "
                << FMOD_ErrorString(result) << std::endl;
        }
    }
    /**
     * @brief Default constructor for AudioSystem
     * - Creates the AudioSystem instance and logs creation message
     */
    AudioSystem::AudioSystem() {
        std::cout << "[Audio] AudioSystem created\n";
    }
    
    /**
     * @brief Default destructor for AudioSystem
     * -  Logs destruction message. Does not clean up FMOD resources automatically.
     */
    AudioSystem::~AudioSystem() {
        std::cout << "[Audio] AudioSystem destroyed\n";
    }

    /**
     * @brief The function below initializes the audio system
     *
     * Implementation details:
     * - System_Create is used to create and initialize the main FMOD system objectto fmodSystem
     * - All other FMOD features are dependent on the core API created by System_Create
     * - The init function in the fmod library allows use to configure how fmod is run
     * - FMOD_INIT_NORMAL is used since to tell fmod to use the default settings since we are only using 2D audio
     * - getMasterChannelGroup is to allow us to retireve the handle to the Master Channel Group
     * - All sounds, channels and events are routed into this master group before the sound is sent to output devices
     */
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

    /**
     * @brief The function below updates the audio system
     * @params dt, Takes in delta time
     * Implementation details:
     * - update() is a function in the fmod library that needs to be called regularly
     * - Allows the fmod audio engine to run correctly
     * - Allows use to modify the audio in real-time and processess changes requested while the game is running
     */
    void AudioSystem::Update(float dt) {
        (void)dt; // silence unused variable warning

        DBG_SCOPE_SYS("Audio System", eng::debug::Subsystem::Audio);

        if (!fmodSystem) return;

        // Update FMOD
        fmodSystem->update();

        // Update all audio source components
        UpdateAudioSources();
    }

    /**
     * @brief The function below updates the audio sources in the game
     *
     * Implementation details:
     * - As with the other components, we must first retrive all entities and scan through if they possess the audio component before running the audio system
     * - FMOD Sound* retrieves the pointer associated with the name from the map entry
     * - FMOD Channel* is set to a nullptr to ensure its in a safe state before we use it
     * - playSound that takes in 4 parameters, the audio data, the channel group, which in this case since we don't have one it will default to the master channel, 
     the pause state(in this case we want it to start playing so we set it to be false), and the channel which is auto assigned by the function
     * - After playSound is verified, we can use functions in the library to set the properties of our audio(volume, pitch, mode)
     * - Currently audio looping is disabled since loop is initialized to be false within the struct, so FMOD_LOOP_OFF is used
     * - Play on start is disabled after the code successfully runs
     * - We are also able to modify existing sounds by switching channels to the channel that is running the audio
     */
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

    /**
     * @brief The function is currently unused and is inherited from the base class EngineSystem
     * @params message, Takes in the message to be passed
     */
    void AudioSystem::SendEngineMessage(Message* message) {
        (void)message;
    }

    /**
     * @brief The function below shuts down the audio engine
     *
     * Implementation details:
     * - The sounds map is looped through first
     * - .second holds the FMOD::Sound* pointer, this needs to be validated to prevent crashes in the event a nullptr somehow ended up in the map
     * - The release() function from the FMOD library is used to tell the system that we are finished with the sound data and allows FMOD to deallocate the memory that was used to store the sound's audio data
     * - The sound map is then cleared with .clear(), which removes all entires from the map
     * - The FMOD system is then denitialized and the instance destroyed
     * - close() stops audio mixer threads, shuts dwon the connection to the output devices and stops all currenlty playing sounds
     * - release() is then calle dagain to deallocate the memory used by FMOD
     * - fmodSystem is set to a null ptr to prevent dangling pointers.
     */
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

    /**
     * @brief The function below loads our sound
     * @param filepath - Path to the audio file (e.g., "assets/sounds/jump.wav")
     * @param name - Unique identifier for accessing this sound later
     * @return True if the createSound has successfully loaded, false if not
     * 
     * Implementation details:
     * - Verify that FMOD and if the sound is already loaded
     * - sound is set to a nullptr to ensure its safe state
     * - createSound is a function the fmod library used to load audio data from a file
     * - It takes in the filepath, the mode, optional info and where FMOD should store the pointer 
     * - FMOD_2D and FMOD_DEFAULT are the flags that configures the behaviour of our audio
     * - In this case, we want 2D audio(No positional calculations for now) and to use the default settings
     * - The default in this case basically merges FMOD_CREATESAMPLE and FMOD_LOOP_OFF 
     * - FMOD_CREATE_SAMPLE loads the whole sound into memory
     * - We don't have any optional info we ant to pass in so that parameter is set to a nullptr
     */
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

    /**
     * @brief The function below unloads our sound
     * @param name - Unique identifier of the sound to unload
     *
     * Implementation details:
     * - Searches the map for the name/key specified with find()
     * - Verifies that the key/name is found
     * - Validates the FMOD pointer is valid
     * - Uses fmod library's release() function to free the audio data from the memory
     * - erase() is then used to release the key-value pair from the map so it is not longer tracked by our engine
     */
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

    /**
     * @brief The function below plays our sound
     * @param soundName - Name of the sound to play (must be loaded first)
     * @param loop - If true, sound loops indefinitely; if false, plays once
     *
     * Implementation details:
     * - Verifies fmodsystem is intialized
     * - Searches the map for the name/key specified with find()
     * - returns if the key/name of the sound cannot be found
     * - FMOD Channel* is set to a nullptr to ensure its in a safe state before we use it
     * - playSound that takes in 4 parameters, the audio data, the channel group, which in this case since we don't have one it will default to the master channel,
     the pause state(in this case we want it to start playing so we set it to be false), and the channel which is auto assigned by the function  
     * - Depending on the boolean value of the loop parameter, we will either set the audio to be looped or just to be played as normal
     */
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

    /**
     * @brief The function below stops all sounds
     *
     * Implementation details:
     * - Verifies that the masterGroup exists
     * - stop() is a function from the fmod() library halts all sounds since all our audo event routes its audio signal through the master group
     */
    void AudioSystem::StopAllSounds() {
        if (masterGroup) {
            masterGroup->stop();
        }
    }

    /**
     * @brief The function sets the master volume of the entire application
     *
     * Implementation details:
     * - Verifies that the masterGroup exists
     * - setVolume() is a function from the fmod() to set the volume of the 
     */
    void AudioSystem::SetMasterVolume(float volume) {
        if (masterGroup) {
            masterGroup->setVolume(volume);
        }
    }

} // namespace Framework