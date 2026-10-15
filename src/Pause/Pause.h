/**
===============================================================================
 File:          Pause.h
 Author:        Padilla Carl Jameson Z.
 Email:         c.padilla@digipen.edu
 Date:          2025-11-20
 Contribution:  100%
 ------------------------------------------------------------------------------

 PAUSE MENU - Simple Pause System with Game State Check

 Brief:
    Simple pause menu system with keyboard navigation and callback-based
    actions. Provides Resume, Main Menu, and Exit options with visual
    selection feedback. Includes input state tracking to prevent repeated
    key presses.

 Usage:
    // Setup callbacks
    PauseMenuSimple::PauseMenuCallbacks callbacks;
    callbacks.onResume = []() { SetPaused(false); };
    callbacks.onMainMenu = []() { GSM::SetNextState("MainMenu"); };
    callbacks.onExit = []() { engine->Quit(); };

    // Create state
    PauseMenuSimple::PauseMenuState state;

    // In Update (when paused)
    PauseMenuSimple::UpdatePauseMenu(engine, state, callbacks);

    // In Draw (when paused)
    PauseMenuSimple::DrawPauseMenu(engine, state);


 Copyright (C) 2025 DigiPen Institute of Technology.
 Reproduction or disclosure of this file or its contents
 without the prior written consent of DigiPen Institute of
 Technology is prohibited.
===============================================================================
*/

#pragma once
#include "Precompiled.h"

namespace Framework
{
    class CoreEngine;
}

namespace PauseMenuSimple
{
    // ========================================================================
    // PAUSE MENU CALLBACKS
    // ========================================================================

    typedef void (*PauseMenuCallback)();

    struct PauseMenuCallbacks
    {
        PauseMenuCallback onResume = nullptr;
        PauseMenuCallback onMainMenu = nullptr;
        PauseMenuCallback onExit = nullptr;
        // Removed: onSettings
    };

    // ========================================================================
    // PAUSE MENU STATE
    // ========================================================================

    struct PauseMenuState
    {
        int selectedOption = 0;  // 0=Resume, 1=MainMenu, 2=Exit
        bool wasUpPressed = false;
        bool wasDownPressed = false;
        bool wasEnterPressed = false;
    };

    // ========================================================================
    // PAUSE MENU RENDERING
    // ========================================================================

    /**
     * @brief Update pause menu input (call in Update)
     * @return true if an option was selected
     */
    bool UpdatePauseMenu(Framework::CoreEngine* engine,
        PauseMenuState& state,
        const PauseMenuCallbacks& callbacks);

    /**
     * @brief Draw the pause menu using DrawText4 (call in Draw)
     */
    void DrawPauseMenu(Framework::CoreEngine* engine,
        const PauseMenuState& state);

} // namespace PauseMenuSimple