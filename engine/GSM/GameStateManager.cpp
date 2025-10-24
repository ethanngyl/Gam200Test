/**
===============================================================================
 File:           GameStateManager.cpp
 Description:    Implementation of simple game state manager
===============================================================================
 */

#include "Precompiled.h"
#include "GameStateManager.h"
#include "MainMenuSystem.h"
#include "EntitySpawner.h"
#include <fstream>
#include <sstream>

namespace Framework {

    GameStateManager::GameStateManager()
        : m_entityManager(nullptr)
        , m_mainMenuSystem(nullptr)
        , m_entitySpawner(nullptr)
        , m_currentState(GameState::None)
        , m_initialState(GameState::MainMenu)
    {
    }

    void SetupGame(Framework::EntitySpawner* spawner)
    {
        LOG_INFO("CORE", "=== Setting up game ===");

        // Note: Player is spawned separately so we can get its Entity ID

        // Spawn some initial enemies
        spawner->SpawnEnemyWave(5, 0.6f);

        // Spawn walls
        spawner->SpawnObstacle(Framework::Vector2D(-1.8f, 0.0f), Framework::Vector2D(0.1f, 2.0f));
        spawner->SpawnObstacle(Framework::Vector2D(1.8f, 0.0f), Framework::Vector2D(0.1f, 2.0f));

        LOG_INFO("CORE", "Game setup complete!");
    }
    void GameStateManager::Initialize()
    {
        LOG_INFO("CORE", "[GSM] Initializing Game State Manager...");

        // Load config file
        if (!LoadConfig()) {
            LOG_ERROR("ERROR", "[GSM] Failed to load config, using default: MainMenu");
            m_initialState = GameState::MainMenu;
        }

        // Enter initial state
        ChangeState(m_initialState);
        
        LOG_INFO("CORE", "[GSM] Initialization complete");
    }

    void GameStateManager::Update(float dt)
    {
        (void)dt;
        // GSM doesn't need per-frame updates
        // State transitions are triggered by messages/events
    }

    void GameStateManager::SendEngineMessage(Message* msg)
    {
        // GSM doesn't need to handle messages
        // State changes are triggered via direct ChangeState() calls
        (void)msg;
    }

    bool GameStateManager::LoadConfig()
    {
        std::ifstream file(m_configPath);
        if (!file.is_open()) {
            LOG_ERROR("ERROR", "[GSM] Cannot open config file: " , m_configPath);
            return false;
        }

        std::string line;
        while (std::getline(file, line)) {
            // Skip empty lines and comments
            if (line.empty() || line[0] == '#') continue;

            // Parse key-value pairs
            std::istringstream iss(line);
            std::string key, value;
            
            if (std::getline(iss, key, '=') && std::getline(iss, value)) {
                // Trim whitespace
                key.erase(0, key.find_first_not_of(" \t"));
                key.erase(key.find_last_not_of(" \t") + 1);
                value.erase(0, value.find_first_not_of(" \t"));
                value.erase(value.find_last_not_of(" \t") + 1);

                if (key == "initial_state") {
                    m_initialState = ParseState(value);
                    LOG_INFO("CORE", "[GSM] Config: initial_state = " , value);
                }
            }
        }

        file.close();
        return true;
    }

    GameState GameStateManager::ParseState(const std::string& stateName)
    {
        if (stateName == "MainMenu") return GameState::MainMenu;
        if (stateName == "Level1") return GameState::Level1;
        
        LOG_ERROR("ERROR", "[GSM] Unknown state: " , stateName , ", defaulting to MainMenu");
        return GameState::MainMenu;
    }

    void GameStateManager::ChangeState(GameState newState)
    {
        if (newState == m_currentState) {
            LOG_INFO("CORE", "[GSM] Already in requested state");
            return;
        }

        LOG_INFO("CORE", "[GSM] Changing state...");

        // Exit current state
        ExitState(m_currentState);

        // Update state
        m_currentState = newState;

        // Enter new state
        EnterState(newState);

        LOG_INFO("CORE", "[GSM] State change complete");
    }

    void GameStateManager::ExitState(GameState state)
    {
        switch (state) {
            case GameState::MainMenu:
                LOG_INFO("CORE", "[GSM] Exiting MainMenu");
                if (m_mainMenuSystem) {
                    m_mainMenuSystem->SetVisible(false);

                }
                CleanupStateEntities();
                break;

            case GameState::Level1:
                LOG_INFO("CORE", "[GSM] Exiting Level1");
                CleanupStateEntities();
                break;

            case GameState::None:
                // No cleanup needed for None state
                break;
        }
    }

    void GameStateManager::EnterState(GameState state)
    {
        switch (state) {
            case GameState::MainMenu:
                LOG_INFO("CORE", "[GSM] Entering MainMenu");
                if (m_mainMenuSystem) {
                    m_mainMenuSystem->SetVisible(true);

                }
                break;

            case GameState::Level1:
                LOG_INFO("CORE", "[GSM] Entering Level1");

                if (!m_entitySpawner || !m_entityManager) {
                    LOG_ERROR("ERROR", "[GSM] EntitySpawner or EntityManager not set!");
                    break;
                }

                // Spawn initial enemies
                m_entitySpawner->SpawnEnemyWave(5, 0.6f);

                // Spawn walls
                Entity wall1 = m_entitySpawner->SpawnObstacle(
                    Vector2D(-1.8f, 0.0f),
                    Vector2D(0.1f, 2.0f)
                );
                Entity wall2 = m_entitySpawner->SpawnObstacle(
                    Vector2D(1.8f, 0.0f),
                    Vector2D(0.1f, 2.0f)
                );

                m_stateEntities.push_back(wall1);
                m_stateEntities.push_back(wall2);

                LOG_INFO("CORE", "[GSM] Level1 setup complete");
                break;
        }
    }

    void GameStateManager::CleanupStateEntities()
    {
        if (!m_entityManager) return;

        LOG_INFO("CORE", "[GSM] Cleaning up state entities...");

        // Destroy all entities spawned in this state
        for (Entity entity : m_stateEntities) {
            if (entity.IsValid()) {
                m_entityManager->DestroyEntity(entity);
            }
        }

        m_stateEntities.clear();
        
        LOG_INFO("CORE", "[GSM] Cleanup complete");
    }

} // namespace Framework