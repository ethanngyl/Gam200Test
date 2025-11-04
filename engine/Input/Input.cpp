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
#include "EntitySpawner.h"
#include "ConfigReader.h"
namespace Framework
{
    InputSystem::InputSystem()
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

    void InputSystem::Update(float dt)
    {
        (void)dt;
        // Store previous frame's key states
        PreviousKeys = CurrentKeys;

        // Movement keys
        UpdateKeyState(KEY_W, GetAsyncKeyState(KEY_W));
        UpdateKeyState(KEY_A, GetAsyncKeyState(KEY_A));
        UpdateKeyState(KEY_S, GetAsyncKeyState(KEY_S));
        UpdateKeyState(KEY_D, GetAsyncKeyState(KEY_D));

		//Arrow keys (ADDED THESE 4 LINES OF CODE RECENTLY BY CJ - DONT FORGET TO REMOVE THIS COMMENT)
        UpdateKeyState(KEY_UP, GetAsyncKeyState(KEY_UP));
        UpdateKeyState(KEY_DOWN, GetAsyncKeyState(KEY_DOWN));
        UpdateKeyState(KEY_LEFT, GetAsyncKeyState(KEY_LEFT));
        UpdateKeyState(KEY_RIGHT, GetAsyncKeyState(KEY_RIGHT));

        // System keys
        UpdateKeyState(KEY_Q, GetAsyncKeyState(KEY_Q));
        UpdateKeyState(KEY_ESCAPE, GetAsyncKeyState(KEY_ESCAPE));
        UpdateKeyState(KEY_SPACE, GetAsyncKeyState(KEY_SPACE));
        UpdateKeyState(KEY_ENTER, GetAsyncKeyState(KEY_ENTER));

        // Shooting/spawning keys (for PlayerController)
        UpdateKeyState(KEY_SHIFT, GetAsyncKeyState(KEY_SHIFT));
        UpdateKeyState(KEY_E, GetAsyncKeyState(KEY_E));
        UpdateKeyState(KEY_O, GetAsyncKeyState(KEY_O));  
        UpdateKeyState(KEY_R, GetAsyncKeyState(KEY_R));
        UpdateKeyState(KEY_T, GetAsyncKeyState(KEY_T));

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

        // Mouse buttons
        UpdateKeyState(MOUSE_LEFT, GetAsyncKeyState(MOUSE_LEFT));
        UpdateKeyState(MOUSE_RIGHT, GetAsyncKeyState(MOUSE_RIGHT));

        // Check for quit conditions
        if (IsKeyPressed(KEY_Q) || IsKeyPressed(KEY_ESCAPE))
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

    void InputSystem::GetMousePosition(float& x, float& y)
    {
#ifdef _WIN32
        POINT point;
        GetCursorPos(&point);
        x = static_cast<float>(point.x);
        y = static_cast<float>(point.y);
#endif
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
#ifdef _WIN32
        return (::GetAsyncKeyState(key) & 0x8000) != 0;
#else
        return false;
#endif
    }
}