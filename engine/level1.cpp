/*
===============================================================================
 File:          level1.cpp
 Author:        GE YONGQI
 Email:         yongqi.ge@digipen.edu
 Date:          2025-10-31
 Contribution:  100%
 ------------------------------------------------------------------------------
  Level 1 gameplay state (implementation)

  Design notes:
     - Spawns the player, enemies, and sets up gameplay entities
     - Links the PlayerControllerSystem to the spawned player entity
     - Uses the CoreEngine’s GraphicsSystemV2 for camera tracking

  Highlights:
     - Integrates with UISystem and ImGuiSystem for in-game control
     - Cleans up all entities on state exit
     - Fully logged for debugging and system verification
===============================================================================
*/

#include "Precompiled.h"   

#include "EntitySpawner.h"      
#include "PlayerManager.h"
#include "ConfigReader/ConfigReader.h"
#include "ImguiSystem.h"


extern Framework::CoreEngine* engine;
using Framework::Vector2D;
using Framework::EntitySpawner;
using Framework::PlayerControllerSystem;
using Framework::GraphicsSystemV2;

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
    if (engine && engine->GetImGuiSystem()) {
        engine->GetImGuiSystem()->Enable();
        LOG_INFO("MENU", "ImGui disabled in menu");
    }
    engine->SetPlaying(true);

	GraphicsSystemV2* graphics = engine->GetGraphicsSystem();
    if (!graphics) {
        LOG_ERROR("LEVEL1", "Graphics system is null!");
        return;
	}

    EntitySpawner* spawner = engine->GetSpawner();
    if (!spawner) {
        LOG_ERROR("LEVEL1", "Spawner is null!");
        return;
    }

    PlayerControllerSystem* playerControll = engine->GetPlayerController();
    if (!playerControll) {
        LOG_ERROR("LEVEL1", "playerControll is null!");
        return;
    }

    // ========================================================================
    // Spawn Player - Save the returned entity ID
    // ========================================================================
    Framework::Entity playerEntity = spawner->SpawnPlayer(Vector2D(0.0f, -0.5f));

    //------------------------------------------------------------------
    //  SPRITE ANIMATION SETUP FOR PLAYER (config-driven)
    //------------------------------------------------------------------
    auto* em = engine->GetEntityManager();
    auto* gfx = engine->GetGraphicsSystem();

    if (em && gfx) {
        // ============================================================================
        // Load animation setting
        // ============================================================================
        ConfigReader::LoadConfig("assets/anim_Bird.txt");

        // === Give player sprite animation ===
        auto& anim = em->AddComponent<Framework::SpriteAnimation>(playerEntity);

        // Sprite path
        std::string spritePath = ConfigReader::GetString("sprite", "");
        anim.spriteSheet = gfx->GetResourceManager().LoadTexture(spritePath);
        Framework::Texture* tex = gfx->GetResourceManager().GetTexture(anim.spriteSheet);

        // Sheet layout
        anim.rows = ConfigReader::GetInt("rows", 1);
        anim.columns = ConfigReader::GetInt("columns", 1);
        anim.frameCount = ConfigReader::GetInt("frameCount", 1);
        anim.frameTime = ConfigReader::GetFloat("frameTime", 0.0f);
        anim.loop = ConfigReader::GetBool("loop", true);
        anim.uvShrinkPx = ConfigReader::GetFloat("uvShrinkPx", 0.0f);

        // Derived frame size
        anim.frameWidth = tex->GetWidth() / anim.columns;
        anim.frameHeight = tex->GetHeight() / anim.rows;
        anim.playing = true;
        anim.currentFrame = 0;

        // Renderable
        auto& rend = em->AddComponent<Framework::Renderable>(playerEntity);
        rend.visible = true;
        rend.layer = 1;

        // Transform
        auto& xform = em->AddComponent<Framework::Transform>(playerEntity);
        xform.upperLimit = 2.0f;
        xform.lowerLimit = 0.5f;
    }

	graphics->SetFollowTarget(playerEntity);
    // ========================================================================
    // Tell the PlayerController who the player entity is
    // ========================================================================
    playerControll->SetPlayerEntity(playerEntity);

    // Optional: Configure shooting parameters
    playerControll->SetShootCooldown(0.2f);     
    playerControll->SetProjectileSpeed(0.5f);

    LOG_INFO("LEVEL1", "PlayerController configured with entity ID: %u", playerEntity);

    // Generate enemy waves
    spawner->SpawnEnemyWave(5, 0.8f);

    LOG_INFO("LEVEL1", "All entities spawned successfully");
}

void level1_Update()
{
    if (engine && engine->GetInputSystem() &&
        engine->GetInputSystem()->IsKeyPressed(Framework::KEY_2))
    {
        next = mainMenu;
    }
}

void level1_Draw()
{
    // Only responsible for rendering related matters
    // Do not create entities here
    //LOG_INFO("MENU", "=== level1 Draw ===");
}

void level1_Free()
{
    LOG_INFO("LEVEL1", "=== Level1 Free ===");


    if (engine && engine->GetImGuiSystem()) {
        engine->GetImGuiSystem()->Disable();
        LOG_INFO("MENU", "ImGui disabled in menu");
    }

    if (engine) {
        engine->SetPlaying(false);

        if (engine->GetGraphicsSystem()) {
            engine->GetGraphicsSystem()->ClearFollowTarget();
        }

        LOG_INFO("LEVEL1", "Switched back to EDITOR mode");
    }

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