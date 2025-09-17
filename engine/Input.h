#pragma once
#include "Interface.h"
#include <map>

namespace Framework
{
    // Key codes for common keys
    enum KeyCode
    {
        KEY_UNKNOWN = 0,
        KEY_A = 'A', KEY_B = 'B', KEY_C = 'C', KEY_D = 'D',
        KEY_E = 'E', KEY_F = 'F', KEY_G = 'G', KEY_H = 'H',
        KEY_I = 'I', KEY_J = 'J', KEY_K = 'K', KEY_L = 'L',
        KEY_M = 'M', KEY_N = 'N', KEY_O = 'O', KEY_P = 'P',
        KEY_Q = 'Q', KEY_R = 'R', KEY_S = 'S', KEY_T = 'T',
        KEY_U = 'U', KEY_V = 'V', KEY_W = 'W', KEY_X = 'X',
        KEY_Y = 'Y', KEY_Z = 'Z',

        KEY_SPACE = ' ',
        KEY_ENTER = 13,
        KEY_ESCAPE = 27,

        // Arrow keys (Windows virtual key codes)
        KEY_LEFT = 37,
        KEY_UP = 38,
        KEY_RIGHT = 39,
        KEY_DOWN = 40
    };

    class InputSystem : public InterfaceSystem
    {
    public:
        InputSystem();
        virtual ~InputSystem();

        // ISystem interface
        virtual void Initialize() override;
        virtual void Update(float dt) override;
        virtual void SendMessage(Message* message) override;

        // Input query functions
        bool IsKeyDown(KeyCode key);
        bool IsKeyPressed(KeyCode key);  // True only on the frame key was pressed
        bool IsKeyReleased(KeyCode key); // True only on the frame key was released

    private:
        std::map<KeyCode, bool> CurrentKeys;     // Keys down this frame
        std::map<KeyCode, bool> PreviousKeys;    // Keys down last frame

        void UpdateKeyState(KeyCode key, bool isDown);
        bool GetAsyncKeyState(KeyCode key);      // Platform-specific key checking
    };
}