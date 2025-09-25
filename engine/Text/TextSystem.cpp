#include "TextSystem.h"
#include "Core.h"        // for CORE->BroadcastMessage(...)
#include <algorithm>
#include <sstream>
#include <cctype>

namespace Framework {

    // -------- Utility: word wrapping --------
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

    // -------- Utility: print at console position (simple) --------
    void TextSystem::print_at(int x, int y, std::string const& s) {
#ifdef _WIN32
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
    TextSystem::TextSystem() {}
    TextSystem::~TextSystem() {}

    void TextSystem::Initialize() {
        m_demoShown = false;
    }

    void TextSystem::Update(float /*dt*/) {
#ifdef _WIN32
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

    void TextSystem::SendEngineMessage(Message* /*message*/) {
        // No-op for now
    }

    // -------- Public API --------
    bool TextSystem::LoadFont(std::string const& logicalName, std::string const& ttfPath) {
        if (logicalName.empty() || ttfPath.empty()) return false;
        m_fonts[logicalName] = ttfPath;
        if (!m_activeFont.has_value()) m_activeFont = logicalName;
        return true;
    }

    bool TextSystem::SetActiveFont(std::string const& logicalName) {
        auto it = m_fonts.find(logicalName);
        if (it == m_fonts.end()) return false;
        m_activeFont = logicalName;
        return true;
    }

    std::optional<std::string> TextSystem::GetActiveFont() const {
        return m_activeFont;
    }

    size_t TextSystem::AddTextBox(TextBox const& box) {
        m_boxes.push_back(box);
        m_boxes.back().dirty = true;
        return m_boxes.size() - 1;
    }

    bool TextSystem::SetText(size_t handle, std::string const& text) {
        if (handle >= m_boxes.size()) return false;
        m_boxes[handle].text = text;
        m_boxes[handle].dirty = true;
        return true;
    }

    bool TextSystem::MoveBox(size_t handle, int x, int y) {
        if (handle >= m_boxes.size()) return false;
        m_boxes[handle].x = x; m_boxes[handle].y = y;
        m_boxes[handle].dirty = true;
        return true;
    }

    bool TextSystem::SetWidth(size_t handle, int width) {
        if (handle >= m_boxes.size() || width <= 0) return false;
        m_boxes[handle].width = width;
        m_boxes[handle].dirty = true;
        return true;
    }

    bool TextSystem::SetStyle(size_t handle, TextStyle const& style) {
        if (handle >= m_boxes.size()) return false;
        m_boxes[handle].style = style;
        m_boxes[handle].dirty = true;
        return true;
    }

    void TextSystem::DialogueClear() {
        m_dialogue.clear();
        if (m_dialogueBox.has_value() && *m_dialogueBox < m_boxes.size()) {
            m_boxes[*m_dialogueBox].text.clear();
            m_boxes[*m_dialogueBox].dirty = true;
        }
    }

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

    bool TextSystem::DialogueNext() {
        if (!m_dialogueBox.has_value()) return false;
        if (m_dialogue.empty()) return false;
        SetText(*m_dialogueBox, m_dialogue.front());
        m_dialogue.pop_front();
        return true;
    }

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
    void TextSystem::render_dirty() {
        for (auto& b : m_boxes) {
            if (!b.dirty) continue;
            auto lines = wrap_text(b.text, b.width);
            print_box(b.x, b.y, b.width, lines, b.style);
            b.dirty = false;
        }
    }

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