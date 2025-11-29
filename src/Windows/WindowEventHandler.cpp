/**
===============================================================================
 File:          WindowEventHandler.cpp (FINAL - Input Reset Enabled)
 Author:        GE YONGQI
 Date:          2025-11-28

 This version has InputSystem::ResetAllKeyStates() enabled
===============================================================================
*/

#include "Precompiled.h"
#include "WindowEventHandler.h"
#include "GlobalPauseManager.h"
#include "Core.h"
#include "AudioSystem.h"   
#include "Input.h"      

namespace Framework {

    // ============================================================================
    // STATIC MEMBER INITIALIZATION
    // ============================================================================

    bool WindowEventHandler::s_initialized = false;
    bool WindowEventHandler::s_wasMinimized = false;
    bool WindowEventHandler::s_hadFocus = true;
    bool WindowEventHandler::s_wasFullscreen = false;
    bool WindowEventHandler::s_wasManuallyPaused = false;
    bool WindowEventHandler::s_wasAltEnterPressed = false;

    // ============================================================================
    // PUBLIC API
    // ============================================================================

    void WindowEventHandler::Initialize() {
        if (s_initialized) return;

        LOG_INFO("WindowEvents", "Initializing window event handler");
        LOG_INFO("WindowEvents", "TECH 1701: CTRL-ALT-DEL, minimize/restore handling enabled");
        LOG_INFO("WindowEvents", "TECH 1702: ALT-TAB, fullscreen toggle handling enabled");

        s_initialized = true;
    }

    void WindowEventHandler::Update(GLFWwindow* window) {
        if (!s_initialized) {
            Initialize();
        }

        if (!window) {
            LOG_ERROR("WindowEvents", "Null window pointer!");
            return;
        }

        CheckMinimize(window);
        CheckFocus(window);
        CheckFullscreenToggle(window);
    }

    bool WindowEventHandler::WasAutoPaused() {
        return !s_wasManuallyPaused && GlobalPause::IsPaused();
    }

    // ============================================================================
    // MINIMIZE/RESTORE HANDLING (TECH 1701)
    // ============================================================================

    void WindowEventHandler::CheckMinimize(GLFWwindow* window) {
        bool isMinimized = (glfwGetWindowAttrib(window, GLFW_ICONIFIED) == GLFW_TRUE);

        if (isMinimized && !s_wasMinimized) {
            LOG_INFO("WindowEvents", "========================================");
            LOG_INFO("WindowEvents", "TECH 1701: Window minimized");
            LOG_INFO("WindowEvents", "========================================");

            s_wasManuallyPaused = GlobalPause::IsPaused();

            if (!s_wasManuallyPaused) {
                AutoPause("Window minimized");
            }
            else {
                LOG_INFO("WindowEvents", "Game was already manually paused - preserving state");
            }
        }
        else if (!isMinimized && s_wasMinimized) {
            LOG_INFO("WindowEvents", "========================================");
            LOG_INFO("WindowEvents", "TECH 1701: Window restored");
            LOG_INFO("WindowEvents", "========================================");

            if (!s_wasManuallyPaused) {
                AutoResume();
            }
            else {
                LOG_INFO("WindowEvents", "Game was manually paused - keeping paused");
            }
        }

        s_wasMinimized = isMinimized;
    }

    // ============================================================================
    // FOCUS HANDLING (TECH 1701 - CTRL-ALT-DEL, etc.)
    // ============================================================================

    void WindowEventHandler::CheckFocus(GLFWwindow* window) {
        bool hasFocus = (glfwGetWindowAttrib(window, GLFW_FOCUSED) == GLFW_TRUE);

        if (!hasFocus && s_hadFocus) {
            LOG_INFO("WindowEvents", "========================================");
            LOG_INFO("WindowEvents", "TECH 1701: Window lost focus");
            LOG_INFO("WindowEvents", "  (Could be: CTRL-ALT-DEL, ALT-TAB, clicked outside, etc.)");
            LOG_INFO("WindowEvents", "========================================");

            s_wasManuallyPaused = GlobalPause::IsPaused();

            if (!s_wasManuallyPaused) {
                AutoPause("Focus lost");
            }
            else {
                LOG_INFO("WindowEvents", "Game was already manually paused - preserving state");
            }
        }
        else if (hasFocus && !s_hadFocus) {
            LOG_INFO("WindowEvents", "========================================");
            LOG_INFO("WindowEvents", "TECH 1701: Window gained focus");
            LOG_INFO("WindowEvents", "========================================");

            if (!s_wasManuallyPaused) {
                AutoResume();
            }
            else {
                LOG_INFO("WindowEvents", "Game was manually paused - keeping paused");
            }
        }

        s_hadFocus = hasFocus;
    }

    // ============================================================================
    // FULLSCREEN TOGGLE (TECH 1702)
    // ============================================================================

    void WindowEventHandler::CheckFullscreenToggle(GLFWwindow* window) {
        bool isAltPressed = glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_RIGHT_ALT) == GLFW_PRESS;
        bool isEnterPressed = glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS;
        bool isTabPressed = glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS;

        // ALT+ENTER: Toggle fullscreen
        if (isAltPressed && isEnterPressed && !s_wasAltEnterPressed) {
            LOG_INFO("WindowEvents", "========================================");
            LOG_INFO("WindowEvents", "TECH 1702: ALT+ENTER detected - toggling fullscreen");
            LOG_INFO("WindowEvents", "========================================");

            ToggleFullscreen(window);
            s_wasAltEnterPressed = true;
        }
        else if (!isEnterPressed) {
            s_wasAltEnterPressed = false;
        }

        // ALT+TAB: Auto-pause
        if (isAltPressed && isTabPressed) {
            if (!GlobalPause::IsPaused()) {
                LOG_INFO("WindowEvents", "========================================");
                LOG_INFO("WindowEvents", "TECH 1702: ALT+TAB detected - auto-pausing");
                LOG_INFO("WindowEvents", "========================================");

                s_wasManuallyPaused = false;
                AutoPause("ALT+TAB detected");
            }
        }
    }

    // ============================================================================
    // HELPER FUNCTIONS
    // ============================================================================

    void WindowEventHandler::AutoPause(const char* reason) {
        extern CoreEngine* CORE;

        LOG_INFO("WindowEvents", " Auto-pausing game: %s", reason);

        GlobalPause::SetPaused(true);

        // Mute audio
        if (CORE && CORE->GetAudioSystem()) {
            CORE->GetAudioSystem()->SetMasterVolume(0.0f);
            LOG_INFO("WindowEvents", "   Audio muted");
        }

        // ========================================================================
        // ENABLED: Reset input states (prevents stuck keys)
        // ========================================================================
        if (CORE && CORE->GetInputSystem()) {
            CORE->GetInputSystem()->ResetAllKeyStates();  //  NOW ENABLED!
            LOG_INFO("WindowEvents", "   Input states reset");
        }

        LOG_INFO("WindowEvents", "   Game paused successfully");
    }

    void WindowEventHandler::AutoResume() {
        extern CoreEngine* CORE;

        LOG_INFO("WindowEvents", " Auto-resuming game");

        GlobalPause::SetPaused(false);

        // Restore audio
        if (CORE && CORE->GetAudioSystem()) {
            CORE->GetAudioSystem()->SetMasterVolume(1.0f);
            LOG_INFO("WindowEvents", "   Audio restored");
        }

        // ========================================================================
        // ENABLED: Reset input states (prevents lingering presses)
        // ========================================================================
        if (CORE && CORE->GetInputSystem()) {
            CORE->GetInputSystem()->ResetAllKeyStates();  //  NOW ENABLED!
            LOG_INFO("WindowEvents", "   Input states reset");
        }

        LOG_INFO("WindowEvents", "   Game resumed successfully");
    }

    void WindowEventHandler::ToggleFullscreen(GLFWwindow* window) {
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        if (!monitor) {
            LOG_ERROR("WindowEvents", "Failed to get primary monitor!");
            return;
        }

        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        if (!mode) {
            LOG_ERROR("WindowEvents", "Failed to get video mode!");
            return;
        }

        bool isCurrentlyFullscreen = (glfwGetWindowMonitor(window) != nullptr);

        if (isCurrentlyFullscreen) {
            LOG_INFO("WindowEvents", "   Switching to WINDOWED mode (1280x720)");
            glfwSetWindowMonitor(window, nullptr, 100, 100, 1280, 720, GLFW_DONT_CARE);
        }
        else {
            LOG_INFO("WindowEvents", "   Switching to FULLSCREEN mode (%dx%d @ %dHz)",
                mode->width, mode->height, mode->refreshRate);
            glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
        }

        LOG_INFO("WindowEvents", "   Fullscreen toggle complete");
    }

} // namespace Framework