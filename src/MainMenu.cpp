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
    next = Level_select;
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

    // ========================================================================
    // LOAD CONFIGURATION
    // ========================================================================
    ConfigReader::LoadConfig("assets/valueloader.txt");
    LOG_INFO("MENU", "Loaded menu configuration from valueloader.txt");

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
        "assets/Ui_btn.png",                    // Texture
        Framework::Vector2D(0.0f, 0.3f),         // Position
        Framework::Vector2D(1.0f, 0.5f),         // Size
        OnPlayButtonClicked                       // Callback
    );

    if (playButton) {
        LOG_INFO("MENU", "Play button created successfully");
    }

    // Create Exit button - ONE LINE!
    exitButton = ui->CreateButton(
        "assets/Ui_btn.png",                    // Texture
        Framework::Vector2D(0.0f, -0.3f),        // Position
        Framework::Vector2D(1.0f, 0.5f),         // Size
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
    if (!engine) return;

    auto graphics = engine->GetGraphicsSystem();
    auto windowSystem = engine->GetWindowSystem();
    if (!graphics || !windowSystem) return;

    GLFWwindow* window = windowSystem->GetWindow();
    if (!window) return;

    // ========================================================================
    // READ UI TEXT SETTINGS FROM CONFIGURATION FILE
    // ========================================================================

    // Font settings
    std::string fontLarge = ConfigReader::GetString("menu_ui_font_large", "Sans48");

    // Text content
    std::string textPlay = ConfigReader::GetString("menu_text_play", "Play");
    std::string textExit = ConfigReader::GetString("menu_text_exit", "Exit");

    // Text scale and color
    float textScale = ConfigReader::GetFloat("menu_ui_text_scale", 1.0f);
    float colorR = ConfigReader::GetFloat("menu_ui_text_color_r", 1.0f);
    float colorG = ConfigReader::GetFloat("menu_ui_text_color_g", 1.0f);
    float colorB = ConfigReader::GetFloat("menu_ui_text_color_b", 1.0f);
    glm::vec3 textColor(colorR, colorG, colorB);

    // Text offsets for centering
    float playOffsetX = ConfigReader::GetFloat("menu_play_text_offset_x", -60.0f);
    float playOffsetY = ConfigReader::GetFloat("menu_play_text_offset_y", -24.0f);
    float exitOffsetX = ConfigReader::GetFloat("menu_exit_text_offset_x", -50.0f);
    float exitOffsetY = ConfigReader::GetFloat("menu_exit_text_offset_y", -24.0f);

    // ========================================================================
    // COORDINATE CONVERSION: WORLD TO SCREEN
    // ========================================================================

    // Get framebuffer size for accurate pixel coordinates
    int fbWidth, fbHeight;
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);

    // World coordinate to screen coordinate conversion function
    auto worldToScreen = [&](float worldX, float worldY) -> glm::vec2 {
        // Step 1: World coordinates -> NDC (using camera's ViewProjection matrix)
        glm::mat4 viewProj = graphics->GetCamera().GetViewProjectionMatrix();
        glm::vec4 clipSpace = viewProj * glm::vec4(worldX, worldY, 0.0f, 1.0f);

        // Step 2: NDC coordinates
        float ndcX = clipSpace.x;
        float ndcY = clipSpace.y;

        // Step 3: NDC -> Screen pixel coordinates
        // NDC range: [-1, 1] -> Screen pixels: [0, fbWidth] and [0, fbHeight]
        // Note: TextRenderer's origin is at bottom-left, Y-axis points up
        float screenX = (ndcX + 1.0f) * 0.5f * fbWidth;
        float screenY = (ndcY + 1.0f) * 0.5f * fbHeight;

        return glm::vec2(screenX, screenY);
        };

    // ========================================================================
    // RENDER TEXT ON BUTTONS (using configuration values)
    // ========================================================================

    // Play button text (world coordinates: 0.0, 0.3)
    if (playButton) {
        glm::vec2 screenPos = worldToScreen(playButton->position.x, playButton->position.y);
        float textX = screenPos.x + playOffsetX;  // Apply offset from config
        float textY = screenPos.y + playOffsetY;
        graphics->DrawText4(fontLarge, textPlay, textX, textY, textScale, textColor);
    }

    // Exit button text (world coordinates: 0.0, -0.3)
    if (exitButton) {
        glm::vec2 screenPos = worldToScreen(exitButton->position.x, exitButton->position.y);
        float textX = screenPos.x + exitOffsetX;  // Apply offset from config
        float textY = screenPos.y + exitOffsetY;
        graphics->DrawText4(fontLarge, textExit, textX, textY, textScale, textColor);
    }
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
    //Do not use the code below, unloading will completely remove it from the fmod library
    //if (engine && engine->GetAudioSystem()) {
    //    engine->GetAudioSystem()->UnloadSound("mmbgm");
    //    LOG_INFO("MENU", "Unloaded menu audio assets");
    //}
}