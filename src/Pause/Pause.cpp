/*
===============================================================================
File:         Pause.cpp
Author:       Padilla Carl Jameson Z
Email:        c.padilla@digipen.edu
Date:         2025-11-20
Contribution:
------------------------------------------------------------------------------
Pause System for ALT-TAB and CTRL-ALT-DEL handling (Requirements 1701/1702)

This is the production version with minimal logging.
===============================================================================
*/

#include "Precompiled.h"
#include "Pause.h"
#include "Core.h"
#include "Audio/AudioSystem.h"
#include "WindowSystem.h"

namespace Framework
{
    PauseSystem* PauseSystem::s_instance = nullptr;

    PauseSystem::PauseSystem()
        : isPaused(false)
        , pauseReason(PauseReason::None)
        , windowHasFocus(true)
        , windowIsMinimized(false)
        , savedMasterVolume(1.0f)
        , audioWasPlaying(false)
        , focusLostTime(0.0)
        , focusGainedTime(0.0)
        , coreEngine(nullptr)
        , wasKeyPressed(false)
    {
        s_instance = this;
    }

    PauseSystem::~PauseSystem()
    {
        s_instance = nullptr;
    }

    void PauseSystem::Initialize()
    {
        if (!coreEngine) {
            LOG_ERROR("PAUSE", "CoreEngine not set");
            return;
        }

        auto* windowSystem = coreEngine->GetWindowSystem();
        if (!windowSystem) {
            LOG_ERROR("PAUSE", "WindowSystem is null");
            return;
        }

        GLFWwindow* window = windowSystem->GetWindow();
        if (!window) {
            LOG_ERROR("PAUSE", "GLFW window is null");
            return;
        }

        glfwSetWindowFocusCallback(window, WindowFocusCallback);
        glfwSetWindowIconifyCallback(window, WindowIconifyCallback);
        glfwSetWindowUserPointer(window, this);
    }

    // ===============================================================================
    // *** NEW: Helper function to check if pause is allowed ***
    // ===============================================================================
    bool PauseSystem::IsPauseAllowedInCurrentState()
    {
        // Get current game state from extern variable
        extern int current;

        // Only allow pause in actual gameplay levels
        // Disable in menus (mainMenu, levelSelect)
        switch (current)
        {
        case mainMenu:      // Main menu
        case Level_select:   // Level select menu
            return false;   // Pause DISABLED in menus

        case LEVEL_1:        // Level 1
        case LEVEL_2:        // Level 2
        case LEVEL_3:        // Level 3
            return true;    // Pause ENABLED in gameplay

        default:
            return true;    // Default: allow pause
        }
    }

    void PauseSystem::Update(float dt)
    {
        DBG_SCOPE_SYS("Pause System", eng::debug::Subsystem::Engine);

        (void)dt;

        if (!coreEngine) {
            return;
        }

        auto* inputSystem = coreEngine->GetInputSystem();
        if (!inputSystem) {
            return;
        }

        // ===============================================================================
        // Key Debouncing - Edge Detection
        // ===============================================================================
        bool isKeyDown = inputSystem->IsKeyDown(Framework::KEY_P);

        // Detect rising edge (key just pressed)
        bool keyJustPressed = isKeyDown && !wasKeyPressed;

        // Update key state for next frame
        wasKeyPressed = isKeyDown;

        // ===============================================================================
        // Check if pause is allowed before toggling ***
        // ===============================================================================
        if (keyJustPressed && IsPauseAllowedInCurrentState())
        {
            if (isPaused && pauseReason == PauseReason::Manual) {
                Resume();
            }
            else if (!isPaused) {
                Pause(PauseReason::Manual);
            }
        }
    }

    void PauseSystem::Draw()
    {
        if (!isPaused) {
            return;
        }

        auto* graphics = coreEngine->GetGraphicsSystem();
        if (!graphics) {
            return;
        }

        auto* windowSystem = coreEngine->GetWindowSystem();
        if (!windowSystem) {
            return;
        }

        // Load configuration
        std::string pauseText = ConfigReader::GetString("pause_text", "PAUSED");
        std::string resumeHintManual = ConfigReader::GetString("pause_hint_manual", "Press P to Resume");
        std::string resumeHintFocus = ConfigReader::GetString("pause_hint_focus", "Return to window to resume");
        std::string fontName = ConfigReader::GetString("pause_font", "Sans48");

        int windowWidth = windowSystem->GetWidth();
        int windowHeight = windowSystem->GetHeight();

        float centerXPercent = ConfigReader::GetFloat("pause_text_x_percent", 0.5f);
        float centerYPercent = ConfigReader::GetFloat("pause_text_y_percent", 0.5f);
        float hintOffsetYPercent = ConfigReader::GetFloat("pause_hint_offset_y_percent", 0.1f);
        float hintOffsetXPercent = ConfigReader::GetFloat("pause_hint_offset_x_percent", 0.1f);

        float centerX = windowWidth * centerXPercent;
        float centerY = windowHeight * centerYPercent;
        float hintOffsetY = windowHeight * hintOffsetYPercent;
        float hintOffsetX = windowWidth * hintOffsetXPercent;

        float pauseTextScale = ConfigReader::GetFloat("pause_text_scale", 2.0f);
        float hintTextScale = ConfigReader::GetFloat("pause_hint_scale", 1.0f);

        float pauseColorR = ConfigReader::GetFloat("pause_text_color_r", 1.0f);
        float pauseColorG = ConfigReader::GetFloat("pause_text_color_g", 1.0f);
        float pauseColorB = ConfigReader::GetFloat("pause_text_color_b", 1.0f);

        float hintColorR = ConfigReader::GetFloat("pause_hint_color_r", 0.8f);
        float hintColorG = ConfigReader::GetFloat("pause_hint_color_g", 0.8f);
        float hintColorB = ConfigReader::GetFloat("pause_hint_color_b", 0.8f);

        // Select appropriate hint text
        const char* resumeHint = "";
        if (pauseReason == PauseReason::Manual) {
            resumeHint = resumeHintManual.c_str();
        }
        else if (pauseReason == PauseReason::WindowFocus) {
            resumeHint = resumeHintFocus.c_str();
        }

        // Draw pause overlay
        graphics->DrawText4(fontName, pauseText, centerX, centerY, pauseTextScale,
            glm::vec3(pauseColorR, pauseColorG, pauseColorB));

        if (resumeHint[0] != '\0') {
            graphics->DrawText4(fontName, resumeHint, centerX - hintOffsetX, centerY - hintOffsetY,
                hintTextScale, glm::vec3(hintColorR, hintColorG, hintColorB));
        }
    }

    void PauseSystem::SendEngineMessage(Message* message)
    {
        if (message->MessageId == Status::Quit) {
            if (isPaused) {
                Resume();
            }
        }
    }

    void PauseSystem::Pause(PauseReason reason)
    {
        if (isPaused) {
            return;
        }

        isPaused = true;
        pauseReason = reason;

        LogPauseState(true, reason);
        PauseAllSystems();
    }

    void PauseSystem::Resume()
    {
        if (!isPaused) {
            return;
        }

        // Cannot manually resume from window focus pause
        if (pauseReason == PauseReason::WindowFocus ||
            pauseReason == PauseReason::TaskManager) {
            return;
        }

        isPaused = false;
        pauseReason = PauseReason::None;

        LogPauseState(false, PauseReason::None);
        ResumeAllSystems();
    }

    void PauseSystem::WindowFocusCallback(GLFWwindow* window, int focused)
    {
        PauseSystem* pauseSystem = static_cast<PauseSystem*>(
            glfwGetWindowUserPointer(window));

        if (!pauseSystem) {
            return;
        }

        // ===============================================================================
        // Only auto-pause on focus loss if in gameplay
        // ===============================================================================
        if (!pauseSystem->IsPauseAllowedInCurrentState()) {
            return;  // Don't auto-pause in menus
        }

        double currentTime = glfwGetTime();

        if (focused) {
            pauseSystem->windowHasFocus = true;
            pauseSystem->focusGainedTime = currentTime;

            // Resume if paused due to focus loss
            if (pauseSystem->isPaused &&
                (pauseSystem->pauseReason == PauseReason::WindowFocus ||
                    pauseSystem->pauseReason == PauseReason::TaskManager))
            {
                pauseSystem->isPaused = false;
                pauseSystem->pauseReason = PauseReason::None;
                pauseSystem->ResumeAllSystems();
            }
        }
        else {
            pauseSystem->windowHasFocus = false;
            pauseSystem->focusLostTime = currentTime;

            // Pause if not already paused
            if (!pauseSystem->isPaused) {
                pauseSystem->isPaused = true;
                pauseSystem->pauseReason = PauseReason::WindowFocus;
                pauseSystem->PauseAllSystems();
            }
        }
    }

    void PauseSystem::WindowIconifyCallback(GLFWwindow* window, int iconified)
    {
        PauseSystem* pauseSystem = static_cast<PauseSystem*>(
            glfwGetWindowUserPointer(window));

        if (!pauseSystem) {
            return;
        }

        // ===============================================================================
        // Only auto-pause on minimize if in gameplay
        // ===============================================================================
        if (!pauseSystem->IsPauseAllowedInCurrentState()) {
            return;  // Don't auto-pause in menus
        }

        if (iconified) {
            pauseSystem->windowIsMinimized = true;

            if (!pauseSystem->isPaused) {
                pauseSystem->isPaused = true;
                pauseSystem->pauseReason = PauseReason::WindowFocus;
                pauseSystem->PauseAllSystems();
            }
        }
        else {
            pauseSystem->windowIsMinimized = false;

            if (pauseSystem->isPaused &&
                pauseSystem->pauseReason == PauseReason::WindowFocus)
            {
                pauseSystem->isPaused = false;
                pauseSystem->pauseReason = PauseReason::None;
                pauseSystem->ResumeAllSystems();
            }
        }
    }

    void PauseSystem::PauseAllSystems()
    {
        if (!coreEngine) {
            return;
        }

        PauseAudio();
    }

    void PauseSystem::ResumeAllSystems()
    {
        if (!coreEngine) {
            return;
        }

        ResumeAudio();
    }

    void PauseSystem::PauseAudio()
    {
        auto* audioSystem = coreEngine->GetAudioSystem();
        if (!audioSystem) {
            return;
        }

        savedMasterVolume = 1.0f;
        audioSystem->SetMasterVolume(0.0f);
        audioWasPlaying = true;
    }

    void PauseSystem::ResumeAudio()
    {
        auto* audioSystem = coreEngine->GetAudioSystem();
        if (!audioSystem) {
            return;
        }

        if (audioWasPlaying) {
            audioSystem->SetMasterVolume(savedMasterVolume);
            audioWasPlaying = false;
        }
    }

    void PauseSystem::LogPauseState(bool pausing, PauseReason reason)
    {
        if (pausing) {
            const char* reasonStr = "Unknown";
            switch (reason) {
            case PauseReason::None: reasonStr = "None"; break;
            case PauseReason::WindowFocus: reasonStr = "Window Focus Loss"; break;
            case PauseReason::Manual: reasonStr = "Manual"; break;
            case PauseReason::TaskManager: reasonStr = "Task Manager"; break;
            }

            LOG_INFO("PAUSE", "Game paused - Reason: %s", reasonStr);
        }
        else {
            LOG_INFO("PAUSE", "Game resumed");
        }
    }

} // namespace Framework