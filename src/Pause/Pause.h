/*
===============================================================================
 File:          Pause.h (With Game State Check)
 Author:        Padilla Carl Jameson Z
 Email:         c.padilla@digipen.edu
 Date:          2025-11-20
 Contribution:  100%
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