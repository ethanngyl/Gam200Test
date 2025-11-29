/*
===============================================================================
 File:          EditorModeManager.h
 Author:        GE YONGQI
 Email:         yongqi.ge@digipen.edu
 Date:          2025-11-28
 ------------------------------------------------------------------------------
  Global Editor Mode Manager

  Purpose:
    Provides a global state for "Editor Mode" which is toggled by F1 key.
    When editor mode is active:
    - ImGui editor panels are shown
    - Game functionality is disabled (movement, buttons, AI, etc.)
    - Game audio is muted (but ImGui UI sounds still work)
    - Rendering continues (to see current game state)

  Usage:
    EditorMode::Toggle();          // Toggle editor mode on/off
    EditorMode::SetEditorMode(true);   // Enable editor mode
    bool isEditor = EditorMode::IsEditorMode();  // Check current state

  Difference from IsPlaying():
    IsPlaying() = PLAY/STOP button in ImGui (gameplay vs level editing)
    EditorMode = F1 key (show/hide ImGui + freeze game)

  Priority:
    EditorMode > IsPlaying() > IsPaused()
    Editor mode has highest priority
===============================================================================
*/

#pragma once
#include "Precompiled.h"

namespace Framework {

    /**
     * @brief Global editor mode manager
     *
     * Controls whether the editor interface (ImGui) is active and
     * whether game functionality should be frozen.
     */
    class EditorMode {
    public:
        /**
         * @brief Check if editor mode is currently active
         * @return true if in editor mode, false otherwise
         */
        static bool IsEditorMode() {
            return editorModeActive;
        }

        /**
         * @brief Set editor mode state
         * @param active true to enable editor mode, false to disable
         */
        static void SetEditorMode(bool active) {
            if (editorModeActive == active) return;  // No change

            editorModeActive = active;
            LOG_INFO("EDITOR", "Editor mode: %s", active ? "ON" : "OFF");
        }

        /**
         * @brief Toggle editor mode (switch between on/off)
         */
        static void Toggle() {
            editorModeActive = !editorModeActive;
            LOG_INFO("EDITOR", "Editor mode toggled: %s",
                editorModeActive ? "ON" : "OFF");
        }

        /**
         * @brief Reset editor mode to default state (off)
         * Called when switching game states/levels
         */
        static void Reset() {
            editorModeActive = false;
            LOG_INFO("EDITOR", "Editor mode reset");
        }

    private:
        static inline bool editorModeActive = false;
    };

} // namespace Framework