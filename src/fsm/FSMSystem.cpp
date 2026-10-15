/**
===============================================================================
 File:          FSMSystem.cpp
 Author:        Padilla Carl Jameson Z.
 Email:         c.padilla@digipen.edu
 Date:          2026-01-26
 Contribution:  100%
 ------------------------------------------------------------------------------

 FINITE STATE MACHINE - Engine System Implementation

 Brief notes:
   Updates all entities with FSMComponent each frame.
   Follows your engine's system implementation pattern.

 Copyright (C) 2026 DigiPen Institute of Technology.
 Reproduction or disclosure of this file or its contents
 without the prior written consent of DigiPen Institute of
 Technology is prohibited.
===============================================================================
*/

#include "Precompiled.h"
#include "FSMSystem.h"
#include "FSMComponent.h"
#include "ECSEntityManager.h"

namespace Framework {

    // =========================================================================
    // CONSTRUCTOR / DESTRUCTOR
    // =========================================================================

    FSMSystem::FSMSystem()
        : m_entityManager(nullptr)
    {
    }

    FSMSystem::~FSMSystem() {
    }

    // =========================================================================
    // ENGINE INTERFACE
    // =========================================================================

    void FSMSystem::Initialize() {
        std::cout << "[FSMSystem] Initialized" << std::endl;
    }

    void FSMSystem::Update(float dt) {
        if (!m_entityManager) return;

        // Iterate all entities
        auto entities = m_entityManager->GetAllEntities();

        for (auto& entity : entities) {
            // Check if entity has FSMComponent
            if (m_entityManager->HasComponent<FSMComponent>(entity)) {
                auto& fsmComp = m_entityManager->GetComponent<FSMComponent>(entity);

                // Ensure state machine has owner and entity manager set
                if (!fsmComp.stateMachine.GetOwner().IsValid()) {
                    fsmComp.stateMachine.SetOwner(entity);
                }
                if (!fsmComp.stateMachine.GetEntityManager()) {
                    fsmComp.stateMachine.SetEntityManager(m_entityManager);
                }

                // Update the state machine
                fsmComp.stateMachine.Update(dt);
            }
        }
    }

    void FSMSystem::SendEngineMessage(Message* msg) {
        (void)msg;
    }

} // namespace Framework