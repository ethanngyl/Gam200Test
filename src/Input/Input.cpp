/**
===============================================================================
 File:           Input.cpp (FIXED VERSION)
 Author:         Josh Ong (Modified by GE YONGQI)
 Email:          josh.o@digipen.edu
 Date:           2025-09-22
 Modification:   2025-11-06
 ------------------------------------------------------------------------------
  FIXED: GetMousePosition() now returns window-relative coordinates

 Changes:
 - Added window pointer to InputSystem
 - Changed GetCursorPos() to glfwGetCursorPos()
 - Now returns coordinates relative to window, not screen

 Copyright (C) 2026 DigiPen Institute of Technology.
 Reproduction or disclosure of this file or its contents
 without the prior written consent of DigiPen Institute of
 Technology is prohibited.
===============================================================================
 */

#include "Precompiled.h"
#include "EntitySpawner.h"
#include "ConfigReader.h"
namespace Framework
{
    InputSystem::InputSystem()
        : window(nullptr)  //  Initialize window pointer
    {
    }

    InputSystem::~InputSystem()
    {
    }

    void InputSystem::Initialize()
    {
        std::cout << "InputSystem: Initializing...\n";
        std::cout << "InputSystem: Use WASD to move, ESC to quit\n";
        std::cout << "InputSystem: SPACE to shoot, E to spawn enemy, O to spawn obstacle\n";
    }

    //  NEW: Set window pointer
    void InputSystem::SetWindow(GLFWwindow* win)
    {
        window = win;
        std::cout << "InputSystem: Window set\n";
    }

    void InputSystem::Update(float dt)
    {
        DBG_SCOPE_SYS("IO System", eng::debug::Subsystem::IO);

        (void)dt;
        // Store previous frame's key states
        PreviousKeys = CurrentKeys;

        // Movement keys
        UpdateKeyState(KEY_W, GetAsyncKeyState(KEY_W));
        UpdateKeyState(KEY_A, GetAsyncKeyState(KEY_A));
        UpdateKeyState(KEY_S, GetAsyncKeyState(KEY_S));
        UpdateKeyState(KEY_D, GetAsyncKeyState(KEY_D));

        //Arrow Keys: For player
        UpdateKeyState(KEY_UP, GetAsyncKeyState(KEY_UP));
        UpdateKeyState(KEY_DOWN, GetAsyncKeyState(KEY_DOWN));
        UpdateKeyState(KEY_LEFT, GetAsyncKeyState(KEY_LEFT));
        UpdateKeyState(KEY_RIGHT, GetAsyncKeyState(KEY_RIGHT));


        // System keys
        UpdateKeyState(KEY_Q, GetAsyncKeyState(KEY_Q));
        UpdateKeyState(KEY_ESCAPE, GetAsyncKeyState(KEY_ESCAPE));
        UpdateKeyState(KEY_SPACE, GetAsyncKeyState(KEY_SPACE));
        UpdateKeyState(KEY_ENTER, GetAsyncKeyState(KEY_ENTER));
		UpdateKeyState(KEY_DELETE, GetAsyncKeyState(KEY_DELETE));
        UpdateKeyState(KEY_Z, GetAsyncKeyState(KEY_Z));
        UpdateKeyState(KEY_LEFT_CONTROL, GetAsyncKeyState(KEY_LEFT_CONTROL));
        UpdateKeyState(KEY_RIGHT_CONTROL, GetAsyncKeyState(KEY_RIGHT_CONTROL));
        // Shooting/spawning keys (for PlayerController)
        UpdateKeyState(KEY_SHIFT, GetAsyncKeyState(KEY_SHIFT));
        UpdateKeyState(KEY_E, GetAsyncKeyState(KEY_E));
        UpdateKeyState(KEY_O, GetAsyncKeyState(KEY_O));
        UpdateKeyState(KEY_R, GetAsyncKeyState(KEY_R));
        UpdateKeyState(KEY_T, GetAsyncKeyState(KEY_T));
        UpdateKeyState(KEY_P, GetAsyncKeyState(KEY_P));
        UpdateKeyState(KEY_TAB, GetAsyncKeyState(KEY_TAB));

        UpdateKeyState(KEY_M, GetAsyncKeyState(KEY_M));
        UpdateKeyState(KEY_O, GetAsyncKeyState(KEY_O));
        UpdateKeyState(KEY_J, GetAsyncKeyState(KEY_J));
        UpdateKeyState(KEY_K, GetAsyncKeyState(KEY_K));
        UpdateKeyState(KEY_L, GetAsyncKeyState(KEY_L));

        UpdateKeyState(KEY_X, GetAsyncKeyState(KEY_X));
        UpdateKeyState(KEY_C, GetAsyncKeyState(KEY_C));
        UpdateKeyState(KEY_V, GetAsyncKeyState(KEY_V));
        UpdateKeyState(KEY_B, GetAsyncKeyState(KEY_B));
        UpdateKeyState(KEY_N, GetAsyncKeyState(KEY_N));

        // Number keys
        UpdateKeyState(KEY_1, GetAsyncKeyState(KEY_1));
        UpdateKeyState(KEY_2, GetAsyncKeyState(KEY_2));
        UpdateKeyState(KEY_3, GetAsyncKeyState(KEY_3));
        UpdateKeyState(KEY_4, GetAsyncKeyState(KEY_4));
        UpdateKeyState(KEY_5, GetAsyncKeyState(KEY_5));
        UpdateKeyState(KEY_6, GetAsyncKeyState(KEY_6));
        UpdateKeyState(KEY_7, GetAsyncKeyState(KEY_7));
        UpdateKeyState(KEY_8, GetAsyncKeyState(KEY_8));
        UpdateKeyState(KEY_9, GetAsyncKeyState(KEY_9));
        UpdateKeyState(KEY_0, GetAsyncKeyState(KEY_0));

        // Function keys 
        UpdateKeyState(KEY_F1, GetAsyncKeyState(KEY_F1));
        UpdateKeyState(KEY_F2, GetAsyncKeyState(KEY_F2));
        UpdateKeyState(KEY_F3, GetAsyncKeyState(KEY_F3));
        UpdateKeyState(KEY_F9, GetAsyncKeyState(KEY_F9)); 

        // Mouse buttons
        UpdateKeyState(MOUSE_LEFT, GetAsyncKeyState(MOUSE_LEFT));
        UpdateKeyState(MOUSE_RIGHT, GetAsyncKeyState(MOUSE_RIGHT));


        // Check for quit conditions
        if (IsKeyPressed(KEY_Q))
        {
            std::cout << "InputSystem: Quit key pressed!\n";
            Message quitMsg(Status::Quit);
            Framework::CORE->BroadcastMessage(&quitMsg);
        }

        // Movement debug output (optional - can remove if too verbose)
        if (IsKeyPressed(KEY_W)) std::cout << "W pressed - Move Up\n";
        if (IsKeyPressed(KEY_A)) std::cout << "A pressed - Move Left\n";
        if (IsKeyPressed(KEY_S)) std::cout << "S pressed - Move Down\n";
        if (IsKeyPressed(KEY_D)) std::cout << "D pressed - Move Right\n";

        // Mouse debug output
        if (IsKeyPressed(MOUSE_LEFT)) {
            float mouseX, mouseY;
            GetMousePosition(mouseX, mouseY);
            std::cout << "Left click at: " << mouseX << ", " << mouseY << "\n";
        }
        if (IsKeyDown(MOUSE_RIGHT)) {
            std::cout << "Right mouse button held down\n";
        }
    }

    //  FIXED: Now returns window-relative coordinates
    void InputSystem::GetMousePosition(float& x, float& y)
    {
        // Default values if window is not available
        x = 0.0f;
        y = 0.0f;

        if (!window) {
            return;
        }

        //  Use GLFW to get window-relative coordinates
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);

        x = static_cast<float>(xpos);
        y = static_cast<float>(ypos);

        // These coordinates are now:
        // - (0, 0) = top-left of window
        // - (windowWidth, windowHeight) = bottom-right of window
        // - Independent of window position on screen
    }

    void InputSystem::ResetAllKeyStates()
    {
        std::cout << "InputSystem: Resetting all key states\n";

        // Clear all current key states
        CurrentKeys.clear();

        // Clear all previous key states
        PreviousKeys.clear();

        std::cout << "InputSystem: All key states reset (CurrentKeys and PreviousKeys cleared)\n";
    }

    void InputSystem::SendEngineMessage(Message* message)
    {
        if (message->MessageId == Status::Quit)
        {
            std::cout << "InputSystem: Received quit message\n";
        }
    }

    bool InputSystem::IsKeyDown(KeyCode key)
    {
        auto it = CurrentKeys.find(key);
        return (it != CurrentKeys.end()) ? it->second : false;
    }

    bool InputSystem::IsKeyPressed(KeyCode key)
    {
        bool currentlyDown = IsKeyDown(key);
        auto it = PreviousKeys.find(key);
        bool wasDown = (it != PreviousKeys.end()) ? it->second : false;
        return currentlyDown && !wasDown;
    }

    bool InputSystem::IsKeyReleased(KeyCode key)
    {
        bool currentlyDown = IsKeyDown(key);
        auto it = PreviousKeys.find(key);
        bool wasDown = (it != PreviousKeys.end()) ? it->second : false;
        return !currentlyDown && wasDown;
    }

    void InputSystem::UpdateKeyState(KeyCode key, bool isDown)
    {
        CurrentKeys[key] = isDown;
    }

    bool InputSystem::GetAsyncKeyState(KeyCode key)
    {
        //this line is to let input system ignore any input when the
        // window is not focused
        //jiahao
        if (window && !glfwGetWindowAttrib(window, GLFW_FOCUSED)) {
            return false;
        }

#ifdef _WIN32
        return (::GetAsyncKeyState(key) & 0x8000) != 0;
#else
        return false;
#endif
    }
}