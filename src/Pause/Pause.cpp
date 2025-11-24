/*
===============================================================================
  File:         Pause.cpp
  Author:       
  Co-Author:    
  Date:         2025-11-20
  Contribution: 
  Description:  Manages Game Pause state via Input and Window Events.
===============================================================================

  SYSTEM OVERVIEW
  ----------------
  This system handles both MANUAL pausing (User presses 'P') and AUTOMATIC
  pausing (Alt-Tab, Minimize, Focus Loss). It coordinates with the CoreEngine
  to halt gameplay/physics and mute audio, while keeping the Input system
  active to detect resume commands.

  KEY FEATURES
  ------------
  1. Auto-Pause: Triggered by GLFW Focus Loss (Alt-Tab) or Iconify (Minimize).
  2. Manual Pause: Triggered by keyboard input.
  3. Visuals: Renders a configurable UI overlay using GraphicsSystem.
  4. Audio: Mutes master volume on pause; restores it on resume.
  5. Scaling: UI adapts to window resolution changes via percentage coordinates.

  INTEGRATION GUIDE
  -----------------
  1. Core.cpp:
     - Initialize PauseSystem in `CreateAllSystems`.
     - Link dependencies via `SetCoreEngine`.
  2. Main Loop (main.cpp):
     - Check `!IsPaused()` before calling `UpdateSingleFrame`.
     - ALWAYS call `pauseSystem->Update()` (needs to run even when paused).
  3. Level Files (e.g., level1.cpp):
     - Call `engine->GetPauseSystem()->Draw()` at the end of the render loop
       to ensure the overlay appears on top of the game world.

  CONFIGURATION (valueloader.txt)
  -------------------------------
  Settings for the pause screen appearance:
  - Text Content: "pause_text", "pause_hint_manual", "pause_hint_focus"
  - Styling:      Font name, Scale, and RGB Color values.
  - Layout:       X/Y Percentages (0.0 to 1.0) for resolution independence.

  USAGE EXAMPLE
  -------------
  // Init
  pauseSystem = new PauseSystem();
  pauseSystem->SetCoreEngine(this);
  pauseSystem->Initialize();

  // Game Loop
  pauseSystem->Update(dt);
  if (!pauseSystem->IsPaused()) {
      engine->UpdateSingleFrame(dt);
  }

  // Render Loop
  pauseSystem->Draw();

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
    {
        LOG_INFO("PAUSE", "PauseSystem created");
        s_instance = this;
    }

    PauseSystem::~PauseSystem()
    {
        LOG_INFO("PAUSE", "PauseSystem destroyed");
        s_instance = nullptr;
    }

    void PauseSystem::Initialize()
    {
        LOG_INFO("PAUSE", "Initializing PauseSystem");

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
        LOG_INFO("PAUSE", "Registered window focus callback");

        glfwSetWindowIconifyCallback(window, WindowIconifyCallback);
        LOG_INFO("PAUSE", "Registered window iconify callback");

        glfwSetWindowUserPointer(window, this);

        LOG_INFO("PAUSE", "PauseSystem initialized");
    }

    void PauseSystem::Update(float dt)
    {
        DBG_SCOPE_SYS("Pause System", eng::debug::Subsystem::Engine);

        (void)dt;

        if (!coreEngine) {
            LOG_ERROR("PAUSE", "CoreEngine is null in Update");
            return;
        }

        auto* inputSystem = coreEngine->GetInputSystem();
        if (!inputSystem) {
            LOG_WARN("PAUSE", "InputSystem is null");
            return;
        }

        // Debug: Check if P is being pressed
        if (inputSystem->IsKeyDown(Framework::KEY_P)) {
            LOG_INFO("PAUSE", "P key is DOWN");
        }

        if (inputSystem->IsKeyPressed(Framework::KEY_P))
        {
            LOG_INFO("PAUSE", "P key PRESSED - isPaused=%d, reason=%d", isPaused, (int)pauseReason);

            if (isPaused && pauseReason == PauseReason::Manual) {
                LOG_INFO("PAUSE", "Calling Resume()");
                Resume();
            }
            else if (!isPaused) {
                LOG_INFO("PAUSE", "Calling Pause(Manual)");
                Pause(PauseReason::Manual);
            }
            else {
                LOG_WARN("PAUSE", "Paused but not manual (reason=%d), ignoring P press", (int)pauseReason);
            }
        }
    }

    void PauseSystem::Draw()
    {
        static bool hasLoggedThisPause = false;

        if (!isPaused) {
            hasLoggedThisPause = false;
            return;
        }

        auto* graphics = coreEngine->GetGraphicsSystem();
        if (!graphics) {
            LOG_ERROR("PAUSE", "GraphicsSystem is null");
            return;
        }

        auto* windowSystem = coreEngine->GetWindowSystem();
        if (!windowSystem) {
            LOG_ERROR("PAUSE", "WindowSystem is null");
            return;
        }

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

        if (!hasLoggedThisPause) {
            LOG_INFO("PAUSE", "Pause overlay at (%.1f, %.1f) - Window: %dx%d",
                centerX, centerY, windowWidth, windowHeight);
            hasLoggedThisPause = true;
        }

        float pauseTextScale = ConfigReader::GetFloat("pause_text_scale", 2.0f);
        float hintTextScale = ConfigReader::GetFloat("pause_hint_scale", 1.0f);

        float pauseColorR = ConfigReader::GetFloat("pause_text_color_r", 1.0f);
        float pauseColorG = ConfigReader::GetFloat("pause_text_color_g", 1.0f);
        float pauseColorB = ConfigReader::GetFloat("pause_text_color_b", 1.0f);

        float hintColorR = ConfigReader::GetFloat("pause_hint_color_r", 0.8f);
        float hintColorG = ConfigReader::GetFloat("pause_hint_color_g", 0.8f);
        float hintColorB = ConfigReader::GetFloat("pause_hint_color_b", 0.8f);

        const char* resumeHint = "";
        if (pauseReason == PauseReason::Manual) {
            resumeHint = resumeHintManual.c_str();
        }
        else if (pauseReason == PauseReason::WindowFocus) {
            resumeHint = resumeHintFocus.c_str();
        }

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
            LOG_INFO("PAUSE", "Received quit message");
            if (isPaused) {
                Resume();
            }
        }
    }

    void PauseSystem::Pause(PauseReason reason)
    {
        if (isPaused) {
            LOG_WARN("PAUSE", "Already paused");
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
            LOG_WARN("PAUSE", "Not paused");
            return;
        }

        if (pauseReason == PauseReason::WindowFocus ||
            pauseReason == PauseReason::TaskManager) {
            LOG_WARN("PAUSE", "Cannot manually resume from window focus pause");
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
            LOG_ERROR("PAUSE", "PauseSystem not found in GLFW user pointer");
            return;
        }

        double currentTime = glfwGetTime();

        if (focused) {
            LOG_INFO("PAUSE", "Window gained focus");

            pauseSystem->windowHasFocus = true;
            pauseSystem->focusGainedTime = currentTime;

            double outOfFocusTime = currentTime - pauseSystem->focusLostTime;
            LOG_INFO("PAUSE", "Window was out of focus for %.2f seconds", outOfFocusTime);

            if (pauseSystem->isPaused &&
                (pauseSystem->pauseReason == PauseReason::WindowFocus ||
                    pauseSystem->pauseReason == PauseReason::TaskManager))
            {
                pauseSystem->isPaused = false;
                pauseSystem->pauseReason = PauseReason::None;
                pauseSystem->ResumeAllSystems();

                LOG_INFO("PAUSE", "Game resumed");
            }
        }
        else {
            LOG_INFO("PAUSE", "Window lost focus");

            pauseSystem->windowHasFocus = false;
            pauseSystem->focusLostTime = currentTime;

            if (!pauseSystem->isPaused) {
                pauseSystem->isPaused = true;
                pauseSystem->pauseReason = PauseReason::WindowFocus;
                pauseSystem->PauseAllSystems();

                LOG_INFO("PAUSE", "Game paused");
            }
        }
    }

    void PauseSystem::WindowIconifyCallback(GLFWwindow* window, int iconified)
    {
        PauseSystem* pauseSystem = static_cast<PauseSystem*>(
            glfwGetWindowUserPointer(window));

        if (!pauseSystem) {
            LOG_ERROR("PAUSE", "PauseSystem not found in GLFW user pointer");
            return;
        }

        if (iconified) {
            LOG_INFO("PAUSE", "Window minimized");

            pauseSystem->windowIsMinimized = true;

            if (!pauseSystem->isPaused) {
                pauseSystem->isPaused = true;
                pauseSystem->pauseReason = PauseReason::WindowFocus;
                pauseSystem->PauseAllSystems();
            }
        }
        else {
            LOG_INFO("PAUSE", "Window restored");

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
        LOG_INFO("PAUSE", "Pausing all systems");

        if (!coreEngine) {
            LOG_ERROR("PAUSE", "CoreEngine is null");
            return;
        }

        PauseAudio();
        coreEngine->SetPlaying(false);

        LOG_INFO("PAUSE", "All systems paused");
    }

    void PauseSystem::ResumeAllSystems()
    {
        LOG_INFO("PAUSE", "Resuming all systems");

        if (!coreEngine) {
            LOG_ERROR("PAUSE", "CoreEngine is null");
            return;
        }

        ResumeAudio();
        coreEngine->SetPlaying(true);

        LOG_INFO("PAUSE", "All systems resumed");
    }

    void PauseSystem::PauseAudio()
    {
        auto* audioSystem = coreEngine->GetAudioSystem();
        if (!audioSystem) {
            LOG_WARN("PAUSE", "AudioSystem is null, skipping audio pause");
            return;
        }

        savedMasterVolume = 1.0f;
        audioSystem->SetMasterVolume(0.0f);
        audioWasPlaying = true;

        LOG_INFO("PAUSE", "Audio paused");
    }

    void PauseSystem::ResumeAudio()
    {
        auto* audioSystem = coreEngine->GetAudioSystem();
        if (!audioSystem) {
            LOG_WARN("PAUSE", "AudioSystem is null, skipping audio resume");
            return;
        }

        if (audioWasPlaying) {
            audioSystem->SetMasterVolume(savedMasterVolume);
            audioWasPlaying = false;

            LOG_INFO("PAUSE", "Audio resumed");
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

            LOG_INFO("PAUSE", "PAUSED - Reason: %s", reasonStr);
        }
        else {
            LOG_INFO("PAUSE", "RESUMED");
        }
    }

} // namespace Framework