/**
===============================================================================
 File:          FSMComponent.h
 Author:        Padilla Carl Jameson Z.
 Email:         c.padilla@digipen.edu
 Date:          2026-01-26
 Contribution:  100%
 ------------------------------------------------------------------------------

 FINITE STATE MACHINE - ECS Component

 Brief:
    Gives an entity a "brain" by attaching a State Machine.
    Use this to manage specific behaviors (e.g., switching from Idle to Chase).

 Usage:
    1. Add FSM component to an entity
        entityManager->AddComponent<FSMComponent>(entity);

    2. Define states and start the machine
        auto& fsm = entityManager->GetComponent<FSMComponent>(entity);
        fsm.stateMachine.AddState("Idle", std::make_unique<EnemyIdleState>());
        fsm.stateMachine.AddState("Chase", std::make_unique<EnemyChaseState>());
    
    fsm.stateMachine.Start("Idle");


 Copyright (C) 2026 DigiPen Institute of Technology.
 Reproduction or disclosure of this file or its contents
 without the prior written consent of DigiPen Institute of
 Technology is prohibited.
===============================================================================
*/

#pragma once

#include "ECSComponent.h"
#include "StateMachine.h"

namespace Framework {

    /**
     * @struct FSMComponent
     * @brief ECS component that holds a StateMachine for an entity
     *
     * Attach this to any entity that needs state-based behavior.
     * The FSMSystem will automatically update all entities with this component.
     */
    struct FSMComponent : public Component<FSMComponent> {
        StateMachine stateMachine;

        FSMComponent() = default;
    };

} // namespace Framework