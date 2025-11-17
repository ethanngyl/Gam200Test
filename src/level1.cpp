/*
===============================================================================
 File:          level1.cpp
 Author:        TAN WEI LEONG
 Email:         weileong.tan@digipen.edu
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
#include "Audio/AudioSystem.h"
#include "EntitySpawner.h"      
#include "PlayerManager.h"
#include "ImguiSystem.h"
#include "Component.h" 
extern Framework::CoreEngine* engine;
using Framework::Vector2D;
using Framework::ScriptComponent;

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

    auto* scriptSystem = engine->GetScriptSystem();

    if (!spawner || !scriptSystem) {
        LOG_ERROR("LEVEL1", "Missing spawner or script system!");
        return;
    }

    // Spawn a test entity
    auto testEntity = spawner->SpawnPlayer(Vector2D(0.0f, 0.0f));
    LOG_INFO("LEVEL1", "Spawned test entity: %u", testEntity.GetID());

    // Load script onto the entity
    scriptSystem->LoadScript(testEntity, "./assets/scripts/test_simple.lua");
    LOG_INFO("LEVEL1", "Loaded test script onto entity %u", testEntity.GetID());


    // ========================================================================
    // Spawn Player - Save the returned entity ID
    // ========================================================================
    auto playerEntity = spawner->SpawnPlayer(Vector2D(0.0f, -0.5f));
    auto audioSystem = engine->GetAudioSystem();

    auto enemyEntity = spawner->SpawnEnemy(Vector2D(1.0f, -0.5f));


    auto* playerController = engine->GetPlayerController();
    playerController->SetAudioSystem(audioSystem);

    if (playerController) {
        playerController->SetPlayerEntity(playerEntity);

        // ADD THIS LINE - Disable grid movement for level 1
        playerController->SetGridMovementEnabled(false);

        LOG_INFO("LEVEL1", "PlayerController configured with entity ID: %u", playerEntity);
    }

    if (playerController) {
        playerController->SetPlayerEntity(playerEntity);
        LOG_INFO("LEVEL1", "PlayerController configured with entity ID: %u", playerEntity);
    }
    else {
        LOG_ERROR("LEVEL1", "PlayerController system is null! Shooting disabled.");
    }

    //------------------------------------------------------------------
    //  SPRITE ANIMATION SETUP FOR PLAYER (config-driven)
    //------------------------------------------------------------------
    auto* em = engine->GetEntityManager();
    auto* gfx = engine->GetGraphicsSystem();

    if (em && gfx) {
        // ============================================================================
        // Load animation setting
        // ============================================================================
        //ConfigReader::LoadConfig("assets/anim_Bird.txt");
        //LOG_INFO("LEVEL1", "Loaded animation configuration from anim_Bird.txt");

        //// === Give player sprite animation ===
        //auto& anim = em->AddComponent<Framework::SpriteAnimation>(playerEntity);

        //// Sprite path
        //std::string spritePath = ConfigReader::GetString("sprite", "");
        //anim.spriteSheet = gfx->GetResourceManager().LoadTexture(spritePath);
        //Framework::Texture* tex = gfx->GetResourceManager().GetTexture(anim.spriteSheet);

        //// Sheet layout
        //anim.rows = ConfigReader::GetInt("rows", 1);
        //anim.columns = ConfigReader::GetInt("columns", 1);
        //anim.frameCount = ConfigReader::GetInt("frameCount", 1);
        //anim.frameTime = ConfigReader::GetFloat("frameTime", 0.0f);
        //anim.loop = ConfigReader::GetBool("loop", true);
        //anim.uvShrinkPx = ConfigReader::GetFloat("uvShrinkPx", 0.0f);

        //// Derived frame size
        //anim.frameWidth = tex->GetWidth() / anim.columns;
        //anim.frameHeight = tex->GetHeight() / anim.rows;
        //anim.playing = true;
        //anim.currentFrame = 0;

         // ============================================================================
        // Load animation setting
        // ============================================================================
        LOG_INFO("LEVEL1", "Loaded animation configuration from file");

        // ============================
        // 1. Add SpriteAnimation
        // ============================
        auto& anim = em->AddComponent<Framework::SpriteAnimation>(playerEntity);

        auto* animSys = engine->GetAnimationSystem();
        if (animSys)
        {
            animSys->LoadAnimationConfig("assets/spritesheet_config.txt");

            // default animation
            animSys->LoadAnimation(playerEntity, anim, gfx, animSys->animEntries[0].file);
        }

        // ============================
        // 2. Add Renderable
        // ============================
        auto& rend = em->AddComponent<Framework::Renderable>(playerEntity);
        rend.visible = true;
        rend.layer = 1;

        // ============================
        // 3. Add Transform
        // ============================
        auto& xform = em->AddComponent<Framework::Transform>(playerEntity);

        ConfigReader::LoadConfig("assets/valueloader.txt");
        LOG_INFO("LEVEL1", "Reloaded UI configuration");
    }

    graphics->SetFollowTarget(playerEntity);
    // ========================================================================
    // Tell the PlayerController who the player entity is
    // ========================================================================

    LOG_INFO("LEVEL1", "PlayerController configured with entity ID: %u", playerEntity);


    LOG_INFO("LEVEL1", "All entities spawned successfully");
}

void level1_Update()
{
    auto* animSys = engine->GetAnimationSystem();
    auto* input = engine->GetInputSystem();
    auto* em = engine->GetEntityManager();
    auto* gfx = engine->GetGraphicsSystem();
    auto player = engine->GetPlayerController()->GetPlayerEntity();

    if (animSys && em && gfx && player.IsValid())
    {
        if (em->HasComponent<Framework::SpriteAnimation>(player))
        {
            auto& anim = em->GetComponent<Framework::SpriteAnimation>(player);

            for (auto& entry : animSys->animEntries)
            {
                if (input->IsKeyPressed((Framework::KeyCode)entry.key))
                {
                    LOG_INFO("LEVEL1", ">>> Key pressed: %c, switching to %s",
                        entry.key, entry.file.c_str());
                    animSys->LoadAnimation(player, anim, gfx, entry.file);
                }
            }
        }
    }

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

    if (engine && engine->GetScriptSystem()) {
        auto* em = engine->GetEntityManager();
        auto entities = em->GetAllEntities();

        for (auto entity : entities) {
            if (em->HasComponent<ScriptComponent>(entity)) {
                engine->GetScriptSystem()->UnloadScript(entity);
            }
        }
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