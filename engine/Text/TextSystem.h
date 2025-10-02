/*
===============================================================================
 File:          TextSystem.h
 Author:        PADILLA CARL JAMESON Z.
 Email:         c.padilla@digipen.edu
 Date:          2025-09-30
 Contribution:  100%
 ------------------------------------------------------------------------------
 Implementation of TextSystem header files

 Responsibilities:
   - Declare the TextSystem class as part of the engine’s InterfaceSystem layer.
   - Manage text boxes (position, width, style, word-wrapping, borders, padding).
   - Provide font registration and active font management (manual or auto-load).
   - Support dialogue queue operations: enqueue, advance, and clear.
   - Expose public API functions for engine/game to interact with text UI.

 Platform notes:
   - Runs on Windows console; overlay output uses Win32 cursor positioning APIs.
   - Default mode avoids cursor repositioning and simply appends to console logs.
   - Portable to non-Windows platforms via standard console output (std::cout).

 Safety:
   - Public API functions do not perform direct console I/O except during Update().
   - All API calls return status codes (true/false) instead of throwing exceptions.
   - Dialogue and font containers use STL types with safe bounds/optionals.
   - Assumes .ttf/.otf fonts exist in a Fonts/ directory; no runtime validation
     beyond registration success.
===============================================================================
*/

#pragma once

#include "Interface.h"
#include "Message.h"
#include "Precompiled.h"

#include <string>
#include <vector>
#include <unordered_map>
#include <deque>
#include <optional>
#include <filesystem>

namespace Framework {

    struct TextStyle {
        std::string fontName;     // Logical font name (must be loaded)
        char border = '#';        // Box border character
        int  padding = 1;         // Inner padding (spaces)
    };

    struct TextBox {
        int x = 0;                // Console column (0-based)
        int y = 0;                // Console row    (0-based)
        int width = 40;           // Inner text width (content area)
        std::string text;         // Raw content (will be wrapped)
        TextStyle style;

        bool dirty = true;        // Reprint when changed
    };

    class TextSystem : public InterfaceSystem
    {
    public:
        TextSystem();            // Initialize the system           
        ~TextSystem() override;  // Clean up resources

        // InterfaceSystem
        void Initialize() override;                         // Called once to set up system
        void Update(float dt) override;                     // Per-frame update
        void SendEngineMessage(Message* message) override;  // Handle engine messages

        // --- Public API (engine/game can call these later) ---
        bool LoadFont(std::string const& logicalName, std::string const& ttfPath); // Register font alias -> file path

        /*! \brief Set the active font by alias.
            \return true on success.
            \note No console I/O; return status code. */
        bool SetActiveFont(std::string const& logicalName);

        std::optional<std::string> GetActiveFont() const; // Get the current active font alias

        size_t AddTextBox(TextBox const& box);                    // Create a new text box

        bool   SetText(size_t handle, std::string const& text);   // Change text of a box
            
        bool   MoveBox(size_t handle, int x, int y);              // Move box to new position

        bool   SetWidth(size_t handle, int width);                // Change wrap width of a box

        bool   SetStyle(size_t handle, TextStyle const& style);   // Apply a new style to a box

        void   DialogueClear(); // Clear dialogue queue and box
        void   DialogueQueue(std::vector<std::string> const& lines, TextStyle const& style, 
                                                     int width = 50, int x = 0, int y = 0); // Queue dialogue lines

        void   SetConsoleOverlay(bool enabled);  // Toggle overlay mode on/off

        bool   DialogueNext();                   // Advance to next dialogue line

        bool   LoadFontsFromFolder(std::string const& folder); // Load all fonts from a folder

    private:
        // Fonts (simulated)
        std::unordered_map<std::string, std::string> m_fonts; // alias -> path
        std::optional<std::string> m_activeFont;

        // Text boxes
        std::vector<TextBox> m_boxes;

        // Dialogue
        std::deque<std::string> m_dialogue;
        std::optional<size_t>   m_dialogueBox; // handle of the active dialogue box

        // Internal helpers
        static std::vector<std::string> wrap_text(std::string const& s, int maxWidth);
        void print_at(int x, int y, std::string const& s); 
        void print_box(int x, int y, int width, std::vector<std::string> const& lines, 
            TextStyle const& style);

        void render_dirty();       // Render only dirty boxes
        void ensure_demo_once();   // shows a demo once at startup

        bool m_demoShown = false;   // Track if demo was shown
        bool m_useOverlay = false;  // Toggle overlay vs log mode
    };

} // namespace Framework