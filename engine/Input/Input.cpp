/**
===============================================================================
 File:           Input.cpp
 Author:         Josh Ong
 Email:          josh.o@digipen.edu
 Date:           2025-09-22
 Contribution:   100%
 ------------------------------------------------------------------------------
 Implementation of the InputSystem class.

  Design notes:
  This file implements the core logic for input handling. The Update() method
  is called once per frame. It first copies the current key states into the
  previous key state buffer, then polls the operating system for the live
  state of all relevant keys, updating the current state buffer.

  The IsKeyPressed() and IsKeyReleased() functions work by comparing the
  current and previous state buffers to detect transitions. Platform-specific
  code, like the Windows GetAsyncKeyState() API call, is wrapped in a helper
  function to make the system easier to port.
===============================================================================
 */

#include "Precompiled.h"

namespace Framework
{
    /**
     * @brief Constructs the InputSystem
     *
     * Initializes empty key state maps.
     */
    InputSystem::InputSystem()
    {
    }

    /**
     * @brief Destructor for InputSystem
     */
    InputSystem::~InputSystem()
    {
    }

    /**
     * @brief Initializes the input system
     *
     * Prints initialization message and control instructions to console.
     */
    void InputSystem::Initialize()
    {
        std::cout << "InputSystem: Initializing...\n";
        std::cout << "InputSystem: Use WASD to move, ESC to quit\n";
    }

    /**
     * @brief Updates input state for the current frame
     * @param dt Delta time (unused)
     *
     * Performs the following operations:
     * 1. Copies current frame states to previous frame buffer
     * 2. Polls all registered keys using platform-specific APIs
     * 3. Detects quit conditions (Q or ESC key pressed)
     * 4. Outputs debug information for key presses
     *
     * @note This runs every frame before other systems update
     */
    void InputSystem::Update(float dt)
    {
        (void)dt;
        // Store previous frame's key states
        PreviousKeys = CurrentKeys;

        // Check all keys we care about
        UpdateKeyState(KEY_W, GetAsyncKeyState(KEY_W));
        UpdateKeyState(KEY_A, GetAsyncKeyState(KEY_A));
        UpdateKeyState(KEY_S, GetAsyncKeyState(KEY_S));
        UpdateKeyState(KEY_D, GetAsyncKeyState(KEY_D));
        UpdateKeyState(KEY_Q, GetAsyncKeyState(KEY_Q));
        UpdateKeyState(KEY_1, GetAsyncKeyState(KEY_1));
        UpdateKeyState(KEY_2, GetAsyncKeyState(KEY_2));
        UpdateKeyState(KEY_3, GetAsyncKeyState(KEY_3));
        UpdateKeyState(KEY_4, GetAsyncKeyState(KEY_4));
        UpdateKeyState(KEY_ESCAPE, GetAsyncKeyState(KEY_ESCAPE));
        UpdateKeyState(KEY_SPACE, GetAsyncKeyState(KEY_SPACE));
        UpdateKeyState(KEY_ENTER, GetAsyncKeyState(KEY_ENTER));
        UpdateKeyState(KEY_T, GetAsyncKeyState(KEY_T));
        UpdateKeyState(KEY_R, GetAsyncKeyState(KEY_R));
        UpdateKeyState(KEY_5, GetAsyncKeyState(KEY_5));
        UpdateKeyState(KEY_6, GetAsyncKeyState(KEY_6));
        UpdateKeyState(KEY_7, GetAsyncKeyState(KEY_7));
        UpdateKeyState(KEY_8, GetAsyncKeyState(KEY_8));
        UpdateKeyState(KEY_9, GetAsyncKeyState(KEY_9));
        UpdateKeyState(KEY_0, GetAsyncKeyState(KEY_0));
        UpdateKeyState(MOUSE_LEFT, GetAsyncKeyState(MOUSE_LEFT));
        UpdateKeyState(MOUSE_RIGHT, GetAsyncKeyState(MOUSE_RIGHT));


        // Check for quit conditions
        if (IsKeyPressed(KEY_Q) || IsKeyPressed(KEY_ESCAPE))
        {
            std::cout << "InputSystem: Quit key pressed!\n";
            Message quitMsg(Status::Quit);
            Framework::CORE->BroadcastMessage(&quitMsg);
        }

        //Movement and Input Testing

        if (IsKeyPressed(KEY_W)) std::cout << "W pressed - Move Up\n";
        if (IsKeyPressed(KEY_A)) std::cout << "A pressed - Move Left\n";
        if (IsKeyPressed(KEY_S)) std::cout << "S pressed - Move Down\n";
        if (IsKeyPressed(KEY_D)) std::cout << "D pressed - Move Right\n";
        if (IsKeyPressed(KEY_SPACE)) std::cout << "Key Space pressed\n";
        if (IsKeyPressed(KEY_1)) std::cout << "Key 1 pressed\n";
        if (IsKeyPressed(KEY_2)) std::cout << "Key 2 pressed\n";
        if (IsKeyPressed(KEY_3)) std::cout << "Key 3 pressed\n";
        if (IsKeyPressed(KEY_4)) std::cout << "Key 4 pressed\n";
        if (IsKeyPressed(MOUSE_LEFT)) std::cout << "Left Mouse Key Pressed\n";
        if (IsKeyPressed(MOUSE_RIGHT)) std::cout << "Right Mouse Key Pressed\n";

        if (IsKeyPressed(MOUSE_LEFT)) {
            float mouseX, mouseY;
            GetMousePosition(mouseX, mouseY);

            std::cout << "Left click at: " << mouseX << ", " << mouseY << "\n";
        }
        if (IsKeyDown(MOUSE_RIGHT)) {
            std::cout << "Right mouse button held down\n";
        }
    }

    /**
     * @brief Gets the current mouse cursor position
     * @param[out] x Mouse X coordinate in screen space
     * @param[out] y Mouse Y coordinate in screen space
     *
     * @note Windows-only implementation using GetCursorPos
     * @note Returns screen coordinates, not game world coordinates
     */
    void InputSystem::GetMousePosition(float& x, float& y)
    {
#ifdef _WIN32
        POINT point;
        GetCursorPos(&point);

        // Convert screen coordinates to your game coordinates
        // You'll need the window handle to convert properly
        x = static_cast<float>(point.x);
        y = static_cast<float>(point.y);
#endif
    }

    /**
     * @brief Handles engine messages
     * @param message Pointer to the message to process
     *
     * Currently only handles Quit messages for debug output.
     */
    void InputSystem::SendEngineMessage(Message* message)
    {
        // Handle messages sent to input system
        if (message->MessageId == Status::Quit)
        {
            std::cout << "InputSystem: Received quit message\n";
        }
    }

    /**
     * @brief Checks if a key is currently held down
     * @param key Key to check
     * @return True if the key is down this frame
     */
    bool InputSystem::IsKeyDown(KeyCode key)
    {
        auto it = CurrentKeys.find(key);
        return (it != CurrentKeys.end()) ? it->second : false;
    }

    /**
     * @brief Checks if a key was just pressed this frame
     * @param key Key to check
     * @return True only if the key is down this frame and was up last frame
     *
     * This provides frame-accurate "on press" detection for actions
     * that should only trigger once per key press (like jumping).
     */
    bool InputSystem::IsKeyPressed(KeyCode key)
    {
        // Key is pressed if it's down this frame but wasn't down last frame
        bool currentlyDown = IsKeyDown(key);
        auto it = PreviousKeys.find(key);
        bool wasDown = (it != PreviousKeys.end()) ? it->second : false;

        return currentlyDown && !wasDown;
    }

    /**
     * @brief Checks if a key was just released this frame
     * @param key Key to check
     * @return True only if the key is up this frame and was down last frame
     */
    bool InputSystem::IsKeyReleased(KeyCode key)
    {
        // Key is released if it was down last frame but isn't down this frame
        bool currentlyDown = IsKeyDown(key);
        auto it = PreviousKeys.find(key);
        bool wasDown = (it != PreviousKeys.end()) ? it->second : false;

        return !currentlyDown && wasDown;
    }

    /**
     * @brief Updates the current frame state for a specific key
     * @param key Key to update
     * @param isDown True if the key is currently pressed
     *
     * Stores the key state in the CurrentKeys map for this frame.
     */
    void InputSystem::UpdateKeyState(KeyCode key, bool isDown)
    {
        CurrentKeys[key] = isDown;
    }

    /**
     * @brief Platform-specific key state query
     * @param key Key to check
     * @return True if the key is currently pressed according to the OS
     *
     * Windows implementation uses GetAsyncKeyState to check the
     * high-order bit (0x8000) which indicates if the key is down.
     *
     * @note Returns false on non-Windows platforms (stub implementation)
     */
    bool InputSystem::GetAsyncKeyState(KeyCode key)
    {
        // Windows-specific key checking
#ifdef _WIN32
        return (::GetAsyncKeyState(key) & 0x8000) != 0;
#else
        // For other platforms, you'd implement different key checking
        return false;
#endif
    }
}