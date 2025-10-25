// Level1.cpp
#include "Precompiled.h"        
#include "Core.h"               
#include "EntitySpawner.h"      
#include "Vector2D.h"    
#include "GSM/GameStateList.h"
#include "GSM/GameStateManager.h"

extern Framework::CoreEngine* engine;
using Framework::Vector2D;
using Framework::EntitySpawner;

void level1_Load()
{
    LOG_INFO("LEVEL1", "=== Level1 Load ===");
}

void level1_Initialize()
{
    LOG_INFO("LEVEL1", "=== Level1 Initialize ===");

    if (!engine) {
        LOG_ERROR("LEVEL1", "Engine is null!");
        return;
    }

    EntitySpawner* spawner = engine->GetSpawner();
    if (!spawner) {
        LOG_ERROR("LEVEL1", "Spawner is null!");
        return;
    }

    // Generate players
    spawner->SpawnPlayer(Vector2D(0.0f, -0.5f));

    // Generate enemy waves
    spawner->SpawnEnemyWave(5, 0.8f);

    // Generate some obstacles
    spawner->SpawnObstacle(Vector2D(-0.5f, 0.0f), Vector2D(0.3f, 0.3f));
    spawner->SpawnObstacle(Vector2D(0.5f, 0.0f), Vector2D(0.3f, 0.3f));

    // Generate a circular pattern
    spawner->SpawnCircle("circle", 8, Vector2D(0.0f, 0.0f), 0.6f);

    LOG_INFO("LEVEL1", "All entities spawned successfully");
}

void level1_Update()
{
    if (engine && engine->GetInputSystem() &&
        engine->GetInputSystem()->IsKeyPressed(Framework::KEY_SPACE))
    {
        LOG_INFO("TEST", "Space pressed!");
        next = mainMenu;
    }
    
}

void level1_Draw()
{
    // Only responsible for rendering related matters
    // Do not create entities here
    LOG_INFO("MENU", "=== level1 Draw ===");
}

void level1_Free()
{
    LOG_INFO("LEVEL1", "=== Level1 Free ===");

    // Clean All Entities
    if (engine && engine->GetEntityManager()) {
        auto entities = engine->GetEntityManager()->GetAllEntities();
        for (auto entity : entities) {
            engine->GetEntityManager()->DestroyEntity(entity);
        }
    }
}

void level1_Unload()
{
    LOG_INFO("LEVEL1", "=== Level1 Unload ===");
}