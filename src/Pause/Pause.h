/*
===============================================================================
 File:          Pause.h
 Author:        Padilla Carl Jameson Z
 Email:         c.padilla@digipen.edu
 Date:          2025-11-20
 Contribution:  
 ------------------------------------------------------------------------------
  Pause System for ALT-TAB and CTRL-ALT-DEL handling (Requirements 1701/1702)

  Responsibilities:
     - Detects window focus loss (ALT-TAB, CTRL-ALT-DEL)
     - Pauses gameplay logic, physics, audio, and input
     - Automatically resumes when window regains focus
     - Supports manual pause via ESC key (optional)
     - Handles window minimize/restore properly

  Implementation:
     - Uses GLFW window focus callbacks
     - Coordinates with all engine systems via CoreEngine
     - Maintains pause state across game state transitions

===============================================================================
*/

#pragma once
#include "Precompiled.h"

namespace Framework
{
    class CoreEngine;
    class AudioSystem;
    class WindowSystem;

    enum class PauseReason
    {
        None,
        WindowFocus,
        Manual,
        TaskManager
    };

    class PauseSystem : public EngineSystem
    {
    public:
        PauseSystem();
        ~PauseSystem() override;

        void Initialize() override;
        void Update(float dt) override;
        void Draw();
        void SendEngineMessage(Message* message) override;

        bool IsPaused() const { return isPaused; }
        PauseReason GetPauseReason() const { return pauseReason; }

        void Pause(PauseReason reason);
        void Resume();

        void SetCoreEngine(CoreEngine* engine) { coreEngine = engine; }

        static void WindowFocusCallback(GLFWwindow* window, int focused);
        static void WindowIconifyCallback(GLFWwindow* window, int iconified);

    private:
        bool isPaused;
        PauseReason pauseReason;
        bool windowHasFocus;
        bool windowIsMinimized;

        float savedMasterVolume;
        bool audioWasPlaying;

        double focusLostTime;
        double focusGainedTime;

        CoreEngine* coreEngine;

        void PauseAllSystems();
        void ResumeAllSystems();
        void PauseAudio();
        void ResumeAudio();
        void LogPauseState(bool pausing, PauseReason reason);

        static PauseSystem* s_instance;
    };

} // namespace Framework