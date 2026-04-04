/*
===============================================================================
 File:          AudioSystem.h
 Author:        ETHAN NG
 Email:         n.ethanyongle@digipen.edu
 Date:          2025-10-31
 Contribution:  100%
 ------------------------------------------------------------------------------
 Audio System Header

 Overview:
    The AudioSystem class provides audio functioanlities for the game engine
    via use of the FMOD audio library and also is integrated with the current
    ECS system in place.

 Key Features:
    - 2D audio playback (no positional audio calculations)
    - Sound loading and caching system
    - Entity-Component-System (ECS) integration
    - Master volume control via channel groups
    - Per-sound volume, pitch, and loop controls
    - Support for up to 32 simultaneous audio channels


===============================================================================
*/

#pragma once
#include "ECSEntityManager.h"
#include <unordered_map>
#include "Precompiled.h"

// Forward declarations for FMOD types
namespace FMOD {
    class System;
    class Sound;
    class Channel;
    class ChannelGroup;
}

namespace Framework {

    /**
     * @class AudioSystem
     * @brief Core audio management system integrating FMOD with the game engine
     *
     * The AudioSystem handles all audio-related functionality including:
     * - Initializing and managing the FMOD audio engine
     * - Loading and caching sound files
     * - Playing sounds directly or through entity components
     * - Controlling master volume and individual sound properties
     * - Updating audio state each frame
     *
     * Usage Pattern:
     * @code
     *   AudioSystem audio;
     *   audio.Initialize();                              // Init FMOD
     *   audio.LoadSound("assets/sfx.wav", "jump");      // Load sound
     *   audio.PlaySound("jump", false);                 // Play once
     *   audio.SetMasterVolume(0.8f);                    // Set volume
     *   audio.Update(deltaTime);                        // Call each frame
     *   audio.Shutdown();                               // Cleanup
     * @endcode
     */
    class AudioSystem : public EngineSystem {
    public:
        /**
         * @brief Default constructor
         *
         * Creates the AudioSystem instance and logs creation message.
         * Does not initialize FMOD - call Initialize() explicitly.
         */
        AudioSystem();

        /**
         * @brief Destructor
         *
         * Logs destruction message. Does not clean up FMOD resources
         * automatically - call Shutdown() before destruction.
         */
        ~AudioSystem();

        /**
         * @brief Initializes the FMOD audio system
         * @override EngineSystem::Initialize
         *
         * Performs the following operations:
         * 1. Creates the main FMOD System object via System_Create()
         * 2. Initializes FMOD with default 2D audio settings (FMOD_INIT_NORMAL)
         * 3. Sets up the master channel group for global volume control
         * 4. Configures max simultaneous channels (default: 32)
         *
         * All audio features depend on successful initialization of this
         * core FMOD system. The function uses FMOD_INIT_NORMAL since the
         * system only handles 2D audio without spatial calculations.
         *
         * @note Must be called before any other audio operations
         * @note Logs initialization progress and any errors to console
         */
        void Initialize() override;

        /**
         * @brief Updates the audio system each frame
         * @override EngineSystem::Update
         * @param dt Delta time since last frame (currently unused)
         *
         * Performs two critical operations:
         * 1. Updates FMOD engine state (processes streaming, callbacks, etc.)
         * 2. Updates all AudioSource components attached to entities
         *
         * Entity audio sources are processed to:
         * - Trigger playOnStart sounds
         * - Update volume/pitch for playing sounds
         * - Track playing state of each audio source
         *
         * @note Should be called once per frame from the main game loop
         */
        void Update(float dt) override;

        /**
         * @brief Message handling interface (currently unused)
         * @override EngineSystem::SendEngineMessage
         * @param message Message pointer to process
         *
         * Inherited from EngineSystem base class for future message-based
         * communication between engine systems. Currently not in use.
         */
        void SendEngineMessage(Message* message) override;

        /**
         * @brief Loads an audio file and caches it with a unique name
         * @param filepath - Path to the audio file (e.g., "assets/sounds/jump.wav")
         * @param name - Unique identifier for accessing this sound later
         * @return true if sound loaded successfully, false otherwise
         *
         * Loading Process:
         * 1. Verifies FMOD system is initialized
         * 2. Checks if sound is already loaded (prevents duplicates)
         * 3. Uses FMOD's createSound() to load audio data into memory
         * 4. Caches the sound pointer in the sounds map
         *
         * Audio Configuration:
         * - FMOD_2D: No positional audio calculations
         * - FMOD_DEFAULT: Combines FMOD_CREATESAMPLE + FMOD_LOOP_OFF
         *   - FMOD_CREATESAMPLE: Loads entire file into memory
         *   - FMOD_LOOP_OFF: Does not loop by default
         *
         * Example:
         * @code
         *   audio.LoadSound("assets/music/bgm.mp3", "background_music");
         *   audio.LoadSound("assets/sfx/jump.wav", "jump_sound");
         * @endcode
         */
        bool LoadSound(const std::string& filepath, const std::string& name);

        /**
         * @brief Unloads a sound from memory and removes it from cache
         * @param name - Unique identifier of the sound to unload
         *
         * Unloading Process:
         * 1. Searches the sound cache for the specified name
         * 2. Validates the FMOD sound pointer
         * 3. Calls FMOD's release() to free audio data from memory
         * 4. Removes the entry from the sounds map
         *
         * Example:
         * @code
         *   audio.UnloadSound("background_music");
         * @endcode
         */
        void UnloadSound(const std::string& name);

        /**
         * @brief Plays a loaded sound directly (no entity required)
         * @param soundName - Name of the sound to play (must be loaded first)
         * @param loop If true, sound loops indefinitely; if false, plays once
         *
         * Playback Process:
         * 1. Verifies FMOD system is initialized
         * 2. Searches for sound in cache by name
         * 3. Calls FMOD's playSound() with automatic channel assignment
         * 4. Sets loop mode based on parameter
         *
         * Channel Behavior:
         * - Channel group: nullptr (defaults to master channel group)
         * - Pause state: false (starts playing immediately)
         * - Channel: Auto-assigned by FMOD from available channels
         *
         * Example:
         * @code
         *   audio.PlaySound("jump_sound", false);        // Play once
         *   audio.PlaySound("background_music", true);   // Loop forever
         * @endcode
         */
        void PlaySound(const std::string& soundName, bool loop = false);

        /**
         * @brief Immediately stops all currently playing sounds
         *
         * Stops all audio by halting the master channel group. Since all
         * audio signals route through the master group before output, this
         * effectively silences all sounds instantly.
         *
         * Affects:
         * - All entity-based audio sources
         * - All directly played sounds
         * - Background music and sound effects
         *
         * Use Cases:
         * - Pausing the game
         * - Scene transitions
         * - Emergency audio cutoff
         *
         * Example:
         * @code
         *   audio.StopAllSounds();  // Instant silence
         * @endcode
         */
        void StopAllSounds();

        // M5 1105: BGM fade support
        void PlayMusic(const std::string& soundName, float fadeInSec = 0.5f, bool loop = true);
        void StopMusic(float fadeOutSec = 0.5f);

        /**
         * @brief Sets the global master volume for all audio
         * @param volume Volume level from 0.0 (silent) to 1.0 (full volume)
         *
         * The master volume affects all audio output by controlling the
         * master channel group volume. All sounds, channels, and audio
         * events route through this master group, so this provides true
         * global volume control.
         *
         * Volume Ranges:
         * - 0.0: Complete silence
         * - 0.5: Half volume
         * - 1.0: Full volume (no attenuation)
         * - >1.0: Amplification (may cause clipping)
         *
         *
         * Example:
         * @code
         *   audio.SetMasterVolume(0.8f);   // 80% volume
         *   audio.SetMasterVolume(0.0f);   // Mute all audio
         * @endcode
         */
        void SetMasterVolume(float volume);  // 0.0 to 1.0

        void SetMusicVolume(float volume);   // controls musicGroup only
        float GetMusicVolume() const;

        void SetSfxVolume(float volume);     // controls sfxGroup only
        float GetSfxVolume() const;

        /**
         * @brief Shuts down the audio system and releases all resources
         *
         * Shutdown Sequence:
         * 1. Iterates through all cached sounds
         * 2. Validates each sound pointer
         * 3. Calls FMOD's release() on each sound to free audio data
         * 4. Clears the sounds map
         * 5. Closes FMOD system (stops mixer threads, halts playback)
         * 6. Releases FMOD system instance
         * 7. Sets fmodSystem to nullptr to prevent dangling pointer
         *
         * Resource Cleanup:
         * - Deallocates all sound audio data from memory
         * - Shuts down connection to output devices
         * - Stops all audio mixer threads
         * - Halts all currently playing sounds
         *
         * Example:
         * @code
         *   audio.Shutdown();  // Clean shutdown
         * @endcode
         */
        void Shutdown();

        /**
         * @brief Sets the entity manager for ECS integration
         * @param em Pointer to the EntityManager instance
         *
         * The entity manager is required for the audio system to:
         * - Query entities with AudioSource components
         * - Update entity-based audio sources each frame
         * - Integrate with the ECS architecture
         *
         * @note Must be called before Update() processes entity audio sources
         * @note Does not take ownership (non-owning pointer)
         */
        void SetEntityManager(EntityManager* em) { entityManager = em; };
        void ReloadAudioLibrary();
    private:
        /**
         * @brief Updates all AudioSource components attached to entities
         *
         * This internal function is called every frame by Update() and:
         *
         * For Each Entity with AudioSource Component:
         * 1. Checks playOnStart flag for initial playback
         * 2. Looks up sound in cache by soundName
         * 3. Calls FMOD's playSound() if starting
         * 4. Sets volume, pitch, and loop mode from component
         * 5. Stores FMOD channel pointer in component
         * 6. Disables playOnStart after first play
         *
         * For Already Playing Sounds:
         * 1. Validates channel is still playing
         * 2. Updates isPlaying state
         * 3. Applies real-time volume/pitch changes
         *
         * Playback Parameters:
         * - Volume: Controlled by AudioSource.volume (0.0 to 1.0)
         * - Pitch: Controlled by AudioSource.pitch (affects playback speed)
         * - Loop: FMOD_LOOP_NORMAL if true, FMOD_LOOP_OFF if false
         *
         */
        void UpdateAudioSources();

        // M5 1105: called from Update(dt)
        void UpdateMusicFade(float dt);

        // Pointer to entity manager for ECS integration (non-owning)
        EntityManager* entityManager = nullptr;

        // Core FMOD system object - foundation for all audio operations
        FMOD::System* fmodSystem = nullptr;

        // Master channel group - all audio routes through this before output
        FMOD::ChannelGroup* masterGroup = nullptr;

        // M5 1105: Dedicated BGM routing + fade state
        FMOD::ChannelGroup* musicGroup = nullptr;
        FMOD::ChannelGroup* sfxGroup   = nullptr;
        FMOD::Channel* musicChannel = nullptr;
        std::string currentMusicName;

        bool musicFadeActive = false;
        float musicFadeStartVol = 1.0f;
        float musicFadeTargetVol = 1.0f;
        float musicFadeElapsed = 0.0f;
        float musicFadeDuration = 0.0f;

        bool musicStopWhenFadeDone = false;

        std::string pendingMusicName;
        bool pendingMusicLoop = true;
        float pendingMusicFadeIn = 0.5f;


        // Sound cache mapping unique names to FMOD sound objects
        // Key: Unique sound name, Value: FMOD sound pointer
        std::unordered_map<std::string, FMOD::Sound*> sounds;



        // Maximum number of sounds that can play simultaneously (default: 32)
        // If exceeded, FMOD will steal the oldest/quietest channel
        int maxChannels = 32;
    };

} // namespace Framework