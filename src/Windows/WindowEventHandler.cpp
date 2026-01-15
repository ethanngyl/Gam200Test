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

        // Window event handler initialized

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
            // Window minimized - auto-pause if needed
            if (!s_isAutoPaused) {
                s_wasManuallyPaused = GlobalPause::IsPaused();
                if (!s_wasManuallyPaused) {
                    AutoPause("Window minimized");
                }
            }
        }
        else if (!isMinimized && s_wasMinimized) {
            // Window restored - auto-resume if needed
            if (!s_wasManuallyPaused) {
                AutoResume();
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
            // Window lost focus - auto-pause if needed
            if (!s_isAutoPaused) {
                s_wasManuallyPaused = GlobalPause::IsPaused();
                if (!s_wasManuallyPaused) {
                    AutoPause("Focus lost");
                }
            }
        }
        else if (hasFocus && !s_hadFocus) {
            // Window gained focus - auto-resume if needed
            if (!s_wasManuallyPaused) {
                AutoResume();
            }
        }

        s_hadFocus = hasFocus;
    }

    // ============================================================================
    // FULLSCREEN TOGGLE (TECH 1702)
    // ============================================================================

    void WindowEventHandler::CheckFullscreenToggle(GLFWwindow* window) {
        bool isCtrlPressed = glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS;
        bool isAltPressed = glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_RIGHT_ALT) == GLFW_PRESS;
        bool isEnterPressed = glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS;
        bool isTabPressed = glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS;

        // CTRL+ALT+ENTER or ALT+ENTER: Toggle fullscreen
        bool isFullscreenToggle = (isCtrlPressed && isAltPressed && isEnterPressed) || 
                                   (isAltPressed && isEnterPressed);
        
        if (isFullscreenToggle && !s_wasAltEnterPressed) {
            ToggleFullscreen(window);
            s_wasAltEnterPressed = true;
        }
        else if (!isEnterPressed) {
            s_wasAltEnterPressed = false;
        }

        // ALT+TAB: Auto-pause
        if (isAltPressed && isTabPressed) {
            if (!GlobalPause::IsPaused()) {
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
        }

        // Reset input states
        if (CORE && CORE->GetInputSystem()) {
            CORE->GetInputSystem()->ResetAllKeyStates();
        }

        // Game resumed
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
            glfwSetWindowMonitor(window, nullptr, 100, 100, 1280, 720, GLFW_DONT_CARE);
        }
        else {
            glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
        }
    }

} // namespace Framework