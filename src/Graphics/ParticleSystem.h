#pragma once
#include "Precompiled.h"
#include "ECSEntityManager.h"
#include "RenderComponents.h"
#include "Component.h"

namespace Framework {

    class EntityManager;
    class GraphicsSystemV2;

    // IMPORTANT: replace MeshRenderer with whatever your render component type is
    // In many engines it’s MeshRenderer or Renderable.
    struct Transform;
    struct ParticleEmitter;

    class ParticleSystem : public EngineSystem {
    public:
        void SetEntityManager(EntityManager* em) { entityManager = em; }
        void SetGraphicsSystem(GraphicsSystemV2* gs) { graphics = gs; }

        virtual void Initialize() override;
        virtual void Update(float dt) override;
        virtual void SendEngineMessage(Message*) override {}

    private:
        struct Preset
        {
            std::string emitMode = "rate"; // "rate" or "burst"
            float emitRate = 0.0f;
            int burstCount = 0;
            int maxParticles = 100;

            float lifeMin = 1.0f, lifeMax = 2.0f;
            float speedMin = 10.0f, speedMax = 40.0f;
            float spreadDeg = 360.0f;

            float startSize = 0.2f;
            float endSize = 0.0f;
            glm::vec4 startColor{ 1,1,1,1 };
            glm::vec4 endColor{ 1,1,1,0 };
        };

        struct ParticleState
        {
            bool alive = false;
            float age = 0.0f;
            float lifetime = 1.0f;
            Vector2D vel{};
        };

        struct EmitterRuntime
        {
            bool poolBuilt = false;
            float emitAcc = 0.0f;
            bool burstFired = false;
            std::vector<Entity> pool;
            std::vector<ParticleState> state;
        };

    private:
        bool LoadPresets(const std::string& path);
        const Preset* GetPreset(const std::string& name) const;

        void EnsurePool(Entity emitter, const Preset& p);
        int FindDead(const EmitterRuntime& rt) const;

        void SpawnRate(Entity emitter, ParticleEmitter& em, const Preset& p, float dt, const Transform& et);
        void SpawnBurst(Entity emitter, ParticleEmitter& em, const Preset& p, const Transform& et);
        void UpdateParticles(Entity emitter, const Preset& p, float dt);

    private:
        EntityManager* entityManager = nullptr;
        GraphicsSystemV2* graphics = nullptr;

        std::unordered_map<std::string, Preset> presets;
        std::unordered_map<uint32_t, EmitterRuntime> runtime; // key = emitter entity id
    };

}
