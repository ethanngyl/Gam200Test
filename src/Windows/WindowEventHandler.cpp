/**
===============================================================================
 File:          WindowEventHandler.cpp (FIXED - Audio Restoration Working)
 Author:        Padilla Carl Jameson Z
 Date:          2025-11-30

 FIXED: s_wasManuallyPaused is now only checked when NOT already auto-paused
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
    bool WindowEventHandler::s_isAutoPaused = false;

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
        return s_isAutoPaused;
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

            // FIXED: Only check if manually paused when NOT already auto-paused
            if (!s_isAutoPaused) {
                s_wasManuallyPaused = GlobalPause::IsPaused();

                if (!s_wasManuallyPaused) {
                    AutoPause("Window minimized");
                }
                else {
                    LOG_INFO("WindowEvents", "Game was already manually paused - preserving state");
                }
            }
            else {
                LOG_INFO("WindowEvents", "Already auto-paused, skipping");
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

            // FIXED: Only check if manually paused when NOT already auto-paused
            if (!s_isAutoPaused) {
                s_wasManuallyPaused = GlobalPause::IsPaused();

                if (!s_wasManuallyPaused) {
                    AutoPause("Focus lost");
                }
                else {
                    LOG_INFO("WindowEvents", "Game was already manually paused - preserving state");
                }
            }
            else {
                LOG_INFO("WindowEvents", "Already auto-paused, skipping");
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
        if (s_isAutoPaused) {
            LOG_INFO("WindowEvents", "Already auto-paused, skipping duplicate pause");
            return;
        }

        extern CoreEngine* CORE;

        LOG_INFO("WindowEvents", " Auto-pausing game: %s", reason);

        s_isAutoPaused = true;
        GlobalPause::SetPaused(true);

        // Mute audio
        if (CORE && CORE->GetAudioSystem()) {
            CORE->GetAudioSystem()->SetMasterVolume(0.0f);
            LOG_INFO("WindowEvents", "   Audio muted");
        }

        // Reset input states
        if (CORE && CORE->GetInputSystem()) {
            CORE->GetInputSystem()->ResetAllKeyStates();
            LOG_INFO("WindowEvents", "   Input states reset");
        }

        LOG_INFO("WindowEvents", "   Game paused successfully");
    }

    void WindowEventHandler::AutoResume() {
        if (!s_isAutoPaused) {
            LOG_INFO("WindowEvents", "Not auto-paused, skipping resume");
            return;
        }

        extern CoreEngine* CORE;

        LOG_INFO("WindowEvents", " Auto-resuming game");

        s_isAutoPaused = false;
        GlobalPause::SetPaused(false);

        // Restore audio
        if (CORE && CORE->GetAudioSystem()) {
            CORE->GetAudioSystem()->SetMasterVolume(1.0f);
            LOG_INFO("WindowEvents", "   Audio restored");
        }

        // Reset input states
        if (CORE && CORE->GetInputSystem()) {
            CORE->GetInputSystem()->ResetAllKeyStates();
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