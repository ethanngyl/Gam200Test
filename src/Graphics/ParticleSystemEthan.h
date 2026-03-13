/*
===============================================================================
Author:      Ethan Ng
Email:       n.ethanyongle@digipen.edu
File:        ParticleSystemEthan.cpp
-------------------------------------------------------------------------------
Brief:
ParticleSystem manages the lifecycle of all particles in the engine.
It iterates entities with ParticleEmitter + Transform, spawns new particles
based on emission rate, updates positions/colors/sizes over lifetime,
and removes dead particles. Rendering is handled by GraphicsSystemV2.
===============================================================================
*/
#pragma once
#include "Interface.h"

namespace Framework {

    class EntityManager;
    class GraphicsSystemV2;

    class ParticleSystemEthan : public EngineSystem {
    public:
        ParticleSystemEthan();
        ~ParticleSystemEthan() override;

        void Initialize() override;
        void Update(float dt) override;
        void SendEngineMessage(Message* message) override;

        void SetEntityManager(EntityManager* em) { entityManager = em; }
        void SetGraphicsSystem(GraphicsSystemV2* gs) { graphicsSystem = gs; }

    private:
        void SpawnParticles(struct ParticleEmitter& emitter, const struct Transform& transform, float dt);
        void UpdateParticles(struct ParticleEmitter& emitter, float dt);

        EntityManager* entityManager = nullptr;
        GraphicsSystemV2* graphicsSystem = nullptr;
    };

} // namespace Framework
