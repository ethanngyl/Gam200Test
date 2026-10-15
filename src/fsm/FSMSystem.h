/**
===============================================================================
 File:          FSMSystem.h
 Author:        Padilla Carl Jameson Z.
 Email:         c.padilla@digipen.edu
 Date:          2026-01-26
 Contribution:  100%
 ------------------------------------------------------------------------------

 FINITE STATE MACHINE - Engine System

 Brief:
    System that manages the logic updates for all FSMComponents.
    It iterates through entities and ticks their active states every frame.

 Integration Guide:
    To enable this system, add the following to Core/Engine:

    1. Core.h
       class FSMSystem;                 (Forward declare)
       FSMSystem* fsmSystem;            (Member variable)

    2. Core.cpp (Initialize)
       fsmSystem = new FSMSystem();
       fsmSystem->SetEntityManager(entityManager);
       AddSystem(fsmSystem);

    3. Core.cpp (Cleanup)
       delete fsmSystem;


 Copyright (C) 2026 DigiPen Institute of Technology.
 Reproduction or disclosure of this file or its contents
 without the prior written consent of DigiPen Institute of
 Technology is prohibited.
===============================================================================
*/

#pragma once

#include "Interface.h"

namespace Framework {

    // Forward declarations
    class EntityManager;

    /**
     * @class FSMSystem
     * @brief Updates all entities that have FSMComponent
     *
     * This system iterates through all entities with FSMComponent
     * and calls Update() on their state machines each frame.
     */
    class FSMSystem : public EngineSystem {
    public:
        FSMSystem();
        ~FSMSystem();

        // === ENGINE INTERFACE ===
        void Initialize() override;
        void Update(float dt) override;
        void SendEngineMessage(Message* msg) override;

        // === DEPENDENCIES ===
        void SetEntityManager(EntityManager* em) { m_entityManager = em; }

    private:
        EntityManager* m_entityManager;
    };

} // namespace Framework