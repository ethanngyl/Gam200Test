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
     - Uses the CoreEngine GraphicsSystemV2 for camera tracking
     - Loads multiple config files for different purposes
===============================================================================
*/

#include "Precompiled.h"   

#include "EntitySpawner.h"      
#include "PlayerManager.h"
#include "ImguiSystem.h"
#include "../Graphics/GraphicsSystemV2.h"


extern Framework::CoreEngine* engine;
using Framework::Vector2D;


void level1_Load()
{
    LOG_INFO("LEVEL1", "=== Level1 Load ===");

    ConfigReader::LoadConfig("assets/valueloader.txt");
    LOG_INFO("LEVEL1", "Loaded UI configuration from valueloader.txt");
}

void level1_Initialize()
{
    LOG_INFO("LEVEL1", "=== Level1 Initialize ===");


    if (!engine) {
        LOG_ERROR("LEVEL1", "Engine is null!");
        return;
    }
    if (engine && engine->GetImGuiSystem()) {
        engine->GetImGuiSystem()->Disable();
        LOG_INFO("MENU", "ImGui disabled in menu");
    }
    engine->SetPlaying(true);

    auto graphics = engine->GetGraphicsSystem();
    if (!graphics) {
        LOG_ERROR("LEVEL1", "Graphics system is null!");
        return;
    }

    auto spawner = engine->GetSpawner();
    if (!spawner) {
        LOG_ERROR("LEVEL1", "Spawner is null!");
        return;
    }

    auto playerControll = engine->GetPlayerController();
    if (!playerControll) {
        LOG_ERROR("LEVEL1", "playerControll is null!");
        return;
    }

    // ========================================================================
    // Spawn Player - Save the returned entity ID
    // ========================================================================
    auto playerEntity = spawner->SpawnPlayer(Vector2D(0.0f, -0.5f));

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
        LOG_INFO("LEVEL1", "Loaded animation configuration from anim_Bird.txt");

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
        auto& rend = em->GetComponent<Framework::Renderable>(playerEntity);
        rend.visible = true;
        rend.layer = 1;


        // Transform
        auto& xform = em->GetComponent<Framework::Transform>(playerEntity);
        xform.upperLimit = ConfigReader::GetFloat("upperLimit", 0.0f);
        xform.lowerLimit = ConfigReader::GetFloat("lowerLimit", 0.0f);

        ConfigReader::LoadConfig("assets/valueloader.txt");
        LOG_INFO("LEVEL1", "Reloaded UI configuration");
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


    LOG_INFO("LEVEL1", "All entities spawned successfully");
}

void level1_Update()
{
    if (engine && engine->GetInputSystem() &&
        engine->GetInputSystem()->IsKeyPressed(Framework::KEY_5))
    {
        next = mainMenu;
    }
    else if (engine && engine->GetInputSystem() &&
        engine->GetInputSystem()->IsKeyPressed(Framework::KEY_6))
    {
        next = LEVEL_2;
    }
}

void level1_Draw()
{
    if (!engine) return;

    auto graphics = engine->GetGraphicsSystem();
    if (!graphics) return;

    // ========================================================================
    // Read UI text settings from configuration file
    // ========================================================================

    // Font
    std::string fontLarge = ConfigReader::GetString("lv1_ui_font_large", "Sans48");

    // Text content
    std::string textScaling = ConfigReader::GetString("lv1_text_scaling", "Scaling Button: 3 & 4");
    std::string textRotation = ConfigReader::GetString("lv1_text_rotation", "Rotation Button: 7 & 8");
    std::string textMovement = ConfigReader::GetString("lv1_text_movement", "Character Movement: W A S D");
    std::string textMenu = ConfigReader::GetString("lv1_text_menu", "Back to Menu: 5");
    std::string textNextLevel = ConfigReader::GetString("lv1_text_next_level", "Next Level: 6");

    // Location
    float textX = ConfigReader::GetFloat("lv1_ui_text_x", 50.0f);
    float textScalingY = ConfigReader::GetFloat("lv1_ui_text_scaling_y", 650.0f);
    float textRotationY = ConfigReader::GetFloat("lv1_ui_text_rotation_y", 550.0f);
    float textMovementY = ConfigReader::GetFloat("lv1_ui_text_movement_y", 450.0f);
    float textMenuY = ConfigReader::GetFloat("lv1_ui_text_menu_y", 350.0f);
    float textNextLevelY = ConfigReader::GetFloat("lv1_ui_text_next_level_y", 250.0f);

    // Scaling and color
    float textScale = ConfigReader::GetFloat("lv1_ui_text_scale", 1.0f);
    float colorR = ConfigReader::GetFloat("lv1_ui_text_color_r", 1.0f);
    float colorG = ConfigReader::GetFloat("lv1_ui_text_color_g", 1.0f);
    float colorB = ConfigReader::GetFloat("lv1_ui_text_color_b", 1.0f);
    glm::vec3 textColor(colorR, colorG, colorB);

    // ========================================================================
    // Render UI text (using values from the configuration file)
    // ========================================================================

    graphics->DrawText4(fontLarge, textScaling,
        textX, textScalingY, textScale, textColor);

    graphics->DrawText4(fontLarge, textRotation,
        textX, textRotationY, textScale, textColor);

    graphics->DrawText4(fontLarge, textMovement,
        textX, textMovementY, textScale, textColor);

    graphics->DrawText4(fontLarge, textMenu,
        textX, textMenuY, textScale, textColor);

    graphics->DrawText4(fontLarge, textNextLevel,
        textX, textNextLevelY, textScale, textColor);
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