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
#include "GlobalPauseManager.h"

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

        // M5 1105: Create Music group under master
        result = fmodSystem->createChannelGroup("Music", &musicGroup);
        CheckFMODError(result, "createChannelGroup(Music)");

        if (result == FMOD_OK && masterGroup && musicGroup)
        {
            result = masterGroup->addGroup(musicGroup);
            CheckFMODError(result, "masterGroup->addGroup(Music)");
        }


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
        DBG_SCOPE_SYS("Audio System", eng::debug::Subsystem::Audio);

        if (!fmodSystem) {
            return;
        }

        bool shouldPlay = false;
        bool isPaused = GlobalPause::IsPaused();
        bool hardPauseAudio = false;

        if (Framework::CORE) {
            shouldPlay = Framework::CORE->IsPlaying();

            bool isEditorMode = Framework::CORE->IsEditorMode();

            // Only hard-pause audio when paused inside Level Editor play mode.
            // Demo mode should keep audio running so Lua can reduce volume instead.
            hardPauseAudio = (!shouldPlay) || (isPaused && isEditorMode);
        }
        else {
            hardPauseAudio = true;
        }

        if (masterGroup) {
            masterGroup->setPaused(hardPauseAudio);
        }

        // In demo mode while paused, audio is still allowed to run
        // because PauseMenu.lua lowers the volume instead of fully stopping it.
        if (shouldPlay && !hardPauseAudio) {
            UpdateMusicFade(dt);
            UpdateAudioSources();
        }

        fmodSystem->update();
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

        /*if (Framework::CORE) {
            if (!Framework::CORE->IsPlaying() || Framework::CORE->IsEditorMode()) {
                return;
            }
        */

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
        musicChannel = nullptr;
        currentMusicName.clear();
        musicFadeActive = false;
        musicStopWhenFadeDone = false;
        pendingMusicName.clear();
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

    // Reload all runtime sounds from the configured audio manifest path.
    void AudioSystem::ReloadAudioLibrary() {
        std::cout << "[AudioSystem] Reloading from AudioConfig.json...\n";

        // 1. Cleanup old sounds
        StopAllSounds();
        for (auto& [name, sound] : sounds) {
            if (sound) sound->release();
        }
        sounds.clear();

        // 2. Read File
        const std::string audioConfigPath =
            ConfigReader::GetProjectPath("audio_config", "assets/JSON/AudioConfig.json");
        std::ifstream file(audioConfigPath);
        if (!file.is_open()) {
            std::cerr << "[AudioSystem] Failed to open config file: " << audioConfigPath << "\n";
            return;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string json = buffer.str();

        // --- PARSING LOGIC ---

        // 1. Locate the "sounds" array
        size_t soundsPos = json.find("\"sounds\"");
        size_t arrayStart = json.find("[", soundsPos);
        if (soundsPos == std::string::npos || arrayStart == std::string::npos) {
            std::cout << "[AudioSystem] No 'sounds' array found.\n";
            return;
        }

        // 2. Loop through objects inside the array
        size_t currentPos = arrayStart;
        int loadedCount = 0;

        while (true) {
            // Find start of next object
            size_t objStart = json.find("{", currentPos);
            size_t objEnd = json.find("}", objStart);

            // Stop if no more objects or we passed the end of the array
            // (A simple heuristic: if we find ']' before the next '{', we are done)
            size_t arrayClose = json.find("]", currentPos);
            if (objStart == std::string::npos || (arrayClose != std::string::npos && objStart > arrayClose)) {
                break;
            }

            // Extract the single object string: { "name": "...", "filepath": "..." }
            std::string entry = json.substr(objStart, objEnd - objStart + 1);

            // Parse "name"
            std::string nameVal, pathVal;

            size_t nKey = entry.find("\"name\"");
            if (nKey != std::string::npos) {
                size_t nColon = entry.find(":", nKey);
                size_t nQ1 = entry.find("\"", nColon);
                size_t nQ2 = entry.find("\"", nQ1 + 1);
                if (nQ1 != std::string::npos && nQ2 != std::string::npos) {
                    nameVal = entry.substr(nQ1 + 1, nQ2 - nQ1 - 1);
                }
            }

            // Parse "filepath"
            size_t pKey = entry.find("\"filepath\"");
            if (pKey != std::string::npos) {
                size_t pColon = entry.find(":", pKey);
                size_t pQ1 = entry.find("\"", pColon);
                size_t pQ2 = entry.find("\"", pQ1 + 1);
                if (pQ1 != std::string::npos && pQ2 != std::string::npos) {
                    pathVal = entry.substr(pQ1 + 1, pQ2 - pQ1 - 1);
                }
            }

            // Load if valid
            if (!nameVal.empty() && !pathVal.empty()) {
                if (LoadSound(pathVal, nameVal)) {
                    loadedCount++;
                }
            }

            // Move cursor past this object
            currentPos = objEnd + 1;
        }

        std::cout << "[AudioSystem] Reload Complete. Loaded " << loadedCount << " sounds.\n";
    }

    /**
         * @brief Plays background music with optional fade-in and queued track swapping
         * @param soundName Name of the music track to play
         * @param fadeInSec Duration in seconds to fade from 0 volume to full volume
         * @param loop If true, the music loops continuously
         *
         * This function is used for BGM playback instead of normal PlaySound().
         * If another music track is already playing, the requested track is stored
         * as pending music, the current track begins fading out, and the new track
         * starts automatically after the fade-out completes.
         *
         * Behavior:
         * - Plays music through the dedicated music channel group
         * - Starts playback paused, sets initial volume to 0, then fades in
         * - Supports immediate full-volume playback when fadeInSec <= 0
         * - Queues the next track if music is already playing
         *
         * This supports smoother scene and menu transitions for task 1105.
         */

    void AudioSystem::PlayMusic(const std::string& soundName, float fadeInSec, bool loop)
    {
        if (!fmodSystem || !musicGroup) return;

        bool isPlaying = false;
        if (musicChannel) {
            musicChannel->isPlaying(&isPlaying);
        }

        // Already playing this exact track: keep current playback and do nothing.
        if (isPlaying && currentMusicName == soundName) {
            return;
        }

        // If already playing, fade out then swap
        if (isPlaying) {
            pendingMusicName = soundName;
            pendingMusicLoop = loop;
            pendingMusicFadeIn = fadeInSec;
            StopMusic(0.5f);
            return;
        }

        auto it = sounds.find(soundName);
        if (it == sounds.end()) {
            std::cerr << "[Audio] Music not found: " << soundName << "\n";
            return;
        }

        FMOD::Channel* channel = nullptr;
        FMOD_RESULT result = fmodSystem->playSound(it->second, musicGroup, true, &channel);
        CheckFMODError(result, "playSound(Music)");
        if (result != FMOD_OK || !channel) return;

        channel->setMode(loop ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF);
        channel->setVolume(0.0f);
        channel->setPaused(false);

        musicChannel = channel;
        currentMusicName = soundName;

        // Fade in
        musicFadeActive = true;
        musicFadeStartVol = 0.0f;
        musicFadeTargetVol = 1.0f;
        musicFadeElapsed = 0.0f;
        musicFadeDuration = (fadeInSec > 0.0f) ? fadeInSec : 0.0f;
        musicStopWhenFadeDone = false;

        if (musicFadeDuration <= 0.0f) {
            musicChannel->setVolume(1.0f);
            musicFadeActive = false;
        }
    }

    /**
     * @brief Stops the current background music with optional fade-out
     * @param fadeOutSec Duration in seconds to fade the music volume to 0
     *
     * If fadeOutSec is greater than 0, the current music channel fades out
     * gradually and is stopped when the fade completes. If fadeOutSec is
     * less than or equal to 0, the music is stopped immediately.
     *
     * This function also works together with PlayMusic() to support
     * transition swapping: if a new track was queued while the old one
     * was fading out, the queued track will start after the stop finishes.
     */

    void AudioSystem::StopMusic(float fadeOutSec)
    {
        if (!musicChannel) return;

        bool isPlaying = false;
        musicChannel->isPlaying(&isPlaying);

        if (!isPlaying) {
            musicChannel = nullptr;
            currentMusicName.clear();
            return;
        }

        float currentVol = 1.0f;
        musicChannel->getVolume(&currentVol);

        if (fadeOutSec <= 0.0f) {
            musicChannel->stop();
            musicChannel = nullptr;
            currentMusicName.clear();
            musicFadeActive = false;
            musicStopWhenFadeDone = false;
            return;
        }

        // Fade out
        musicFadeActive = true;
        musicFadeStartVol = currentVol;
        musicFadeTargetVol = 0.0f;
        musicFadeElapsed = 0.0f;
        musicFadeDuration = fadeOutSec;
        musicStopWhenFadeDone = true;
    }

    /**
    * @brief Updates active music fade state each frame
    * @param dt Delta time since last frame
    *
    * This internal helper advances music fade-in and fade-out over time by
    * interpolating the current music channel volume from the start volume
    * to the target volume. When a fade-out completes, it stops the current
    * track and starts any queued pending music track if one exists.
    *
    * Called once per frame from Update().
    */

    void AudioSystem::UpdateMusicFade(float dt)
    {
        if (!musicFadeActive || !musicChannel) return;
        if (dt <= 0.0f) return;

        musicFadeElapsed += dt;

        if (musicFadeDuration <= 0.0f)
        {
            musicChannel->setVolume(musicFadeTargetVol);
            musicFadeActive = false;
            return;
        }

        float t = musicFadeElapsed / musicFadeDuration;
        if (t > 1.0f) t = 1.0f;

        float newVol = musicFadeStartVol + (musicFadeTargetVol - musicFadeStartVol) * t;
        musicChannel->setVolume(newVol);

        if (t >= 1.0f)
        {
            musicFadeActive = false;

            if (musicStopWhenFadeDone)
            {
                musicChannel->stop();
                musicChannel = nullptr;
                currentMusicName.clear();
                musicStopWhenFadeDone = false;

                // If a track was queued during fade-out, start it now
                if (!pendingMusicName.empty())
                {
                    std::string next = pendingMusicName;
                    bool nextLoop = pendingMusicLoop;
                    float nextFadeIn = pendingMusicFadeIn;

                    pendingMusicName.clear();

                    PlayMusic(next, nextFadeIn, nextLoop);
                }
            }
        }
    }

} // namespace Framework