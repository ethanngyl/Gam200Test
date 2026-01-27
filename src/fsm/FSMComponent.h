/**
===============================================================================
 File:           FSMComponent.h
 Author:
 Date:           2025-01-26
 Contribution:   100%
 ------------------------------------------------------------------------------

 FINITE STATE MACHINE - ECS Component

 Design notes:
   This component attaches a StateMachine to an entity, allowing entity
   behavior to be controlled by states. Follows your ECS component pattern.

 Usage:
   // Add FSM component to entity
   Entity enemy = entityManager->CreateEntity();
   entityManager->AddComponent<FSMComponent>(enemy);

   // Get component and set up states
   auto& fsm = entityManager->GetComponent<FSMComponent>(enemy);
   fsm.stateMachine.AddState("Idle", std::make_unique<EnemyIdleState>());
   fsm.stateMachine.AddState("Chase", std::make_unique<EnemyChaseState>());
   fsm.stateMachine.Start("Idle");

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