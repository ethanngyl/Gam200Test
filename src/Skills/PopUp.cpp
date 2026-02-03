/**
===============================================================================
 File:           PopUp.cpp
 Author:         Padilla Carl Jameson Z.
 Email:          c.padilla@digipen.edu
 Date:           2026-01-26
 Contribution:   100%
 ------------------------------------------------------------------------------

  Brief:
  - Implements a configurable in-game popup window system using ImGui. The popup
    provides a customizable overlay that can be toggled during gameplay for
    displaying information or interactive UI elements.

  Key features:
  - Supports customizable size (width/height) and position (absolute or centered).
  - Configurable background and border colors with adjustable transparency.
  - Adjustable border thickness and internal padding.
  - Keyboard toggle functionality with configurable key binding (default: F3).
  - Automatic hiding when the editor ImGui system is active.
  - Static interface allowing global access without instance management.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
===============================================================================
 */

#include "PopUp.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "Core.h"
#include "ImguiSystem.h"
#include <iostream>

namespace Framework {

    // =========================================================================
    // DEFAULT CONFIGURATION
    // =========================================================================
    // Edit these values to change the popup's default appearance.
    // =========================================================================

    namespace PopUpDefaults {
        // Visibility
        constexpr bool VISIBLE = false;
        constexpr bool CENTERED = true;

        // Size
        constexpr float WIDTH = 400.0f;
        constexpr float HEIGHT = 300.0f;

        // Position (only used if CENTERED = false)
        constexpr float POS_X = 0.0f;
        constexpr float POS_Y = 0.0f;

        // Background color (RGBA)
        constexpr float BG_R = 0.1f;
        constexpr float BG_G = 0.1f;
        constexpr float BG_B = 0.1f;
        constexpr float BG_A = 0.95f;

        // Border color (RGBA)
        constexpr float BORDER_R = 0.4f;
        constexpr float BORDER_G = 0.4f;
        constexpr float BORDER_B = 0.4f;
        constexpr float BORDER_A = 1.0f;

        // Border thickness
        constexpr float BORDER_THICKNESS = 2.0f;

        // Padding inside the box
        constexpr float PADDING = 20.0f;

        // Toggle key (KEY_F3 = VK_F3 = 0x72 = 114)
        constexpr int TOGGLE_KEY = 0x72;
    }

    // =========================================================================
    // STATIC MEMBER INITIALIZATION (using defaults)
    // =========================================================================

    bool    PopUp::s_visible = PopUpDefaults::VISIBLE;
    bool    PopUp::s_centered = PopUpDefaults::CENTERED;

    float   PopUp::s_width = PopUpDefaults::WIDTH;
    float   PopUp::s_height = PopUpDefaults::HEIGHT;

    float   PopUp::s_posX = PopUpDefaults::POS_X;
    float   PopUp::s_posY = PopUpDefaults::POS_Y;

    float   PopUp::s_bgR = PopUpDefaults::BG_R;
    float   PopUp::s_bgG = PopUpDefaults::BG_G;
    float   PopUp::s_bgB = PopUpDefaults::BG_B;
    float   PopUp::s_bgA = PopUpDefaults::BG_A;

    float   PopUp::s_borderR = PopUpDefaults::BORDER_R;
    float   PopUp::s_borderG = PopUpDefaults::BORDER_G;
    float   PopUp::s_borderB = PopUpDefaults::BORDER_B;
    float   PopUp::s_borderA = PopUpDefaults::BORDER_A;

    float   PopUp::s_borderThickness = PopUpDefaults::BORDER_THICKNESS;
    float   PopUp::s_padding = PopUpDefaults::PADDING;

    int     PopUp::s_toggleKey = PopUpDefaults::TOGGLE_KEY;

    // =========================================================================
    // VISIBILITY
    // =========================================================================

    void PopUp::Show() {
        s_visible = true;
        std::cout << "[POPUP] Opened" << std::endl;
    }

    void PopUp::Hide() {
        s_visible = false;
        std::cout << "[POPUP] Closed" << std::endl;
    }

    void PopUp::Toggle() {
        s_visible = !s_visible;
        std::cout << "[POPUP] " << (s_visible ? "Opened" : "Closed") << std::endl;
    }

    bool PopUp::IsVisible() {
        return s_visible;
    }

    // =========================================================================
    // SIZE
    // =========================================================================

    void PopUp::SetSize(float width, float height) {
        s_width = width;
        s_height = height;
    }

    float PopUp::GetWidth() { return s_width; }
    float PopUp::GetHeight() { return s_height; }

    // =========================================================================
    // POSITION
    // =========================================================================

    void PopUp::SetPosition(float x, float y) {
        s_posX = x;
        s_posY = y;
        s_centered = false;
    }

    void PopUp::CenterOnScreen() {
        s_centered = true;
    }

    float PopUp::GetPosX() { return s_posX; }
    float PopUp::GetPosY() { return s_posY; }

    // =========================================================================
    // COLORS
    // =========================================================================

    void PopUp::SetBackgroundColor(float r, float g, float b, float a) {
        s_bgR = r; s_bgG = g; s_bgB = b; s_bgA = a;
    }

    void PopUp::SetBorderColor(float r, float g, float b, float a) {
        s_borderR = r; s_borderG = g; s_borderB = b; s_borderA = a;
    }

    // =========================================================================
    // BORDER & PADDING
    // =========================================================================

    void PopUp::SetBorderThickness(float thickness) {
        s_borderThickness = thickness;
    }

    void PopUp::SetPadding(float padding) {
        s_padding = padding;
    }

    // =========================================================================
    // TOGGLE KEY
    // =========================================================================

    void PopUp::SetToggleKey(int key) {
        s_toggleKey = key;
    }

    // =========================================================================
    // RENDER
    // =========================================================================

    void PopUp::Render() {
        if (!CORE) return;

        // Only render during gameplay (not when editor is open)
        if (CORE->GetImGuiSystem() && CORE->GetImGuiSystem()->IsEnabled()) {
            return;
        }

        // Handle toggle key
        InputSystem* input = CORE->GetInputSystem();
        if (input && input->IsKeyPressed(static_cast<KeyCode>(s_toggleKey))) {
            Toggle();
        }

        if (!s_visible) return;

        // Start ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Calculate position
        ImGuiIO& io = ImGui::GetIO();
        if (s_centered) {
            s_posX = (io.DisplaySize.x - s_width) * 0.5f;
            s_posY = (io.DisplaySize.y - s_height) * 0.5f;
        }

        // Set window properties
        ImGui::SetNextWindowPos(ImVec2(s_posX, s_posY), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(s_width, s_height), ImGuiCond_Always);

        // Window flags
        ImGuiWindowFlags flags =
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoSavedSettings;

        // Apply styles
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(s_bgR, s_bgG, s_bgB, s_bgA));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(s_borderR, s_borderG, s_borderB, s_borderA));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, s_borderThickness);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(s_padding, s_padding));

        // Begin window
        ImGui::Begin("##GamePopup", nullptr, flags);

        // =====================================================================
        // ADD YOUR CONTENT HERE
        // =====================================================================
        //
        // Example:
        //     ImGui::Text("Hello");
        //     if (ImGui::Button("Close")) { PopUp::Hide(); }
        //
        // =====================================================================



        // =====================================================================
        // END CONTENT
        // =====================================================================

        ImGui::End();

        // Restore styles
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(2);

        // End ImGui frame
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

} // namespace Framework