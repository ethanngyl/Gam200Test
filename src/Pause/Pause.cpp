/*
===============================================================================
 File:          Pause.h (With Game State Check)
 Author:        Padilla Carl Jameson Z
 Email:         c.padilla@digipen.edu
 Date:          2025-11-20
 Contribution:
 ------------------------------------------------------------------------------
  Simple Text-Based Pause Menu Implementation


===============================================================================
*/

#include "Pause.h"
#include "Core.h"
#include "GraphicsSystemV2.h"
#include "WindowSystem.h"

namespace PauseMenuSimple
{
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

        // Edge detection (now only 3 options: 0=Resume, 1=MainMenu, 2=Exit)
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

    void DrawPauseMenu(Framework::CoreEngine* engine,
        const PauseMenuState& state)
    {
        if (!engine) return;

        auto* graphics = engine->GetGraphicsSystem();
        auto* windowSystem = engine->GetWindowSystem();

        if (!graphics || !windowSystem) return;

        int windowWidth = windowSystem->GetWidth();
        int windowHeight = windowSystem->GetHeight();

        float centerX = windowWidth * 0.5f;
        float centerY = windowHeight * 0.5f;

        // ====================================================================
        // TITLE
        // ====================================================================
        graphics->DrawText4("Sans48", "PAUSED",
            centerX - 120, centerY + 200, 2.5f,
            glm::vec3(1.0f, 1.0f, 1.0f));

        // ====================================================================
        // MENU OPTIONS (Now only 3 options)
        // ====================================================================
        float menuStartY = centerY + 80;
        float menuSpacing = 70.0f;
        float textOffsetX = centerX - 150;

        // Option 0: Resume
        if (state.selectedOption == 0) {
            graphics->DrawText4("Sans48", "> 1. Resume <",
                textOffsetX, menuStartY, 1.3f,
                glm::vec3(1.0f, 1.0f, 0.3f));
        }
        else {
            graphics->DrawText4("Sans48", "  1. Resume",
                textOffsetX, menuStartY, 1.2f,
                glm::vec3(0.7f, 0.7f, 0.7f));
        }

        // Option 1: Main Menu
        if (state.selectedOption == 1) {
            graphics->DrawText4("Sans48", "> 2. Main Menu <",
                textOffsetX, menuStartY - menuSpacing, 1.3f,
                glm::vec3(1.0f, 1.0f, 0.3f));
        }
        else {
            graphics->DrawText4("Sans48", "  2. Main Menu",
                textOffsetX, menuStartY - menuSpacing, 1.2f,
                glm::vec3(0.7f, 0.7f, 0.7f));
        }

        // Option 2: Exit
        if (state.selectedOption == 2) {
            graphics->DrawText4("Sans48", "> 3. Exit Game <",
                textOffsetX, menuStartY - menuSpacing * 2, 1.3f,
                glm::vec3(1.0f, 1.0f, 0.3f));
        }
        else {
            graphics->DrawText4("Sans48", "  3. Exit Game",
                textOffsetX, menuStartY - menuSpacing * 2, 1.2f,
                glm::vec3(0.7f, 0.7f, 0.7f));
        }

        // ====================================================================
        // CONTROLS HINT
        // ====================================================================
        graphics->DrawText4("Sans48", "Press P to Resume | W/S or Arrow Keys to Navigate",
            centerX - 420, centerY - 250, 0.75f,
            glm::vec3(0.5f, 0.5f, 0.5f));

        graphics->DrawText4("Sans48", "Enter/Space to Select | 1-3 for Quick Select",
            centerX - 360, centerY - 300, 0.75f,
            glm::vec3(0.5f, 0.5f, 0.5f));
    }

} // namespace PauseMenuSimple