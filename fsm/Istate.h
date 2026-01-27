/**
===============================================================================
 File:           IState.h
 Author:
 Date:           2025-01-26
 Contribution:   100%
 ------------------------------------------------------------------------------

 FINITE STATE MACHINE - State Interface

 Design notes:
   This is the base interface for all FSM states. All concrete states must
   inherit from this class and implement the pure virtual methods.

 Usage:
   1. Create a class that inherits from IState
   2. Implement Enter(), Update(), Exit(), and GetName()
   3. Add the state to a StateMachine using AddState()

 Example:
   class PlayerIdleState : public Framework::IState {
   public:
       void Enter() override { }
       void Update(float dt) override { }
       void Exit() override { }
       std::string GetName() const override { return "Idle"; }
   };

===============================================================================
*/

#pragma once

#include "ECSEntity.h"
#include <string>

namespace Framework {

    // Forward declaration
    class StateMachine;
    class EntityManager;

    /**
     * @class IState
     * @brief Abstract base class for all FSM states
     *
     * Inherit from this class to create custom states.
     * Each state handles its own logic and can request transitions
     * through the owning StateMachine.
     */
    class IState {
    public:
        virtual ~IState() = default;

        // =================================================================
        // PURE VIRTUAL METHODS - Must be implemented by derived classes
        // =================================================================

        /**
         * @brief Called once when entering this state
         * Use for: starting animations, resetting timers, playing sounds
         */
        virtual void Enter() = 0;

        /**
         * @brief Called every frame while this state is active
         * @param dt Delta time in seconds
         * Use for: game logic, checking transition conditions
         */
        virtual void Update(float dt) = 0;

        /**
         * @brief Called once when exiting this state
         * Use for: cleanup, stopping animations/sounds
         */
        virtual void Exit() = 0;

        /**
         * @brief Returns the name of this state for debugging
         * @return State name as string
         */
        virtual std::string GetName() const = 0;

        // =================================================================
        // STATE MACHINE ACCESS
        // =================================================================

        /**
         * @brief Sets the owning state machine (called by StateMachine::AddState)
         * @param machine Pointer to the owning StateMachine
         */
        void SetStateMachine(StateMachine* machine) { m_stateMachine = machine; }

        /**
         * @brief Gets the owning state machine
         * @return Pointer to the StateMachine (use for transitions)
         */
        StateMachine* GetStateMachine() const { return m_stateMachine; }

        // =================================================================
        // ENTITY ACCESS (for entity-bound states)
        // =================================================================

        /**
         * @brief Sets the owner entity (called by StateMachine::SetOwner)
         * @param entity The entity this state belongs to
         */
        void SetOwner(Entity entity) { m_owner = entity; }

        /**
         * @brief Gets the owner entity
         * @return The entity this state belongs to
         */
        Entity GetOwner() const { return m_owner; }

        /**
         * @brief Sets the entity manager reference
         * @param em Pointer to the EntityManager
         */
        void SetEntityManager(EntityManager* em) { m_entityManager = em; }

        /**
         * @brief Gets the entity manager
         * @return Pointer to EntityManager (use for component access)
         */
        EntityManager* GetEntityManager() const { return m_entityManager; }

    protected:
        StateMachine* m_stateMachine = nullptr;
        Entity m_owner;
        EntityManager* m_entityManager = nullptr;
    };

} // namespace Framework