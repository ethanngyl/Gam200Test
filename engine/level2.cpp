/*
===============================================================================
 File:          level2.cpp
 Author:        GE YONGQI
 Email:         yongqi.ge@digipen.edu
 Date:          2025-10-31
 Contribution:  100%
 ------------------------------------------------------------------------------
  Level 2 gameplay state (implementation)

  Highlights:
     - Integrates with UISystem and ImGuiSystem for in-game control
     - Cleans up all entities on state exit
     - Fully logged for debugging and system verification
===============================================================================
*/

#include "Precompiled.h"   

#include "EntitySpawner.h"      
#include "PlayerManager.h"
#include "ImguiSystem.h"


extern Framework::CoreEngine* engine;


void level2_Load()
{
    LOG_INFO("LEVEL2", "=== Level2 Load ===");
}

void level2_Initialize()
{
    LOG_INFO("LEVEL2", "=== Level2 Initialize ===");


    if (!engine) {
        LOG_ERROR("LEVEL2", "Engine is null!");
        return;
    }
    if (engine && engine->GetImGuiSystem()) {
        engine->GetImGuiSystem()->Enable();
        LOG_INFO("MENU", "ImGui disabled in menu");
    }
    engine->SetPlaying(true);

    LOG_INFO("MENU", "Menu in EDITOR mode");

    auto graphics = engine->GetGraphicsSystem();
    if (graphics) {
        // Reset camera for menu
        graphics->SetCameraPosition(glm::vec3(0.0f, 0.0f, 0.0f));
        graphics->SetCameraZoom(1.0f);
    }
}

void level2_Update()
{
    if (engine && engine->GetInputSystem() &&
        engine->GetInputSystem()->IsKeyPressed(Framework::KEY_5))
    {
        next = mainMenu;
    }
}

void level2_Draw()
{
    // Only responsible for rendering related matters
    // Do not create entities here
    //LOG_INFO("MENU", "=== level2 Draw ===");
}

void level2_Free()
{
    LOG_INFO("LEVEL2", "=== Level2 Free ===");


    if (engine && engine->GetImGuiSystem()) {
        engine->GetImGuiSystem()->Disable();
        LOG_INFO("MENU", "ImGui disabled in menu");
    }

    if (engine) {
        engine->SetPlaying(false);

        if (engine->GetGraphicsSystem()) {
            engine->GetGraphicsSystem()->ClearFollowTarget();
        }

        LOG_INFO("LEVEL2", "Switched back to EDITOR mode");
    }

    // Clean All Entities
    if (engine && engine->GetEntityManager()) {
        auto entities = engine->GetEntityManager()->GetAllEntities();
        for (auto entity : entities) {
            engine->GetEntityManager()->DestroyEntity(entity);
        }
    }
}

void level2_Unload()
{
    LOG_INFO("LEVEL2", "=== Level2 Unload ===");
}