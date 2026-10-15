/**
===============================================================================
 File:           StateMachine.cpp
 Author:         Padilla carl Jameson Z.    
 Email:          c.padilla@digipen.edu
 Date:           2026-01-26
 Contribution:   100%
 ------------------------------------------------------------------------------

 FINITE STATE MACHINE - StateMachine Implementation

 Brief Notes:
   Implements state management, transitions, and update delegation.
   Uses your engine's logging and follows your coding conventions.

 Copyright (C) 2026 DigiPen Institute of Technology.
 Reproduction or disclosure of this file or its contents
 without the prior written consent of DigiPen Institute of
 Technology is prohibited.
===============================================================================
*/

#include "Precompiled.h"
#include "StateMachine.h"

namespace Framework {

    // =========================================================================
    // CONSTRUCTOR / DESTRUCTOR
    // =========================================================================

    StateMachine::StateMachine()
        : m_currentState(nullptr)
        , m_currentStateName("")
        , m_owner()
        , m_entityManager(nullptr)
        , m_debugEnabled(false)
    {
    }

    StateMachine::~StateMachine() {
        // Exit current state if running
        if (m_currentState) {
            m_currentState->Exit();
            m_currentState = nullptr;
        }
        // unique_ptr handles state cleanup automatically
    }

    // =========================================================================
    // STATE MANAGEMENT
    // =========================================================================

    void StateMachine::AddState(const std::string& name, std::unique_ptr<IState> state) {
        if (!state) {
            std::cout << "[FSM] Error: Cannot add null state '" << name << "'" << std::endl;
            return;
        }

        if (m_states.find(name) != m_states.end()) {
            std::cout << "[FSM] Warning: Replacing existing state '" << name << "'" << std::endl;
        }

        // Wire up the state with references it needs
        state->SetStateMachine(this);
        state->SetOwner(m_owner);
        state->SetEntityManager(m_entityManager);

        m_states[name] = std::move(state);

        if (m_debugEnabled) {
            std::cout << "[FSM] State added: " << name << std::endl;
        }
    }

    void StateMachine::Start(const std::string& name) {
        auto it = m_states.find(name);
        if (it == m_states.end()) {
            std::cout << "[FSM] Error: Cannot start - state '" << name << "' not found" << std::endl;
            return;
        }

        m_currentStateName = name;
        m_currentState = it->second.get();

        if (m_debugEnabled) {
            std::cout << "[FSM] Started with state: " << name << std::endl;
        }

        m_currentState->Enter();
    }

    void StateMachine::ChangeState(const std::string& name) {
        // Check if state exists
        auto it = m_states.find(name);
        if (it == m_states.end()) {
            std::cout << "[FSM] Error: State '" << name << "' not found" << std::endl;
            return;
        }

        // Don't transition to same state
        if (name == m_currentStateName) {
            return;
        }

        std::string previousState = m_currentStateName;

        // Exit current state
        if (m_currentState) {
            if (m_debugEnabled) {
                std::cout << "[FSM] Exiting: " << m_currentStateName << std::endl;
            }
            m_currentState->Exit();
        }

        // Switch to new state
        m_currentStateName = name;
        m_currentState = it->second.get();

        // Enter new state
        if (m_debugEnabled) {
            std::cout << "[FSM] Entering: " << m_currentStateName << std::endl;
            std::cout << "[FSM] Transition: " << previousState << " -> " << m_currentStateName << std::endl;
        }

        m_currentState->Enter();
    }

    // =========================================================================
    // UPDATE
    // =========================================================================

    void StateMachine::Update(float dt) {
        if (m_currentState) {
            m_currentState->Update(dt);
        }
    }

    // =========================================================================
    // QUERIES
    // =========================================================================

    std::string StateMachine::GetCurrentStateName() const {
        return m_currentStateName;
    }

    IState* StateMachine::GetCurrentState() const {
        return m_currentState;
    }

    bool StateMachine::HasState(const std::string& name) const {
        return m_states.find(name) != m_states.end();
    }

    bool StateMachine::IsRunning() const {
        return m_currentState != nullptr;
    }

    // =========================================================================
    // ENTITY BINDING
    // =========================================================================

    void StateMachine::SetOwner(Entity entity) {
        m_owner = entity;

        // Update all existing states
        for (auto& pair : m_states) {
            pair.second->SetOwner(entity);
        }
    }

    Entity StateMachine::GetOwner() const {
        return m_owner;
    }

    void StateMachine::SetEntityManager(EntityManager* em) {
        m_entityManager = em;

        // Update all existing states
        for (auto& pair : m_states) {
            pair.second->SetEntityManager(em);
        }
    }

    EntityManager* StateMachine::GetEntityManager() const {
        return m_entityManager;
    }

    // =========================================================================
    // DEBUG
    // =========================================================================

    void StateMachine::SetDebugEnabled(bool enabled) {
        m_debugEnabled = enabled;
    }

    bool StateMachine::IsDebugEnabled() const {
        return m_debugEnabled;
    }

} // namespace Framework