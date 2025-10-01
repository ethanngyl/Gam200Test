#include "Precompiled.h"

namespace Framework
{
    InputSystem::InputSystem()
    {
        // Constructor - nothing special needed
    }

    InputSystem::~InputSystem()
    {
        // Destructor - cleanup if needed
    }

    void InputSystem::Initialize()
    {
        std::cout << "InputSystem: Initializing...\n";
        std::cout << "InputSystem: Use WASD to move, ESC to quit\n";
    }

    void InputSystem::Update(float dt)
    {
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
    void InputSystem::SendEngineMessage(Message* message)
    {
        // Handle messages sent to input system
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
        // Key is pressed if it's down this frame but wasn't down last frame
        bool currentlyDown = IsKeyDown(key);
        auto it = PreviousKeys.find(key);
        bool wasDown = (it != PreviousKeys.end()) ? it->second : false;

        return currentlyDown && !wasDown;
    }

    bool InputSystem::IsKeyReleased(KeyCode key)
    {
        // Key is released if it was down last frame but isn't down this frame
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
        // Windows-specific key checking
#ifdef _WIN32
        return (::GetAsyncKeyState(key) & 0x8000) != 0;
#else
        // For other platforms, you'd implement different key checking
        return false;
#endif
    }
}