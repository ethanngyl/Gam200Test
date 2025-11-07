/*
===============================================================================
 File:          LevelSelect.cpp
 Author:        TAN WEI LEONG
 Email:         weileong.tan@digipen.edu
 Date:          2025-10-31
 Contribution:  100%
 ------------------------------------------------------------------------------
  Level Select state (implementation)

  Design notes:
     - Uses global UISystem to create Level 1, Level 2, Level 3, and Back buttons
     - Disables ImGui overlay to preserve clean UI visuals
     - Transitions to selected level or back to main menu via button callbacks

  Highlights:
     - Integrates seamlessly with CoreEngine's UISystem and GraphicsSystem
     - Automatically clears previous UI state during transitions
     - Configuration-driven UI text rendering
===============================================================================
*/

#include "Precompiled.h"
#include "ImguiSystem.h"

// ============================================================================
// GLOBAL VARIABLES
// ============================================================================
extern Framework::CoreEngine* engine;

// Button pointers (managed by global UISystem)
static Framework::UIButton* level1Button = nullptr;
static Framework::UIButton* level2Button = nullptr;
static Framework::UIButton* level3Button = nullptr;
static Framework::UIButton* backButton = nullptr;

// ============================================================================
// BUTTON CALLBACK FUNCTIONS
// ============================================================================

void OnLevel1ButtonClicked()
{
    LOG_INFO("LEVELSELECT", "Level 1 button clicked! Transitioning to Level 1...");
    next = LEVEL_1;
}

void OnLevel2ButtonClicked()
{
    LOG_INFO("LEVELSELECT", "Level 2 button clicked! Transitioning to Level 2...");
    next = LEVEL_2;
}

void OnLevel3ButtonClicked()
{
    LOG_INFO("LEVELSELECT", "Level 3 button clicked! Transitioning to Level 3...");
    next = LEVEL_3;
}

void OnBackButtonClicked()
{
    LOG_INFO("LEVELSELECT", "Back button clicked! Returning to Main Menu...");
    next = mainMenu;
}

// ============================================================================
// LEVEL SELECT LIFECYCLE FUNCTIONS
// ============================================================================

void level_select_Load()
{
    LOG_INFO("LEVELSELECT", "=== Level Select Load ===");
}

void level_select_Initialize()
{
    LOG_INFO("LEVELSELECT", "=== Level Select Initialize ===");

    // ========================================================================
    // LOAD CONFIGURATION
    // ========================================================================
    ConfigReader::LoadConfig("assets/valueloader.txt");
    LOG_INFO("LEVELSELECT", "Loaded level select configuration from valueloader.txt");

    if (!engine) {
        LOG_ERROR("LEVELSELECT", "Engine is null!");
        return;
    }

    // Disable ImGui in level select
    if (engine->GetImGuiSystem()) {
        engine->GetImGuiSystem()->Disable();
        LOG_INFO("LEVELSELECT", "ImGui disabled in level select");
    }

    // Set to editor mode
    engine->SetPlaying(false);
    LOG_INFO("LEVELSELECT", "Level Select in EDITOR mode");

    auto graphics = engine->GetGraphicsSystem();
    if (graphics) {
        // Reset camera for level select
        graphics->SetCameraPosition(glm::vec3(0.0f, 0.0f, 0.0f));
        graphics->SetCameraZoom(1.0f);
    }

    auto audio = engine->GetAudioSystem();
    if (audio) {
        // Start playing the menu music (same as main menu)
        audio->PlaySound("mmbgm", true);
        LOG_INFO("LEVELSELECT", "Playing level select background music");
    }

    // ========================================================================
    // GET GLOBAL UI SYSTEM
    // ========================================================================
    auto ui = engine->GetUISystem();
    if (!ui) {
        LOG_ERROR("LEVELSELECT", "UISystem is null!");
        return;
    }

    // Clear any existing buttons from previous states
    ui->ClearAllButtons();
    LOG_INFO("LEVELSELECT", "Previous UI buttons cleared");

    // ========================================================================
    // CREATE BUTTONS USING GLOBAL UI SYSTEM
    // ========================================================================

    // Create Level 1 button
    level1Button = ui->CreateButton(
        "assets/Ui_btn.png",                     // Texture
        Framework::Vector2D(0.0f, 0.6f),         // Position (top)
        Framework::Vector2D(1.0f, 0.4f),         // Size
        OnLevel1ButtonClicked                     // Callback
    );

    if (level1Button) {
        LOG_INFO("LEVELSELECT", "Level 1 button created successfully");
    }

    // Create Level 2 button
    level2Button = ui->CreateButton(
        "assets/Ui_btn.png",                     // Texture
        Framework::Vector2D(0.0f, 0.3f),         // Position (upper middle)
        Framework::Vector2D(1.0f, 0.4f),         // Size
        OnLevel2ButtonClicked                     // Callback
    );

    if (level2Button) {
        LOG_INFO("LEVELSELECT", "Level 2 button created successfully");
    }

    // Create Level 3 button
    level3Button = ui->CreateButton(
        "assets/Ui_btn.png",                     // Texture
        Framework::Vector2D(0.0f, 0.0f),        // Position (lower middle)
        Framework::Vector2D(1.0f, 0.4f),         // Size
        OnLevel3ButtonClicked                     // Callback
    );

    if (level3Button) {
        LOG_INFO("LEVELSELECT", "Level 3 button created successfully");
    }

    // Create Back button
    backButton = ui->CreateButton(
        "assets/Ui_btn.png",                     // Texture
        Framework::Vector2D(0.0f, -0.3f),        // Position (bottom)
        Framework::Vector2D(1.0f, 0.4f),         // Size
        OnBackButtonClicked                       // Callback
    );

    if (backButton) {
        LOG_INFO("LEVELSELECT", "Back button created successfully");
    }

    LOG_INFO("LEVELSELECT", "Level Select initialized with %zu buttons",
        ui->GetButtonCount());
}

void level_select_Update()
{
    if (engine && engine->GetAudioSystem()) {
        engine->GetAudioSystem()->Update(0.0f);
    }
}

void level_select_Draw()
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
    std::string fontLarge = ConfigReader::GetString("levelselect_ui_font_large", "Sans48");

    // Text content
    std::string textLevel1 = ConfigReader::GetString("levelselect_text_level1", "Level 1");
    std::string textLevel2 = ConfigReader::GetString("levelselect_text_level2", "Level 2");
    std::string textLevel3 = ConfigReader::GetString("levelselect_text_level3", "Level 3");
    std::string textBack = ConfigReader::GetString("levelselect_text_back", "Back");

    // Text scale and color
    float textScale = ConfigReader::GetFloat("levelselect_ui_text_scale", 1.0f);
    float colorR = ConfigReader::GetFloat("levelselect_ui_text_color_r", 1.0f);
    float colorG = ConfigReader::GetFloat("levelselect_ui_text_color_g", 1.0f);
    float colorB = ConfigReader::GetFloat("levelselect_ui_text_color_b", 1.0f);
    glm::vec3 textColor(colorR, colorG, colorB);

    // Text offsets for centering
    float level1OffsetX = ConfigReader::GetFloat("levelselect_level1_text_offset_x", -90.0f);
    float level1OffsetY = ConfigReader::GetFloat("levelselect_level1_text_offset_y", -24.0f);
    float level2OffsetX = ConfigReader::GetFloat("levelselect_level2_text_offset_x", -90.0f);
    float level2OffsetY = ConfigReader::GetFloat("levelselect_level2_text_offset_y", -24.0f);
    float level3OffsetX = ConfigReader::GetFloat("levelselect_level3_text_offset_x", -90.0f);
    float level3OffsetY = ConfigReader::GetFloat("levelselect_level3_text_offset_y", -24.0f);
    float backOffsetX = ConfigReader::GetFloat("levelselect_back_text_offset_x", -60.0f);
    float backOffsetY = ConfigReader::GetFloat("levelselect_back_text_offset_y", -24.0f);

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

    // Level 1 button text (world coordinates: 0.0, 0.6)
    if (level1Button) {
        glm::vec2 screenPos = worldToScreen(level1Button->position.x, level1Button->position.y);
        float textX = screenPos.x + level1OffsetX;
        float textY = screenPos.y + level1OffsetY;
        graphics->DrawText4(fontLarge, textLevel1, textX, textY, textScale, textColor);
    }

    // Level 2 button text (world coordinates: 0.0, 0.1)
    if (level2Button) {
        glm::vec2 screenPos = worldToScreen(level2Button->position.x, level2Button->position.y);
        float textX = screenPos.x + level2OffsetX;
        float textY = screenPos.y + level2OffsetY;
        graphics->DrawText4(fontLarge, textLevel2, textX, textY, textScale, textColor);
    }

    // Level 3 button text (world coordinates: 0.0, -0.4)
    if (level3Button) {
        glm::vec2 screenPos = worldToScreen(level3Button->position.x, level3Button->position.y);
        float textX = screenPos.x + level3OffsetX;
        float textY = screenPos.y + level3OffsetY;
        graphics->DrawText4(fontLarge, textLevel3, textX, textY, textScale, textColor);
    }

    // Back button text (world coordinates: 0.0, -0.9)
    if (backButton) {
        glm::vec2 screenPos = worldToScreen(backButton->position.x, backButton->position.y);
        float textX = screenPos.x + backOffsetX;
        float textY = screenPos.y + backOffsetY;
        graphics->DrawText4(fontLarge, textBack, textX, textY, textScale, textColor);
    }
}

void level_select_Free()
{
    LOG_INFO("LEVELSELECT", "=== Level Select Free ===");

    if (engine && engine->GetAudioSystem()) {
        engine->GetAudioSystem()->StopAllSounds();
        LOG_INFO("LEVELSELECT", "Stopping all level select sounds");
    }

    // Clean up buttons using global UI system
    if (engine) {
        auto ui = engine->GetUISystem();
        if (ui) {
            ui->ClearAllButtons();
            LOG_INFO("LEVELSELECT", "All UI buttons cleared");
        }
    }

    level1Button = nullptr;
    level2Button = nullptr;
    level3Button = nullptr;
    backButton = nullptr;
}

void level_select_Unload()
{
    LOG_INFO("LEVELSELECT", "=== Level Select Unload ===");
}