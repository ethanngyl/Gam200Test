/*
===============================================================================
 File:          MainMenu.cpp
 Author:        GE YONGQI
 Email:         yongqi.ge@digipen.edu
 Date:          2025-10-31
 Contribution:  100%
 ------------------------------------------------------------------------------
  Main menu state (implementation)

  Design notes:
     - Uses global UISystem to create Play and Exit buttons
     - Disables ImGui overlay to preserve clean UI visuals
     - Transitions to Level 1 or exits the game via button callbacks

  Highlights:
     - Integrates seamlessly with CoreEngine’s UISystem and GraphicsSystem
     - Automatically clears previous UI state during transitions
===============================================================================
*/


#include "Precompiled.h"

#include "ImguiSystem.h"

 // ============================================================================
 // GLOBAL VARIABLES
 // ============================================================================
extern Framework::CoreEngine* engine;

// Button pointers (managed by global UISystem)
static Framework::UIButton* playButton = nullptr;
static Framework::UIButton* exitButton = nullptr;

// ============================================================================
// BUTTON CALLBACK FUNCTIONS
// ============================================================================

void OnPlayButtonClicked()
{
    LOG_INFO("MENU", "Play button clicked! Transitioning to Level 1...");
    next = LEVEL_1;
}

void OnExitButtonClicked()
{
    LOG_INFO("MENU", "Exit button clicked! Quitting game...");
    next = GS_QUIT;
}

// ============================================================================
// MAIN MENU LIFECYCLE FUNCTIONS
// ============================================================================

void mainMenu_Load()
{
    LOG_INFO("MENU", "=== Main Menu Load ===");
}

void mainMenu_Initialize()
{
    LOG_INFO("MENU", "=== Main Menu Initialize ===");

    if (!engine) {
        LOG_ERROR("MENU", "Engine is null!");
        return;
    }

    // Disable ImGui in menu
    if (engine->GetImGuiSystem()) {
        engine->GetImGuiSystem()->Disable();
        LOG_INFO("MENU", "ImGui disabled in menu");
    }

    // Set to editor mode
    engine->SetPlaying(false);
    LOG_INFO("MENU", "Menu in EDITOR mode");

    auto graphics = engine->GetGraphicsSystem();
    if (graphics) {
        // Reset camera for menu
        graphics->SetCameraPosition(glm::vec3(0.0f, 0.0f, 0.0f));
        graphics->SetCameraZoom(1.0f);
    }

    auto audio = engine->GetAudioSystem();
    if (audio) {
        // Start playing the loaded menu music
        // The PlaySound function manages the channel and automatically applies loop settings
        audio->PlaySound("mmbgm", true);
        LOG_INFO("MENU", "Playing menu background music");
    }
    // ========================================================================
    // GET GLOBAL UI SYSTEM
    // ========================================================================
    auto ui = engine->GetUISystem();
    if (!ui) {
        LOG_ERROR("MENU", "UISystem is null!");
        return;
    }

    // Clear any existing buttons from previous states
    ui->ClearAllButtons();
    LOG_INFO("MENU", "Previous UI buttons cleared");

    // ========================================================================
    // CREATE BUTTONS USING GLOBAL UI SYSTEM
    // ========================================================================

    // Create Play button - ONE LINE!
    playButton = ui->CreateButton(
        "assets/ui_play.png",                    // Texture
        Framework::Vector2D(0.0f, 0.3f),         // Position
        Framework::Vector2D(0.8f, 0.3f),         // Size
        OnPlayButtonClicked                       // Callback
    );

    if (playButton) {
        LOG_INFO("MENU", "Play button created successfully");
    }

    // Create Exit button - ONE LINE!
    exitButton = ui->CreateButton(
        "assets/ui_exit.png",                    // Texture
        Framework::Vector2D(0.0f, -0.3f),        // Position
        Framework::Vector2D(0.8f, 0.3f),         // Size
        OnExitButtonClicked                       // Callback
    );

    if (exitButton) {
        LOG_INFO("MENU", "Exit button created successfully");
    }

    LOG_INFO("MENU", "Main Menu initialized with %zu buttons",
        ui->GetButtonCount());
}

void mainMenu_Update()
{
     if (engine && engine->GetAudioSystem()) {
     engine->GetAudioSystem()->Update(0.0f); 
    }
}

void mainMenu_Draw()
{

}

void mainMenu_Free()
{
    LOG_INFO("MENU", "=== Main Menu Free ===");

    if (engine && engine->GetAudioSystem()) {
        engine->GetAudioSystem()->StopAllSounds();
        LOG_INFO("MENU", "Stopping all menu sounds");
    }
    // Clean up buttons using global UI system
    if (engine) {
        auto ui = engine->GetUISystem();
        if (ui) {
            ui->ClearAllButtons();
            LOG_INFO("MENU", "All UI buttons cleared");
        }
    }

    playButton = nullptr;
    exitButton = nullptr;
}

void mainMenu_Unload()
{
    LOG_INFO("MENU", "=== Main Menu Unload ===");
    if (engine && engine->GetAudioSystem()) {
        engine->GetAudioSystem()->UnloadSound("mmbgm");
        LOG_INFO("MENU", "Unloaded menu audio assets");
    }
}