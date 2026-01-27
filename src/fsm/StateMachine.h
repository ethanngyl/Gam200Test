/**
===============================================================================
 File:           StateMachine.h
 Author:
 Date:           2025-01-26
 Contribution:   100%
 ------------------------------------------------------------------------------

 FINITE STATE MACHINE - StateMachine Class

 Design notes:
   The StateMachine is the core controller that manages states and transitions.
   It maintains the current active state and delegates Update() calls to it.
   States can request transitions via GetStateMachine()->ChangeState().

 Usage:
   StateMachine fsm;
   fsm.AddState("Idle", std::make_unique<IdleState>());
   fsm.AddState("Walk", std::make_unique<WalkState>());
   fsm.Start("Idle");

   // In game loop:
   fsm.Update(dt);

   // To change state (from anywhere):
   fsm.ChangeState("Walk");

===============================================================================
*/

#pragma once

#include "IState.h"
#include "ECSEntity.h"
#include <string>
#include <memory>
#include <unordered_map>

namespace Framework {

    // Forward declarations
    class EntityManager;

    /**
     * @class StateMachine
     * @brief Manages states and handles transitions between them
     *
     * The StateMachine owns all states and is responsible for:
     * - Storing registered states
     * - Maintaining the current active state
     * - Handling state transitions (calling Exit/Enter)
     * - Delegating Update calls to the active state
     */
    class StateMachine {
    public:
        StateMachine();
        ~StateMachine();

        // =================================================================
        // STATE MANAGEMENT
        // =================================================================

        /**
         * @brief Adds a state to the state machine
         * @param name Unique name identifier for the state
         * @param state The state object (ownership transferred)
         */
        void AddState(const std::string& name, std::unique_ptr<IState> state);

        /**
         * @brief Starts the state machine with an initial state
         * @param name Name of the state to start with
         *
         * Must be called after adding states and before Update().
         * Calls Enter() on the initial state.
         */
        void Start(const std::string& name);

        /**
         * @brief Transitions to a different state
         * @param name Name of the state to transition to
         *
         * Calls Exit() on current state, then Enter() on new state.
         * Can be called from outside or from within a state.
         */
        void ChangeState(const std::string& name);

        // =================================================================
        // UPDATE
        // =================================================================

        /**
         * @brief Updates the current state
         * @param dt Delta time in seconds
         *
         * Call this every frame. Delegates to current state's Update().
         */
        void Update(float dt);

        // =================================================================
        // QUERIES
        // =================================================================

        /**
         * @brief Gets the name of the current state
         * @return Current state name, empty string if not started
         */
        std::string GetCurrentStateName() const;

        /**
         * @brief Gets the current state object
         * @return Pointer to current state, nullptr if not started
         */
        IState* GetCurrentState() const;

        /**
         * @brief Checks if a state exists
         * @param name State name to check
         * @return true if state exists
         */
        bool HasState(const std::string& name) const;

        /**
         * @brief Checks if the state machine is running
         * @return true if Start() has been called
         */
        bool IsRunning() const;

        // =================================================================
        // ENTITY BINDING (for ECS integration)
        // =================================================================

        /**
         * @brief Sets the owner entity for this FSM
         * @param entity The entity that owns this state machine
         *
         * All states will have access to this entity via GetOwner().
         */
        void SetOwner(Entity entity);

        /**
         * @brief Gets the owner entity
         * @return The owner entity
         */
        Entity GetOwner() const;

        /**
         * @brief Sets the entity manager reference
         * @param em Pointer to EntityManager
         *
         * All states will have access to this via GetEntityManager().
         */
        void SetEntityManager(EntityManager* em);

        /**
         * @brief Gets the entity manager
         * @return Pointer to EntityManager
         */
        EntityManager* GetEntityManager() const;

        // =================================================================
        // DEBUG
        // =================================================================

        /**
         * @brief Enables or disables debug logging
         * @param enabled true to print state changes to console
         */
        void SetDebugEnabled(bool enabled);

        /**
         * @brief Checks if debug logging is enabled
         * @return true if debug is enabled
         */
        bool IsDebugEnabled() const;

    private:
        std::unordered_map<std::string, std::unique_ptr<IState>> m_states;
        IState* m_currentState;
        std::string m_currentStateName;
        Entity m_owner;
        EntityManager* m_entityManager;
        bool m_debugEnabled;
    };

} // namespace Framework