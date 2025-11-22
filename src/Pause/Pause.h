/*
===============================================================================
 File:          Pause.h (With Game State Check)
 Author:        Padilla Carl Jameson Z
 Email:         c.padilla@digipen.edu
 Date:          2025-11-20
 Contribution:
 ------------------------------------------------------------------------------
  Pause System for ALT-TAB and CTRL-ALT-DEL handling (Requirements 1701/1702)

  NEW: Added game state checking to disable pause in menus
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

        // Key debouncing state
        bool wasKeyPressed;

        // *** NEW: Helper to check if pause is allowed ***
        bool IsPauseAllowedInCurrentState();

        void PauseAllSystems();
        void ResumeAllSystems();
        void PauseAudio();
        void ResumeAudio();
        void LogPauseState(bool pausing, PauseReason reason);

        static PauseSystem* s_instance;
    };

} // namespace Framework