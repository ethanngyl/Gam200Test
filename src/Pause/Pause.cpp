/*
===============================================================================
 File:          Pause.cpp (Fixed Version - Config-Driven Layout)
 Author:        Padilla Carl Jameson Z
 Email:         c.padilla@digipen.edu
 Date:          2025-11-27
 Contribution:  Original: Carl | Fix: Claude
 ------------------------------------------------------------------------------
  Text-Based Pause Menu with Framebuffer-Aware Positioning

  Changes:
  - Uses glfwGetFramebufferSize() instead of window size (fixes DPI scaling)
  - All layout values read from valueloader.txt (easy adjustment)
  - Consistent with Level1 UI style
===============================================================================
*/

#include "Pause.h"
#include "Core.h"
#include "GraphicsSystemV2.h"
#include "WindowSystem.h"
#include "ConfigReader.h"  // Make sure this is included!

namespace PauseMenuSimple
{
    /**
    * @brief Handles pause menu input and navigation.
    * @return true if a menu action was executed, false otherwise
    */
    bool UpdatePauseMenu(Framework::CoreEngine* engine,
        PauseMenuState& state,
        const PauseMenuCallbacks& callbacks)
    {
        if (!engine) return false;

        auto* input = engine->GetInputSystem();
        if (!input) return false;

        // ====================================================================
        // Arrow key navigation
        // ====================================================================
        bool isUpPressed = input->IsKeyDown(Framework::KEY_UP) ||
            input->IsKeyDown(Framework::KEY_W);
        bool isDownPressed = input->IsKeyDown(Framework::KEY_DOWN) ||
            input->IsKeyDown(Framework::KEY_S);
        bool isEnterPressed = input->IsKeyDown(Framework::KEY_ENTER) ||
            input->IsKeyDown(Framework::KEY_SPACE);

        // Edge detection (3 options: 0=Resume, 1=MainMenu, 2=Exit)
        if (isUpPressed && !state.wasUpPressed) {
            state.selectedOption--;
            if (state.selectedOption < 0) state.selectedOption = 2;
        }

        if (isDownPressed && !state.wasDownPressed) {
            state.selectedOption++;
            if (state.selectedOption > 2) state.selectedOption = 0;
        }

        state.wasUpPressed = isUpPressed;
        state.wasDownPressed = isDownPressed;

        // ====================================================================
        // Number key shortcuts (1=Resume, 2=MainMenu, 3=Exit)
        // ====================================================================
        if (input->IsKeyPressed(Framework::KEY_1)) {
            if (callbacks.onResume) callbacks.onResume();
            return true;
        }
        else if (input->IsKeyPressed(Framework::KEY_2)) {
            if (callbacks.onMainMenu) callbacks.onMainMenu();
            return true;
        }
        else if (input->IsKeyPressed(Framework::KEY_3)) {
            if (callbacks.onExit) callbacks.onExit();
            return true;
        }

        // ====================================================================
        // Enter key to select
        // ====================================================================
        if (isEnterPressed && !state.wasEnterPressed) {
            switch (state.selectedOption) {
            case 0:  // Resume
                if (callbacks.onResume) callbacks.onResume();
                return true;
            case 1:  // Main Menu
                if (callbacks.onMainMenu) callbacks.onMainMenu();
                return true;
            case 2:  // Exit
                if (callbacks.onExit) callbacks.onExit();
                return true;
            }
        }

        state.wasEnterPressed = isEnterPressed;

        return false;
    }

    /**
     * @brief Renders the pause menu with config-driven layout.
     */
    void DrawPauseMenu(Framework::CoreEngine* engine,
        const PauseMenuState& state)
    {
        if (!engine) return;

        auto* graphics = engine->GetGraphicsSystem();
        auto* windowSystem = engine->GetWindowSystem();

        if (!graphics || !windowSystem) return;

        // ====================================================================
        // FIX: Use framebuffer size instead of window size
        // This correctly handles high-DPI displays (e.g., Retina, 4K)
        // ====================================================================
        int fbWidth, fbHeight;
        glfwGetFramebufferSize(windowSystem->GetWindow(), &fbWidth, &fbHeight);

        // Load layout configuration from valueloader.txt
        float centerRatioX = ConfigReader::GetFloat("pause_menu_center_x_ratio", 0.5f);
        float centerRatioY = ConfigReader::GetFloat("pause_menu_center_y_ratio", 0.5f);

        float centerX = fbWidth * centerRatioX;
        float centerY = fbHeight * centerRatioY;

        // ====================================================================
        // TITLE CONFIGURATION
        // ====================================================================
        std::string titleText = ConfigReader::GetString("pause_menu_title_text", "PAUSED");
        float titleOffsetX = ConfigReader::GetFloat("pause_menu_title_offset_x", -120.0f);
        float titleOffsetY = ConfigReader::GetFloat("pause_menu_title_offset_y", 200.0f);
        float titleScale = ConfigReader::GetFloat("pause_menu_title_scale", 2.5f);

        float titleR = ConfigReader::GetFloat("pause_menu_title_color_r", 1.0f);
        float titleG = ConfigReader::GetFloat("pause_menu_title_color_g", 1.0f);
        float titleB = ConfigReader::GetFloat("pause_menu_title_color_b", 0.3f);
        glm::vec3 titleColor(titleR, titleG, titleB);

        // ====================================================================
        // MENU OPTIONS CONFIGURATION
        // ====================================================================
        float menuStartYOffset = ConfigReader::GetFloat("pause_menu_start_y_offset", 80.0f);
        float menuSpacing = ConfigReader::GetFloat("pause_menu_spacing", 70.0f);
        float textOffsetX = ConfigReader::GetFloat("pause_menu_text_offset_x", -100.0f);
        float normalScale = ConfigReader::GetFloat("pause_menu_normal_scale", 1.2f);
        float selectedScale = ConfigReader::GetFloat("pause_menu_selected_scale", 1.3f);

        // Colors
        float selectedR = ConfigReader::GetFloat("pause_menu_selected_color_r", 1.0f);
        float selectedG = ConfigReader::GetFloat("pause_menu_selected_color_g", 1.0f);
        float selectedB = ConfigReader::GetFloat("pause_menu_selected_color_b", 0.3f);
        glm::vec3 selectedColor(selectedR, selectedG, selectedB);

        float normalR = ConfigReader::GetFloat("pause_menu_normal_color_r", 0.7f);
        float normalG = ConfigReader::GetFloat("pause_menu_normal_color_g", 0.7f);
        float normalB = ConfigReader::GetFloat("pause_menu_normal_color_b", 0.7f);
        glm::vec3 normalColor(normalR, normalG, normalB);

        // ====================================================================
        // DRAW TITLE
        // ====================================================================
        graphics->DrawText4("Sans48", titleText,
            centerX + titleOffsetX,
            centerY + titleOffsetY,
            titleScale,
            titleColor);

        // ====================================================================
        // DRAW MENU OPTIONS
        // ====================================================================
        float menuStartY = centerY + menuStartYOffset;
        float textBaseX = centerX + textOffsetX;

        // Menu option texts
        const char* options[] = { "Resume", "Main Menu", "Exit Game" };

        for (int i = 0; i < 3; ++i) {
            bool isSelected = (state.selectedOption == i);

            // Add selection markers
            std::string text = isSelected ?
                ("> " + std::string(options[i]) + " <") :
                ("  " + std::string(options[i]));

            float scale = isSelected ? selectedScale : normalScale;
            glm::vec3 color = isSelected ? selectedColor : normalColor;

            graphics->DrawText4("Sans48", text,
                textBaseX,
                menuStartY - i * menuSpacing,
                scale,
                color);
        }

        // ====================================================================
        // DRAW CONTROL HINTS
        // ====================================================================
        std::string hint1Text = ConfigReader::GetString("pause_menu_hint1_text",
            "W/S or Arrow Keys to Navigate");
        float hint1OffsetX = ConfigReader::GetFloat("pause_menu_hint1_offset_x", -280.0f);
        float hint1OffsetY = ConfigReader::GetFloat("pause_menu_hint1_offset_y", -150.0f);

        std::string hint2Text = ConfigReader::GetString("pause_menu_hint2_text",
            "Enter/Space to Select | 1-3 for Quick Select");
        float hint2OffsetX = ConfigReader::GetFloat("pause_menu_hint2_offset_x", -380.0f);
        float hint2OffsetY = ConfigReader::GetFloat("pause_menu_hint2_offset_y", -200.0f);

        float hintScale = ConfigReader::GetFloat("pause_menu_hint_scale", 0.75f);

        graphics->DrawText4("Sans48", hint1Text,
            centerX + hint1OffsetX,
            centerY + hint1OffsetY,
            hintScale,
            titleColor);  // Use same color as title

        graphics->DrawText4("Sans48", hint2Text,
            centerX + hint2OffsetX,
            centerY + hint2OffsetY,
            hintScale,
            titleColor);

        // ====================================================================
        // DEBUG INFO (comment out in production)
        // ====================================================================
#ifdef _DEBUG
// Uncomment to debug positioning:
// LOG_INFO("PAUSE", "FB: %dx%d, Center: (%.1f, %.1f)", 
//          fbWidth, fbHeight, centerX, centerY);
#endif
    }

} // namespace PauseMenuSimple