#pragma once
#include "ECSEntityManager.h"
#include <unordered_map>
#include "Precompiled.h"
namespace FMOD {
    class System;
    class Sound;
    class Channel;
    class ChannelGroup;
}

namespace Framework {

    class AudioSystem : public EngineSystem {
    public:
        AudioSystem();
        ~AudioSystem();
        void Initialize() override;
        void Update(float dt) override;
        void SendEngineMessage(Message* message) override;

        // Load/unload sounds
        bool LoadSound(const std::string& filepath, const std::string& name);
        void UnloadSound(const std::string& name);

        // Play sounds directly (no entity needed)
        void PlaySound(const std::string& soundName, bool loop = false);
        void StopAllSounds();

        // Master volume control
        void SetMasterVolume(float volume);  // 0.0 to 1.0
        void Shutdown();

        void SetEntityManager(EntityManager* em) { entityManager = em; };
    private:
        void UpdateAudioSources();

        EntityManager* entityManager = nullptr;

        // FMOD
        FMOD::System* fmodSystem = nullptr;
        FMOD::ChannelGroup* masterGroup = nullptr;

        // Sound cache
        std::unordered_map<std::string, FMOD::Sound*> sounds;

        int maxChannels = 32;
    };

} // namespace Framework