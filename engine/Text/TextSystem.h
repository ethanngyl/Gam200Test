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
        std::string fontName;     //!< Logical font name (must be loaded)
        char border = '#';        //!< Box border character
        int  padding = 1;         //!< Inner padding (spaces)
    };

    struct TextBox {
        int x = 0;                //!< Console column (0-based)
        int y = 0;                //!< Console row    (0-based)
        int width = 40;           //!< Inner text width (content area)
        std::string text;         //!< Raw content (will be wrapped)
        TextStyle style;

        bool dirty = true;        //!< Reprint when changed
    };

    class TextSystem : public InterfaceSystem
    {
    public:
        TextSystem();
        ~TextSystem() override;

        // InterfaceSystem
        void Initialize() override;
        void Update(float dt) override;
        void SendEngineMessage(Message* message) override;

        // --- Public API (engine/game can call these later) ---
        bool LoadFont(std::string const& logicalName, std::string const& ttfPath);

        /*! \brief Set the active font by alias.
            \return true on success.
            \note No console I/O; return status code. */
        bool SetActiveFont(std::string const& logicalName);

        std::optional<std::string> GetActiveFont() const;

        size_t AddTextBox(TextBox const& box);

        bool   SetText(size_t handle, std::string const& text);

        bool   MoveBox(size_t handle, int x, int y);

        bool   SetWidth(size_t handle, int width);

        bool   SetStyle(size_t handle, TextStyle const& style);

        void   DialogueClear();
        void   DialogueQueue(std::vector<std::string> const& lines, TextStyle const& style, int width = 50, int x = 0, int y = 0);
        bool   DialogueNext();

        bool   LoadFontsFromFolder(std::string const& folder);

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
        static void print_at(int x, int y, std::string const& s);
        static void print_box(int x, int y, int width, std::vector<std::string> const& lines, TextStyle const& style);
        void render_dirty();
        void ensure_demo_once();   // shows a demo once at startup

        bool m_demoShown = false;
    };

} // namespace Framework