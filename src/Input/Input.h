/**
 ===============================================================================
 File:           Input.h (FIXED VERSION)
 Author:         Josh Ong (Modified by GE YONGQI)
 Email:          josh.o@digipen.edu
 Date:           2025-09-22
 Modification:   2025-11-06
 ------------------------------------------------------------------------------
  FIXED: Added window pointer and SetWindow() method

 Changes:
 - Added GLFWwindow* member variable
 - Added SetWindow() method
 - GetMousePosition() now returns window-relative coordinates

 Copyright (C) 2026 DigiPen Institute of Technology.
 Reproduction or disclosure of this file or its contents
 without the prior written consent of DigiPen Institute of
 Technology is prohibited.
===============================================================================
 */
#pragma once
#include "Precompiled.h"

 // Forward declare GLFWwindow
struct GLFWwindow;

namespace Framework
{
    /**
     * @enum KeyCode
     * @brief Key and mouse button identifiers
     *
     * Unified enum for both keyboard keys and mouse buttons.
     * Uses ASCII values for alphanumeric keys and Windows virtual key codes
     * for special keys and mouse buttons.
     */
    enum KeyCode
    {
        // Alphanumeric Keys
        KEY_UNKNOWN = 0,
        KEY_A = 'A', KEY_B = 'B', KEY_C = 'C', KEY_D = 'D',
        KEY_E = 'E', KEY_F = 'F', KEY_G = 'G', KEY_H = 'H',
        KEY_I = 'I', KEY_J = 'J', KEY_K = 'K', KEY_L = 'L',
        KEY_M = 'M', KEY_N = 'N', KEY_O = 'O', KEY_P = 'P',
        KEY_Q = 'Q', KEY_R = 'R', KEY_S = 'S', KEY_T = 'T',
        KEY_U = 'U', KEY_V = 'V', KEY_W = 'W', KEY_X = 'X',
        KEY_Y = 'Y', KEY_Z = 'Z', KEY_1 = '1', KEY_2 = '2',
        KEY_3 = '3', KEY_4 = '4', KEY_5 = '5', KEY_6 = '6',
        KEY_7 = '7', KEY_8 = '8', KEY_9 = '9', KEY_0 = '0',

        KEY_SPACE = ' ',
        KEY_ENTER = 13,
        KEY_ESCAPE = 27,

        // Arrow keys (Windows virtual key codes)
        KEY_LEFT = 37,
        KEY_UP = 38,
        KEY_RIGHT = 39,
        KEY_DOWN = 40,

        // Mouse Buttons
        MOUSE_LEFT = VK_LBUTTON,
        MOUSE_RIGHT = VK_RBUTTON,

        // Function keys
        KEY_F1 = VK_F1,
        KEY_F2 = VK_F2,
        KEY_F3 = VK_F3,
        KEY_F9 = VK_F9,

        //Delete key
		KEY_DELETE = VK_DELETE,

        //Shift Key
        KEY_SHIFT = VK_SHIFT,

        //Tab Key (for target cycling)
        KEY_TAB = VK_TAB,

        KEY_LEFT_CONTROL = VK_LCONTROL,
        KEY_RIGHT_CONTROL = VK_RCONTROL
    };

    /**
     * @class InputSystem
     * @brief Handles real-time keyboard and mouse input
     *
     * Provides three types of input queries:
     * - IsKeyDown: True while key is held (continuous)
     * - IsKeyPressed: True only on the frame the key was first pressed
     * - IsKeyReleased: True only on the frame the key was released
     *
     * Uses double-buffering (current + previous frame states) to detect
     * frame-accurate press and release events.
     *
     * Example usage:
     * @code
     * if (inputSystem->IsKeyPressed(KEY_SPACE)) {
     *     // Jump (fires once per press)
     * }
     * if (inputSystem->IsKeyDown(KEY_W)) {
     *     // Move forward (continuous while held)
     * }
     * @endcode
     */
    class InputSystem : public EngineSystem
    {
    public:
        /**
         * @brief Constructs the input system
         */
        InputSystem();

        /**
         * @brief Destructor
         */
        virtual ~InputSystem();

        /**
         * @brief Initializes the input system
         */
        virtual void Initialize() override;

        /**
         * @brief Updates input state and processes key events
         * @param dt Delta time since last frame (unused)
         *
         * Polls all registered keys and updates current/previous state buffers.
         */
        virtual void Update(float dt) override;

        /**
         * @brief Handles engine messages
         * @param message Message to process
         */
        virtual void SendEngineMessage(Message* message) override;

        /**
         * @brief Checks if a key is currently held down
         * @param key Key to check
         * @return True while the key is held down
         */
        bool IsKeyDown(KeyCode key);

        /**
         * @brief Checks if a key was just pressed this frame
         * @param key Key to check
         * @return True only on the frame the key transitioned from up to down
         */
        bool IsKeyPressed(KeyCode key);

        /**
         * @brief Checks if a key was just released this frame
         * @param key Key to check
         * @return True only on the frame the key transitioned from down to up
         */
        bool IsKeyReleased(KeyCode key);

        /**
         * @brief Gets the current mouse cursor position
         * @param[out] x Mouse X coordinate in WINDOW space (0 = left edge)
         * @param[out] y Mouse Y coordinate in WINDOW space (0 = top edge)
         *
         * IMPORTANT: Returns window-relative coordinates, not screen coordinates
         * - (0, 0) = top-left corner of the window
         * - (windowWidth, windowHeight) = bottom-right corner
         * - Independent of window position on screen
         */
        void GetMousePosition(float& x, float& y);

        /**
         * @brief Sets the GLFW window for mouse position queries
         * @param win GLFW window pointer
         *
         * MUST be called before GetMousePosition() will work correctly
         */
        void SetWindow(GLFWwindow* win);

        void ResetAllKeyStates();


    private:
        std::map<KeyCode, bool> CurrentKeys;     // Keys down this frame
        std::map<KeyCode, bool> PreviousKeys;    // Keys down last frame
        GLFWwindow* window;                      // NEW: Window pointer for mouse queries

        /**
         * @brief Updates the state of a specific key
         * @param key Key to update
         * @param isDown True if key is currently down
         */
        void UpdateKeyState(KeyCode key, bool isDown);

        /**
         * @brief Platform-specific key state query
         * @param key Key to check
         * @return True if key is currently pressed
         *
         * Uses GetAsyncKeyState on Windows. Would need platform-specific
         * implementations for other operating systems.
         */
        bool GetAsyncKeyState(KeyCode key);
    };
}