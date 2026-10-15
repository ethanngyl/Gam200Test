/**
===============================================================================
 File:          WindowEventHandler.h
 Author:        Padilla Carl Jameson Z.
 Email:         c.padilla@digipen.edu
 Date:          2025-11-28
 Contribution:  100%
 ------------------------------------------------------------------------------

 WINDOW EVENT HANDLER - Focus, Minimize & Fullscreen Management

 Brief:
    Handles window events including minimize/restore, focus loss/gain, and
    fullscreen toggling. Automatically pauses the game and mutes audio when
    the window loses focus or is minimized (TECH 1701). Supports ALT+ENTER
    and CTRL+ALT+ENTER fullscreen toggle (TECH 1702). Tracks manual vs auto
    pause states to correctly restore game state on window regain.

 Features:
    - Auto-pause on minimize (GLFW_ICONIFIED)
    - Auto-pause on focus loss (CTRL+ALT+DEL, ALT+TAB, etc.)
    - Audio mute/restore on pause/resume
    - Input state reset on focus changes
    - Fullscreen toggle via ALT+ENTER or CTRL+ALT+ENTER

 Usage:
    // Initialize once at startup
    WindowEventHandler::Initialize();

    // Call every frame in game loop
    WindowEventHandler::Update(glfwWindow);

    // Check if game was auto-paused by window events
    if (WindowEventHandler::WasAutoPaused()) {
        // Show pause overlay, etc.
    }

 Copyright (C) 2026 DigiPen Institute of Technology.
 Reproduction or disclosure of this file or its contents
 without the prior written consent of DigiPen Institute of
 Technology is prohibited.
===============================================================================
*/

#pragma once
#include "Precompiled.h"
#include <GLFW/glfw3.h>

namespace Framework {

    class WindowEventHandler {
    public:
        static void Initialize();
        static void Update(GLFWwindow* window);
        static bool WasAutoPaused();

    private:
        static void CheckMinimize(GLFWwindow* window);
        static void CheckFocus(GLFWwindow* window);
        static void CheckFullscreenToggle(GLFWwindow* window);

        static void AutoPause(const char* reason);
        static void AutoResume();
        static void ToggleFullscreen(GLFWwindow* window);

        // State tracking
        static bool s_initialized;
        static bool s_wasMinimized;
        static bool s_hadFocus;
        static bool s_wasFullscreen;
        static bool s_wasManuallyPaused;
        static bool s_wasAltEnterPressed;
        static bool s_isAutoPaused;  // NEW: Prevents duplicate pause/resume
    };

} // namespace Framework