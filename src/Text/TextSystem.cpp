/*
===============================================================================
 File:          TextSystem.cpp
 Author:        PADILLA CARL JAMESON Z.
 Email:         c.padilla@digipen.edu
 Date:          2025-09-30
 Contribution:  100%
 ------------------------------------------------------------------------------
 Implementation of TextSystem.

 Responsibilities:
  - Provide a simple console-based Text/UI system to prototype before a window/
    renderer exists.
  - Render readable ASCII text boxes with borders, padding, and word wrapping.
  - Auto-load fonts from a `Fonts/` folder and manage them by alias.
  - Include a lightweight dialogue queue (enqueue lines, next, clear) for quick
    narrative/UI testing.
  - Two output modes:
      * Normal log mode (default): append after existing console logs.
      * Overlay mode (optional): fixed (x,y) positioning for HUD-style boxes.
  - Integrate cleanly as an InterfaceSystem: Initialize(), Update(dt),
	SendEngineMessage(...) without coupling to Graphics.

 Platform notes:
  - On Windows, overlay mode uses console APIs to position the cursor.
  - On other platforms, overlay mode falls back to normal log mode.

 Safety:
  - Functions are noexcept where practical to ensure that even during a crash,
	we make a best effort to produce a file.

 Copyright (C) 2026 DigiPen Institute of Technology.
 Reproduction or disclosure of this file or its contents
 without the prior written consent of DigiPen Institute of
 Technology is prohibited.
===============================================================================
*/

#include "TextSystem.h"
#include "Core.h"        // for CORE->BroadcastMessage(...)
#include <algorithm>
#include <sstream>
#include <cctype>
#include "Debugger/Trace.h"


namespace Framework {

    /************************************************************************/
    /*!
    \brief
    Turns the console overlay on or off. When off, text appends like normal logs.
    \param enabled
    True to draw at fixed (x,y) using cursor positioning; false to just append.
    */
    /************************************************************************/
    void TextSystem::SetConsoleOverlay(bool enabled) {
        m_useOverlay = enabled;
    }

    /************************************************************************/
    /*!
    \brief
     Wraps a long string into multiple lines that fit within a maximum width.
    \param s
    The input text to wrap.
    \param maxWidth
    Maximum number of characters per wrapped line.
    */
    /************************************************************************/
    std::vector<std::string> TextSystem::wrap_text(std::string const& s, int maxWidth) {
        std::vector<std::string> lines;
        if (maxWidth <= 0) { lines.push_back(s); return lines; }

        std::istringstream iss(s);
        std::string word, line;
        while (iss >> word) {
            int nextLen = static_cast<int>((line.empty() ? 0 : line.size() + 1) + word.size());
            if (nextLen > maxWidth) {
                if (!line.empty()) lines.push_back(line);
                line = word;
            }
            else {
                if (!line.empty()) line.push_back(' ');
                line += word;
            }
        }
        if (!line.empty()) lines.push_back(line);

        // Also split on explicit '\n' and hard-wrap long chunks without spaces
        std::vector<std::string> finalLines;
        for (auto& L : lines) {
            std::string chunk;
            std::istringstream perLine(L);
            while (std::getline(perLine, chunk, '\n')) {
                while (static_cast<int>(chunk.size()) > maxWidth && maxWidth > 0) {
                    finalLines.push_back(chunk.substr(0, maxWidth));
                    chunk.erase(0, maxWidth);
                }
                finalLines.push_back(chunk);
            }
        }
        if (finalLines.empty()) finalLines.push_back("");
        return finalLines;
    }

    /************************************************************************/
    /*!
    \brief
    Prints one line of text to the console.
    If overlay is off, appends a new line; if on, prints at position (x,y).
    \param x
    Console column (0-based) used when overlay is on.
    \param y
    Console row (0-based) used when overlay is on.
    \param s
    The text to print.
    */
    /************************************************************************/
    // -------- Utility: print at console position (simple) --------
    void TextSystem::print_at(int x, int y, std::string const& s) {
#ifdef _WIN32
    if (!m_useOverlay) {
        std::cout << s << "\n";
        return;
    }
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) { std::cout << s << "\n"; return; }
    COORD pos; pos.X = static_cast<SHORT>(x); pos.Y = static_cast<SHORT>(y);
    SetConsoleCursorPosition(hOut, pos);
    DWORD written = 0;
    WriteConsoleA(hOut, s.c_str(), static_cast<DWORD>(s.size()), &written, nullptr);
#else
    std::cout << s << "\n";
#endif
    }

    /************************************************************************/
    /*!
    \brief
    Draws a bordered text box with padding and wrapped content at (x,y).
    \param x
    Console column (0-based) for the left edge of the box.
    \param y
    Console row (0-based) for the top edge of the box.
    \param width
    Inner text width used for wrapping (not counting padding/borders).
    \param lines
    The pre-wrapped lines to render inside the box.
    \param style
    Border character, padding size, and font alias for the box.
    */
    /************************************************************************/
    // -------- Utility: draw a bordered box --------
    void TextSystem::print_box(int x, int y, int width, std::vector<std::string> const& lines, TextStyle const& style) {
        const int pad = (std::max)(0, style.padding);
        const int inner = (std::max)(1, width);
        const int totalW = inner + (pad * 2);
        const char b = style.border;

        // Top border
        print_at(x, y, std::string(1, b) + std::string(totalW + 2, b) + std::string(1, b));

        // Empty padding row (top)
        std::string padRow = std::string(1, b) + " " + std::string(totalW, ' ') + " " + std::string(1, b);
        for (int i = 0; i < pad; ++i)
            print_at(x, y + 1 + i, padRow);

        // Content rows
        int row = y + 1 + pad;
        for (auto const& line : lines) {
            std::string clipped = line;
            if (static_cast<int>(clipped.size()) > inner) clipped.resize(inner);
            std::string content = clipped + std::string(inner - static_cast<int>(clipped.size()), ' ');
            print_at(x, row++, std::string(1, b) + " " + std::string(pad, ' ') + content + std::string(pad, ' ') + " " + std::string(1, b));
        }

        // Bottom padding
        int bottomPadStart = row;
        for (int i = 0; i < pad; ++i)
            print_at(x, bottomPadStart + i, padRow);

        // Bottom border
        print_at(x, bottomPadStart + pad, std::string(1, b) + std::string(totalW + 2, b) + std::string(1, b));

        // Font note (simulated)
        std::string fontNote = "[font: " + (style.fontName.empty() ? std::string("<default>") : style.fontName) + "]";
        print_at(x, bottomPadStart + pad + 1, fontNote);
    }

    // -------- Lifecycle --------
    /************************************************************************/
    /*!
    \brief
    Constructs the TextSystem and initializes internal state.
    */
    /************************************************************************/
    TextSystem::TextSystem() {}

    /************************************************************************/
    /*!
    \brief
    Destroys the TextSystem and releases owned resources (if any).
    */
    /************************************************************************/
    TextSystem::~TextSystem() {}


    /************************************************************************/
    /*!
    \brief
     Engine lifecycle hook called once to prepare the system.
    */
    /************************************************************************/
    void TextSystem::Initialize() {
        m_demoShown = false;
    }

    /************************************************************************/
    /*!
    \brief
    Engine update: handles hotkeys (N/C/Q), ensures the demo runs once,
    and renders any text boxes marked as dirty.
    \param dt
    Time step in seconds since the previous update.
    */
    /************************************************************************/
    void TextSystem::Update(float /*dt*/) {
#ifdef _WIN32
        DBG_SCOPE_SYS("Text System", eng::debug::Subsystem::Gameplay);

        if (_kbhit()) {
            int ch = _getch();
            if (ch == 'N' || ch == 'n') {
                DialogueNext();
            }
            else if (ch == 'C' || ch == 'c') {
                DialogueClear();
            }
            else if (ch == 'Q' || ch == 'q') {
                if (CORE) {
                    Message m(Status::Quit);
                    CORE->BroadcastMessage(&m);
                }
                return;
            }
        }
#endif

        ensure_demo_once();
        render_dirty();
    }

    /************************************************************************/
    /*!
    \brief
    Receives engine messages. Currently a no-op placeholder for future use.
    \param message
    Pointer to the engine message.
    */
    /************************************************************************/
    void TextSystem::SendEngineMessage(Message* /*message*/) {
        // No-op for now
    }

    /************************************************************************/
    /*!
    \brief
    Registers a font alias mapped to a file path (simulated; no glyph loading).
    \param logicalName
    The alias used to reference the font.
    \param ttfPath
    The file system path to the .ttf/.otf font file.
    */
    /************************************************************************/
    // -------- Public API --------
    bool TextSystem::LoadFont(std::string const& logicalName, std::string const& ttfPath) {
        if (logicalName.empty() || ttfPath.empty()) return false;
        m_fonts[logicalName] = ttfPath;
        if (!m_activeFont.has_value()) m_activeFont = logicalName;
        return true;
    }

    /************************************************************************/
    /*!
    \brief
    Sets a previously loaded font alias as the active font.
    \param logicalName
    The alias of the font to activate.
    */
    /************************************************************************/
    bool TextSystem::SetActiveFont(std::string const& logicalName) {
        auto it = m_fonts.find(logicalName);
        if (it == m_fonts.end()) return false;
        m_activeFont = logicalName;
        return true;
    }

    /************************************************************************/
    /*!
    \brief
    Gets the current active font alias if one is set.
    \return
    The alias string, or empty optional if no active font exists.
    */
    /************************************************************************/
    std::optional<std::string> TextSystem::GetActiveFont() const {
        return m_activeFont;
    }


    /************************************************************************/
    /*!
    \brief
    Creates a new text box and marks it dirty so it will render on update.
    \param box
    Initial properties for the text box (position, width, style, text).
    \return
    The handle (index) assigned to the new text box.
    */
    /************************************************************************/
    size_t TextSystem::AddTextBox(TextBox const& box) {
        m_boxes.push_back(box);
        m_boxes.back().dirty = true;
        return m_boxes.size() - 1;
    }

    /************************************************************************/
    /*!
    \brief
    Replaces the text content of an existing box and marks it dirty.
    \param handle
    The index/handle of the text box to modify.
    \param text
    The new text content for the box.
    \return
    True if successful, false if the handle is invalid.
    */
    /************************************************************************/
    bool TextSystem::SetText(size_t handle, std::string const& text) {
        if (handle >= m_boxes.size()) return false;
        m_boxes[handle].text = text;
        m_boxes[handle].dirty = true;
        return true;
    }

    /************************************************************************/
    /*!
    \brief
    Moves a text box to a new (x,y) position and marks it dirty.
    \param handle
    The index/handle of the text box to move.
    \param x
    New console column (0-based).
    \param y
    New console row (0-based).
    \return
    True if successful, false if the handle is invalid.
    */
    /************************************************************************/
    bool TextSystem::MoveBox(size_t handle, int x, int y) {
        if (handle >= m_boxes.size()) return false;
        m_boxes[handle].x = x; m_boxes[handle].y = y;
        m_boxes[handle].dirty = true;
        return true;
    }

    /************************************************************************/
    /*!
    \brief
    Changes the inner wrap width of a text box and marks it dirty.
    \param handle
    The index/handle of the text box to modify.
    \param width
    New inner text width (must be > 0).
    \return
    True if successful, false if invalid handle or width.
    */
    /************************************************************************/
    bool TextSystem::SetWidth(size_t handle, int width) {
        if (handle >= m_boxes.size() || width <= 0) return false;
        m_boxes[handle].width = width;
        m_boxes[handle].dirty = true;
        return true;
    }

    /************************************************************************/
    /*!
    \brief
    Replaces a text box's style (border, padding, font alias) and marks it dirty.
    \param handle
    The index/handle of the text box to modify.
    \param style
    The new style to apply to the box.
    \return
    True if successful, false if the handle is invalid.
    */
    /************************************************************************/
    bool TextSystem::SetStyle(size_t handle, TextStyle const& style) {
        if (handle >= m_boxes.size()) return false;
        m_boxes[handle].style = style;
        m_boxes[handle].dirty = true;
        return true;
    }

    /************************************************************************/
    /*!
    \brief
    Clears all queued dialogue and wipes the dialogue box text if it exists.
    */
    /************************************************************************/
    void TextSystem::DialogueClear() {
        m_dialogue.clear();
        if (m_dialogueBox.has_value() && *m_dialogueBox < m_boxes.size()) {
            m_boxes[*m_dialogueBox].text.clear();
            m_boxes[*m_dialogueBox].dirty = true;
        }
    }

    /************************************************************************/
    /*!
    \brief
    Queues multiple dialogue lines and ensures a dialogue box exists.
    Immediately displays the first queued line.
    \param lines
    The sequence of dialogue lines to enqueue.
    \param style
    Style used by the dialogue box.
    \param width
    Inner wrap width for the dialogue box.
    \param x
    Console column (0-based) for the dialogue box.
    \param y
    Console row (0-based) for the dialogue box.
    */
    /************************************************************************/
    void TextSystem::DialogueQueue(std::vector<std::string> const& lines, TextStyle const& style, int width, int x, int y) {
        for (auto const& L : lines) m_dialogue.push_back(L);
        if (!m_dialogueBox.has_value()) {
            TextBox box;
            box.x = x; box.y = y; box.width = width; box.text = "";
            box.style = style;
            m_dialogueBox = AddTextBox(box);
        }
        if (!m_dialogue.empty()) {
            SetText(*m_dialogueBox, m_dialogue.front());
            m_dialogue.pop_front();
        }
    }

    /************************************************************************/
    /*!
    \brief
    Advances to the next queued dialogue line and updates the dialogue box.
    \return
    True if successful, false if no dialogue box or no more lines.
    */
    /************************************************************************/
    bool TextSystem::DialogueNext() {
        if (!m_dialogueBox.has_value()) return false;
        if (m_dialogue.empty()) return false;
        SetText(*m_dialogueBox, m_dialogue.front());
        m_dialogue.pop_front();
        return true;
    }

    /************************************************************************/
    /*!
    \brief
    Scans a folder and auto-registers all .ttf/.otf files by filename stem
    as their font alias.
    \param folder
    Path to the directory to scan for font files.
    \return
    True if at least one font was loaded, false otherwise.
    */
    /************************************************************************/
    bool TextSystem::LoadFontsFromFolder(std::string const& folder) {
        namespace fs = std::filesystem;
        std::error_code ec;
        if (!fs::exists(folder, ec) || !fs::is_directory(folder, ec)) return false;

        bool any = false;
        for (auto const& entry : fs::directory_iterator(folder, ec)) {
            if (ec) break;
            if (!entry.is_regular_file()) continue;
            auto ext = entry.path().extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            if (ext == ".ttf" || ext == ".otf") {
                std::string logical = entry.path().stem().string();   // e.g., "PixelOperator"
                std::string path = entry.path().string();          // e.g., "Fonts/PixelOperator.ttf"
                if (LoadFont(logical, path)) any = true;
            }
        }
        return any;
    }

    // -------- Internal --------
    /************************************************************************/
    /*!
    \brief
    Renders only text boxes marked as dirty, then clears their dirty flags.
    */
    /************************************************************************/
    void TextSystem::render_dirty() {
        for (auto& b : m_boxes) {
            if (!b.dirty) continue;
            auto lines = wrap_text(b.text, b.width);
            print_box(b.x, b.y, b.width, lines, b.style);
            b.dirty = false;
        }
    }

    /************************************************************************/
    /*!
    \brief
    Performs a one-time demo setup: auto-loads fonts from "Fonts/",
    selects a default alias if needed, and creates sample boxes.
    */
    /************************************************************************/
    void TextSystem::ensure_demo_once() {
        if (m_demoShown) return;

        LoadFontsFromFolder("Fonts"); // drop your .ttf/.otf here
        if (!m_activeFont.has_value()) {
            m_activeFont = "Default";
        }

        // Static label
        TextStyle labelStyle;
        labelStyle.fontName = m_activeFont.value_or("Default");
        labelStyle.border = '+';
        labelStyle.padding = 0;

        TextBox label;
        label.x = 0; label.y = 0; label.width = 36;
        label.style = labelStyle;
        label.text = "TextSystem ready. Keys: [N] Next  [C] Clear  [Q] Quit";
        AddTextBox(label);

        // Dialogue box demo
        TextStyle dlgStyle;
        dlgStyle.fontName = m_activeFont.value_or("Default");
        dlgStyle.border = '#';
        dlgStyle.padding = 1;

        DialogueQueue({
          "Houston, we've had a problem here.",
          "Say again, please?",
          "Oxygen tank two just went offline.",
          "Copy. Begin power-down procedures."
            }, dlgStyle, /*width*/50, /*x*/2, /*y*/4);

        m_demoShown = true;
    }

} // namespace Framework