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
#include "Pathfinding.h"


extern Framework::CoreEngine* engine;
using Framework::Vector2D;


void level2_Load()
{
    ConfigReader::LoadConfig("assets/valueloader.txt");
}

void level2_Initialize()
{

    if (!engine) {
        LOG_ERROR("LEVEL2", "Engine is null!");
        return;
    }
    if (engine && engine->GetImGuiSystem()) {
        engine->GetImGuiSystem()->Enable();
    }
    engine->SetPlaying(false);


    auto graphics = engine->GetGraphicsSystem();
    if (graphics) {
        // Reset camera for menu
        graphics->SetCameraPosition(glm::vec3(0.0f, 0.0f, 0.0f));
        graphics->SetCameraZoom(1.0f);
    }

    auto spawner = engine->GetSpawner();

    auto playerController = engine->GetPlayerController();
    auto playerEntity = spawner->SpawnPlayer(Vector2D(0.0f, -0.5f));

    playerController->SetPlayerEntity(playerEntity);
    playerController->SetEntitySpawner(spawner);

    if (engine->GetImGuiSystem()) {
        engine->GetImGuiSystem()->SetPlayerEntity(playerEntity);
        LOG_INFO("LEVEL2", "Player entity set on ImGui system");
    }

    if (engine->GetEntityManager()) {
        Framework::PathfindingSystem::SpawnEnemyFurthestFromPlayer(
            playerEntity,
            engine->GetEntityManager(),
            spawner
        );
        LOG_INFO("LEVEL2", "Enemy spawned (will pathfind to player)");
    }
}

void level2_Update()
{
    if (engine && engine->GetInputSystem() &&
        engine->GetInputSystem()->IsKeyPressed(Framework::KEY_5))
    {
        next = mainMenu;
    }

    if (engine && engine->GetInputSystem() &&
        engine->GetInputSystem()->IsKeyPressed(Framework::KEY_3))
    {
        next = LEVEL_3;
    }
}

void level2_Draw()
{
    if (!engine) return;

    auto graphics = engine->GetGraphicsSystem();
    if (!graphics) return;

    // ========================================================================
    // Read UI text settings from configuration file
    // ========================================================================
    
    // Font
    std::string fontLarge = ConfigReader::GetString("lv2_ui_font_large", "Sans48");

    // Text content
    std::string textScaling = ConfigReader::GetString("lv2_text_info", "This level is for Level editor testing");

    // Location
    float textX = ConfigReader::GetFloat("lv2_ui_text_info_x", 500.0f);
    float texty = ConfigReader::GetFloat("lv2_ui_text_info_y", 350.0f);

    // Scaling and color
    float textScale = ConfigReader::GetFloat("lv2_ui_text_scale", 1.0f);
    float colorR = ConfigReader::GetFloat("lv2_ui_text_color_r", 1.0f);
    float colorG = ConfigReader::GetFloat("lv2_ui_text_color_g", 1.0f);
    float colorB = ConfigReader::GetFloat("lv2_ui_text_color_b", 1.0f);

    // ========================================================================
    // Render UI text (using values ​​from the configuration file)
    // ========================================================================
    graphics->DrawText4(fontLarge, textScaling, textX, texty, textScale, glm::vec3(colorR, colorG, colorB));
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