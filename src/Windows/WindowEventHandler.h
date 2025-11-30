/**
===============================================================================
 File:          WindowEventHandler.h
 Author:        Padilla Carl Jameson Z
 Date:          2025-11-28
 Contribution:  100%

 Description:
 Header file for window event handler system.
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