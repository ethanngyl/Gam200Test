/*
===============================================================================
 File:          level1.cpp
 Author:        TAN WEI LEONG
 Email:         weileong.tan@digipen.edu
 Date:          2025-10-31
 Contribution:  100%
===============================================================================
*/

#include "Precompiled.h"
#include "Audio/AudioSystem.h"
#include "EntitySpawner.h"
#include "PlayerManager.h"
#include "ImguiSystem.h"
#include "Component.h" 
#include "Pause.h"
#include "GlobalPauseManager.h"

#include "Component.h"
#include "Pause/Pause.h"
#include "Graphics/RenderLayers.h"
extern Framework::CoreEngine* engine;
using Framework::Vector2D;
using Framework::ScriptComponent;

// ========================================================================
// PAUSE STATE (File scope)
// ========================================================================
namespace {
    bool g_wasPPressed = false;
    PauseMenuSimple::PauseMenuState g_pauseMenuState;
}

void level1_Load()
{
    LOG_INFO("LEVEL1", "=== Level1 Load ===");

    ConfigReader::LoadConfig("assets/valueloader.txt");
    LOG_INFO("LEVEL1", "Loaded UI configuration from valueloader.txt");

    // Reset pause state
    GlobalPause::SetPaused(false);
    g_wasPPressed = false;
    g_pauseMenuState = PauseMenuSimple::PauseMenuState();
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
    auto* scriptSystem = engine->GetScriptSystem();

    if (!spawner || !scriptSystem) {
        LOG_ERROR("LEVEL1", "Missing spawner or script system!");
        return;
    }

    // Spawn test entity
    auto testEntity = spawner->SpawnPlayer(Vector2D(0.0f, 0.0f));
    LOG_INFO("LEVEL1", "Spawned test entity: %u", testEntity.GetID());
    scriptSystem->LoadScript(testEntity, "./assets/scripts/test_simple.lua");

    // Spawn Player
    auto playerEntity = spawner->SpawnPlayer(Vector2D(0.0f, -0.5f));
    auto audioSystem = engine->GetAudioSystem();
    auto enemyEntity = spawner->SpawnEnemy(Vector2D(1.0f, -0.5f));

    auto* playerController = engine->GetPlayerController();
    playerController->SetAudioSystem(audioSystem);

    if (playerController) {
        playerController->SetPlayerEntity(playerEntity);
        playerController->SetGridMovementEnabled(false);
        LOG_INFO("LEVEL1", "PlayerController configured");
    }

    // Sprite animation setup
    auto* em = engine->GetEntityManager();
    auto* gfx = engine->GetGraphicsSystem();

    if (em && gfx) {
        // ============================================================================
        // Load animation configuration
        // ============================================================================
        LOG_INFO("LEVEL1", "Loaded animation configuration from file");

        // ============================
        // 1. Add SpriteAnimation
        // ============================
        auto& anim = em->AddComponent<Framework::SpriteAnimation>(playerEntity);

        auto* animSys = engine->GetAnimationSystem();
        if (animSys)
        {
            animSys->LoadAnimationConfig("assets/JSON/animations.json");

            // Set default animation
            anim.animName = "";
            anim.playing = true;
            anim.group = Framework::AnimGroup::Idle;
            anim.direction = Framework::AnimDirection::Front;
        }

        // ============================
        // 2. Add Renderable
        // ============================
        auto& rend = em->AddComponent<Framework::Renderable>(playerEntity);
        rend.visible = true;
        rend.layer = Framework::RenderLayers::Player;  // Use standard player layer (4)

        // ============================
        // 3. Add Transform
        // ============================
        auto& xform = em->AddComponent<Framework::Transform>(playerEntity);

        ConfigReader::LoadConfig("assets/valueloader.txt");
    }

    graphics->SetFollowTarget(playerEntity);
    LOG_INFO("LEVEL1", "Initialization complete");
}

void level1_Update()
{
    if (!engine) return;

    auto* input = engine->GetInputSystem();
    if (!input) return;

    // ========================================================================
    // P Key Toggle Pause (USING GLOBAL PAUSE)
    // ========================================================================
    bool isPPressed = input->IsKeyDown(Framework::KEY_P);

    if (isPPressed && !g_wasPPressed) {
        GlobalPause::Toggle();  // Toggle global pause state

        if (GlobalPause::IsPaused()) {
            LOG_INFO("LEVEL1", "Game PAUSED");
            if (engine->GetAudioSystem()) {
                engine->GetAudioSystem()->SetMasterVolume(0.0f);
            }
        }
        else {
            LOG_INFO("LEVEL1", "Game RESUMED");
            if (engine->GetAudioSystem()) {
                engine->GetAudioSystem()->SetMasterVolume(1.0f);
            }
        }
    }
    g_wasPPressed = isPPressed;

    // ========================================================================
    // If Paused, Handle Pause Menu Input
    // ========================================================================
    if (GlobalPause::IsPaused()) {
        PauseMenuSimple::PauseMenuCallbacks callbacks;

        callbacks.onResume = []() {
            GlobalPause::SetPaused(false);
            if (engine && engine->GetAudioSystem()) {
                engine->GetAudioSystem()->SetMasterVolume(1.0f);
            }
            LOG_INFO("LEVEL1", "Resume selected");
            };

        callbacks.onMainMenu = []() {
            GlobalPause::SetPaused(false);
            next = mainMenu;
            LOG_INFO("LEVEL1", "Returning to main menu");
            };

        callbacks.onExit = []() {
            next = GS_QUIT;
            LOG_INFO("LEVEL1", "Exiting game");
            };

        PauseMenuSimple::UpdatePauseMenu(engine, g_pauseMenuState, callbacks);

        return;  // Skip level-specific logic when paused
    }

    // ========================================================================
    // Normal Game Logic (Only when NOT paused)
    // ========================================================================

    // Update animations (from Ethan's branch)
    auto* animSys = engine->GetAnimationSystem();
    auto* em = engine->GetEntityManager();
    auto* gfx = engine->GetGraphicsSystem();
    auto player = engine->GetPlayerController()->GetPlayerEntity();

    if (animSys && em && gfx && player.IsValid())
    {
        if (em->HasComponent<Framework::SpriteAnimation>(player))
        {
            auto& anim = em->GetComponent<Framework::SpriteAnimation>(player);
            // Animation entries are handled by AnimationSystem
            auto& entries = animSys->animEntries;
        }
    }

    // Level switching
    if (input->IsKeyPressed(Framework::KEY_5)) {
        next = mainMenu;
    }
    else if (input->IsKeyPressed(Framework::KEY_6)) {
        next = LEVEL_2;
    }
}

void level1_Draw()
{
    if (!engine) return;

    auto graphics = engine->GetGraphicsSystem();
    if (!graphics) return;

    // ========================================================================
    // Normal Game UI (Always render - shows frozen frame when paused)
    // ========================================================================
    std::string fontLarge = ConfigReader::GetString("lv1_ui_font_large", "Sans48");

    std::string textScaling = ConfigReader::GetString("lv1_text_scaling", "Scaling Button: 3 & 4");
    std::string textRotation = ConfigReader::GetString("lv1_text_rotation", "Rotation Button: 7 & 8");
    std::string textMovement = ConfigReader::GetString("lv1_text_movement", "Character Movement: W A S D");
    std::string textMenu = ConfigReader::GetString("lv1_text_menu", "Back to Menu: 5");
    std::string textNextLevel = ConfigReader::GetString("lv1_text_next_level", "Next Level: 6");

    float textX = ConfigReader::GetFloat("lv1_ui_text_x", 50.0f);
    float textScalingY = ConfigReader::GetFloat("lv1_ui_text_scaling_y", 650.0f);
    float textRotationY = ConfigReader::GetFloat("lv1_ui_text_rotation_y", 550.0f);
    float textMovementY = ConfigReader::GetFloat("lv1_ui_text_movement_y", 450.0f);
    float textMenuY = ConfigReader::GetFloat("lv1_ui_text_menu_y", 350.0f);
    float textNextLevelY = ConfigReader::GetFloat("lv1_ui_text_next_level_y", 250.0f);

    float textScale = ConfigReader::GetFloat("lv1_ui_text_scale", 1.0f);
    float colorR = ConfigReader::GetFloat("lv1_ui_text_color_r", 1.0f);
    float colorG = ConfigReader::GetFloat("lv1_ui_text_color_g", 1.0f);
    float colorB = ConfigReader::GetFloat("lv1_ui_text_color_b", 1.0f);
    glm::vec3 textColor(colorR, colorG, colorB);

    graphics->DrawText4(fontLarge, textScaling, textX, textScalingY, textScale, textColor);
    graphics->DrawText4(fontLarge, textRotation, textX, textRotationY, textScale, textColor);
    graphics->DrawText4(fontLarge, textMovement, textX, textMovementY, textScale, textColor);
    graphics->DrawText4(fontLarge, textMenu, textX, textMenuY, textScale, textColor);
    graphics->DrawText4(fontLarge, textNextLevel, textX, textNextLevelY, textScale, textColor);

    // ========================================================================
    // Pause Menu Overlay (If paused)
    // ========================================================================
    if (GlobalPause::IsPaused()) {
        PauseMenuSimple::DrawPauseMenu(engine, g_pauseMenuState);
    }
}

void level1_Free()
{
    LOG_INFO("LEVEL1", "=== Level1 Free ===");

    // Reset pause state
    GlobalPause::SetPaused(false);
    g_wasPPressed = false;

    if (engine && engine->GetImGuiSystem()) {
        engine->GetImGuiSystem()->Disable();
    }

    if (engine) {
        engine->SetPlaying(false);

        if (engine->GetGraphicsSystem()) {
            engine->GetGraphicsSystem()->ClearFollowTarget();
        }
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