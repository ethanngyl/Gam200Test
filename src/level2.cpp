/*
===============================================================================
 File:          level2.cpp
 Author:        GE YONGQI
 Email:         yongqi.ge@digipen.edu
 Date:          2025-10-31
 Contribution:  100%
 ------------------------------------------------------------------------------
  FIXED: Added screen size restoration to prevent text disappearing bug

  Modified: 2025-11-22
  - Added pause functionality using GlobalPauseManager

  Changes:
  1. level2_Draw() - Reset TextRenderer screen size before drawing
  2. level2_Free() - Restore TextRenderer screen size when leaving Level2
  3. Added pause menu support
===============================================================================
*/

#include "Precompiled.h"   

#include "EntitySpawner.h"      
#include "PlayerManager.h"
#include "ImguiSystem.h"
#include "Pathfinding.h"
#include "Pause/Pause.h"
#include "GlobalPauseManager.h"
#include "Audio/AudioSystem.h"

#include "TimeConstants.h"


extern Framework::CoreEngine* engine;
using Framework::Vector2D;

// ========================================================================
// PAUSE STATE (File scope)
// ========================================================================
namespace {
    bool g_wasPPressed = false;
    PauseMenuSimple::PauseMenuState g_pauseMenuState;
}


void level2_Load()
{
    ConfigReader::LoadConfig("assets/valueloader.txt");

    // Reset pause state
    g_wasPPressed = false;
    g_pauseMenuState = PauseMenuSimple::PauseMenuState();
}

void level2_Initialize()
{

    if (!engine) {
        LOG_ERROR("LEVEL2", "Engine is null!");
        return;
    }

    // =======================
    // LOAD SPRITE ANIMATIONS
    // =======================
    auto* animSys = engine->GetAnimationSystem();
    if (animSys)
    {
        animSys->LoadAnimationConfig("assets/animations.json");
        LOG_INFO("LEVEL2", "Loaded animations.json for editor animations");
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
    auto audioSystem = engine->GetAudioSystem();

    auto spawner = engine->GetSpawner();

    auto playerController = engine->GetPlayerController();
    auto playerEntity = spawner->SpawnPlayer(Vector2D(0.0f, -0.5f));

    playerController->SetPlayerEntity(playerEntity);
    playerController->SetEntitySpawner(spawner);
    playerController->SetAudioSystem(audioSystem);


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
            LOG_INFO("LEVEL2", "Game PAUSED");

            // Disable ImGui to prevent interference with pause menu text
            if (engine->GetImGuiSystem()) {
                engine->GetImGuiSystem()->Disable();
            }

            if (engine->GetAudioSystem()) {
                engine->GetAudioSystem()->SetMasterVolume(0.0f);
            }
        }
        else {
            LOG_INFO("LEVEL2", "Game RESUMED (P key)");

            // Re-enable ImGui when resuming via P key
            if (engine->GetImGuiSystem()) {
                engine->GetImGuiSystem()->Enable();
            }

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

            // ⭐ FIX: Re-enable ImGui when resuming from pause menu
            // This is critical because the menu bypass P key detection
            if (engine && engine->GetImGuiSystem()) {
                engine->GetImGuiSystem()->Enable();
            }

            if (engine && engine->GetAudioSystem()) {
                engine->GetAudioSystem()->SetMasterVolume(1.0f);
            }
            LOG_INFO("LEVEL2", "Resume selected");
            };

        callbacks.onMainMenu = []() {
            GlobalPause::SetPaused(false);

            // Also re-enable ImGui before leaving level
            if (engine && engine->GetImGuiSystem()) {
                engine->GetImGuiSystem()->Enable();
            }

            next = mainMenu;
            LOG_INFO("LEVEL2", "Returning to main menu");
            };

        callbacks.onExit = []() {
            next = GS_QUIT;
            LOG_INFO("LEVEL2", "Exiting game");
            };

        PauseMenuSimple::UpdatePauseMenu(engine, g_pauseMenuState, callbacks);

        return;  // Skip level-specific logic when paused
    }

    // ========================================================================
    // Normal Game Logic (Only when NOT paused)
    // ========================================================================
    if (input->IsKeyPressed(Framework::KEY_5))
    // 1. Handle input switching levels
    if (engine && engine->GetInputSystem() &&
        engine->GetInputSystem()->IsKeyPressed(Framework::KEY_5))
    {
        next = mainMenu;
    }

    if (input->IsKeyPressed(Framework::KEY_3))
    {
        next = LEVEL_3;
    }

    // =============================================================
    // 2. UPDATE ANIMATIONS IN LEVEL EDITOR
    // =============================================================
    // Animate in level editor using fixed-step (60 FPS)
    auto* animSys = engine->GetAnimationSystem();
    if (animSys)
        animSys->Update(Framework::Time::FIXED_DT);
}

void level2_Draw()
{
    if (!engine) return;

    auto graphics = engine->GetGraphicsSystem();
    if (!graphics) return;

    // ========================================================================
    // Reset TextRenderer screen size to window size
    // This prevents text from disappearing when returning from Level2
    // ========================================================================
    if (engine->GetWindowSystem()) {
        int fbW, fbH;
        glfwGetFramebufferSize(engine->GetWindowSystem()->GetWindow(), &fbW, &fbH);
        graphics->GetTextRenderer().setScreenSize(fbW, fbH);
        // Uncomment for debugging:
        // LOG_INFO("LEVEL2", "Text renderer screen size set to: %dx%d", fbW, fbH);
    }

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
    // Render UI text (using values from the configuration file)
    // ========================================================================
    graphics->DrawText4(fontLarge, textScaling, textX, texty, textScale, glm::vec3(colorR, colorG, colorB));

    // ========================================================================
    // Pause Menu Overlay (If paused)
    // ========================================================================
    if (GlobalPause::IsPaused()) {
        PauseMenuSimple::DrawPauseMenu(engine, g_pauseMenuState);
    }
}

void level2_Free()
{
    LOG_INFO("LEVEL2", "=== Level2 Free ===");

    // Reset pause state
    g_wasPPressed = false;
    GlobalPause::SetPaused(false);  // Ensure pause is cleared when leaving level

    if (engine && engine->GetImGuiSystem()) {
        engine->GetImGuiSystem()->Disable();
        LOG_INFO("LEVEL2", "ImGui disabled");
    }

    // ========================================================================
    // Restore TextRenderer screen size to window size
    // When Level2 enables ImGui viewport mode, TextRenderer gets set to
    // viewport size (e.g., 800x600). We need to restore it to window size
    // (e.g., 1920x1080) so other levels can display text correctly.
    // ========================================================================
    if (engine && engine->GetGraphicsSystem() && engine->GetWindowSystem()) {
        int fbW, fbH;
        glfwGetFramebufferSize(engine->GetWindowSystem()->GetWindow(), &fbW, &fbH);
        engine->GetGraphicsSystem()->GetTextRenderer().setScreenSize(fbW, fbH);
        LOG_INFO("LEVEL2", "Restored text renderer to window size: %dx%d", fbW, fbH);
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