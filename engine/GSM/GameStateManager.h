/**
===============================================================================
 File:           GameStateManager.h
 Description:    Simple game state manager that reads from config file
                 Manages transitions between MainMenu and Level states
===============================================================================
 */

#pragma once
#include "Precompiled.h"
#include <string>
#include <unordered_map>

namespace Framework {

    // Forward declarations
    class EntitySpawner;
    class MainMenuSystem;

    /**
     * @brief Game state enumeration
     */
    enum class GameState {
        None,
        MainMenu,
        Level1,
        // Add more states here as needed
    };

    /**
     * @brief Simple Game State Manager
     * 
     * Reads initial state from config file and manages state transitions.
     * Controls which systems are active based on current state.
     */
    class GameStateManager : public EngineSystem {
    public:
        GameStateManager();
        ~GameStateManager() = default;

        // EngineSystem interface
        void Initialize() override;
        void Update(float dt) override;
        void SendEngineMessage(Message* msg) override;

        // Setup
        void SetEntityManager(EntityManager* em) { m_entityManager = em; }
        void SetMainMenuSystem(MainMenuSystem* menu) { m_mainMenuSystem = menu; }
        void SetEntitySpawner(EntitySpawner* spawner) { m_entitySpawner = spawner; }

        // State management
        void ChangeState(GameState newState);
        GameState GetCurrentState() const { return m_currentState; }

        // Config file path (default: "game_config.txt")
        void SetConfigPath(const std::string& path) { m_configPath = path; }

    private:
        // Load config file
        bool LoadConfig();
        
        // Parse state name from string
        GameState ParseState(const std::string& stateName);
        
        // State lifecycle
        void ExitState(GameState state);
        void EnterState(GameState state);

        // Clean up entities for a state
        void CleanupStateEntities();

    private:
        EntityManager* m_entityManager = nullptr;
        MainMenuSystem* m_mainMenuSystem = nullptr;
        EntitySpawner* m_entitySpawner = nullptr;

        GameState m_currentState = GameState::None;
        GameState m_initialState = GameState::MainMenu;
        
        std::string m_configPath = "game_config.txt";
        
        // Track entities spawned in current state for cleanup
        std::vector<Entity> m_stateEntities;
    };

} // namespace Framework
