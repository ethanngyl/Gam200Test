/**
===============================================================================
 File:          WindowEventHandler.h
 Author:        Padilla Carl Jameson Z
 Date:          2025-11-28
 Contribution:  100%
 ------------------------------------------------------------------------------
  Unified Window Event Handling for TECH 1701 & 1702

  Requirements:
  - TECH 1701: CTRL-ALT-DEL handling, window minimize/restore, pause logic
  - TECH 1702: ALT-TAB handling, fullscreen toggle

  Features:
  - Auto-pause on window minimize
  - Auto-pause on focus loss (CTRL-ALT-DEL, ALT-TAB, etc.)
  - Auto-resume when window regains focus
  - Preserves manual pause state
  - Fullscreen toggle with ALT+ENTER
  - Input state reset to prevent stuck keys
===============================================================================
*/

#pragma once
#include "Precompiled.h"

// Forward declaration
struct GLFWwindow;

namespace Framework {

    /**
     * @class WindowEventHandler
     * @brief Handles window events for proper pause behavior
     *
     * Call Update() every frame BEFORE game update logic.
     * Automatically pauses/resumes game based on window state.
     */
    class WindowEventHandler {
    public:
        /**
         * @brief Update window event handling (call every frame)
         * @param window GLFW window pointer
         */
        static void Update(GLFWwindow* window);

        /**
         * @brief Initialize the handler (optional, called automatically on first Update)
         */
        static void Initialize();

        /**
         * @brief Check if game was auto-paused (not manually paused)
         * @return true if auto-paused, false if manually paused or not paused
         */
        static bool WasAutoPaused();

    private:
        // State tracking
        static bool s_initialized;
        static bool s_wasMinimized;
        static bool s_hadFocus;
        static bool s_wasFullscreen;
        static bool s_wasManuallyPaused;   // Was paused before window event
        static bool s_wasAltEnterPressed;  // For edge detection

        // Individual event handlers
        static void CheckMinimize(GLFWwindow* window);
        static void CheckFocus(GLFWwindow* window);
        static void CheckFullscreenToggle(GLFWwindow* window);

        // Pause/Resume helpers
        static void AutoPause(const char* reason);
        static void AutoResume();
        static void ToggleFullscreen(GLFWwindow* window);
    };

} // namespace Framework