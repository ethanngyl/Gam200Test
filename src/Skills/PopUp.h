/**
===============================================================================
 File:           PopUp.h
 Date:           2025-01-26
 ------------------------------------------------------------------------------

 IN-GAME POPUP SYSTEM

 A customizable popup box that renders during gameplay.

 ============================================================================
 CONFIGURATION:
 ============================================================================

 All default values are defined at the top of PopUp.cpp
 Edit those values to change the defaults without touching any logic.

 ============================================================================
 API USAGE:
 ============================================================================

 Visibility:
     PopUp::Show();
     PopUp::Hide();
     PopUp::Toggle();
     if (PopUp::IsVisible()) { }

 Size:
     PopUp::SetSize(width, height);
     float w = PopUp::GetWidth();
     float h = PopUp::GetHeight();

 Position:
     PopUp::SetPosition(x, y);
     PopUp::CenterOnScreen();
     float x = PopUp::GetPosX();
     float y = PopUp::GetPosY();

 Colors (RGBA 0.0 - 1.0):
     PopUp::SetBackgroundColor(r, g, b, a);
     PopUp::SetBorderColor(r, g, b, a);

 Border:
     PopUp::SetBorderThickness(thickness);

 Padding:
     PopUp::SetPadding(padding);

 Toggle Key:
     PopUp::SetToggleKey(KEY_F3);

 ============================================================================
 ADDING CONTENT:
 ============================================================================

 In PopUp.cpp, find "ADD YOUR CONTENT HERE" section.
 Use ImGui functions there.

===============================================================================
*/

#pragma once

namespace Framework {

    class PopUp {
    public:
        // === VISIBILITY ===
        static void Show();
        static void Hide();
        static void Toggle();
        static bool IsVisible();

        // === RENDER (call from main.cpp) ===
        static void Render();

        // === SIZE ===
        static void SetSize(float width, float height);
        static float GetWidth();
        static float GetHeight();

        // === POSITION ===
        static void SetPosition(float x, float y);
        static void CenterOnScreen();
        static float GetPosX();
        static float GetPosY();

        // === BACKGROUND COLOR (RGBA 0.0 - 1.0) ===
        static void SetBackgroundColor(float r, float g, float b, float a);

        // === BORDER COLOR (RGBA 0.0 - 1.0) ===
        static void SetBorderColor(float r, float g, float b, float a);

        // === BORDER THICKNESS ===
        static void SetBorderThickness(float thickness);

        // === PADDING ===
        static void SetPadding(float padding);

        // === TOGGLE KEY (use KeyCode values from Input.h, e.g., KEY_F3 = VK_F3) ===
        static void SetToggleKey(int key);

    private:
        // State
        static bool s_visible;
        static bool s_centered;

        // Dimensions
        static float s_width;
        static float s_height;

        // Position
        static float s_posX;
        static float s_posY;

        // Background color
        static float s_bgR;
        static float s_bgG;
        static float s_bgB;
        static float s_bgA;

        // Border color
        static float s_borderR;
        static float s_borderG;
        static float s_borderB;
        static float s_borderA;

        // Border thickness
        static float s_borderThickness;

        // Padding
        static float s_padding;

        // Toggle key (stored as int to avoid header dependency)
        static int s_toggleKey;
    };

} // namespace Framework