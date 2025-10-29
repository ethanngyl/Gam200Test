#pragma once
/**
 * @file TextRenderer.h
 * @brief Minimal FreeType-based text drawing for OpenGL 4.5.
 *
 * Usage:
 *   TextRenderer text;
 *   text.init(screenW, screenH, "shaders/text.vert", "shaders/text.frag");
 *   text.loadFont("Sans48",  "assets/fonts/Roboto-Regular.ttf", 48);
 *   text.loadFont("Serif32", "assets/fonts/NotoSerif-Bold.ttf", 32);
 *   text.draw("Sans48", "Hello", 24.f, 64.f, 1.0f, {1,1,1});
 *   text.setScreenSize(newW, newH); // on resize
 *
 * Notes:
 *  - Draws in pixel space with origin at bottom-left (orthographic).
 *  - Expects glyph textures uploaded as GL_RED and sampled as alpha.
 */

#include <map>
#include <string>
#include <unordered_map>
#include <glm/glm.hpp>

 // Forward declare FreeType types to keep this header lightweight.
struct FT_LibraryRec_;
struct FT_FaceRec_;

class TextRenderer {
public:
    TextRenderer();
    ~TextRenderer();

    // Call once after a valid GL context exists.
    void init(int screenWidth, int screenHeight,
        const std::string& vertPath,
        const std::string& fragPath);

    // Loads ASCII glyphs (0..127) for a .ttf at 'pixelHeight'. Returns success.
    bool loadFont(const std::string& fontKey,
        const std::string& ttfPath,
        unsigned int pixelHeight = 48);

    // Draw a UTF-8 string (ASCII subset rendered; extended set would need atlas).
    // (x, y) are pixel coordinates from bottom-left.
    void draw(const std::string& fontKey,
        const std::string& text,
        float x, float y,
        float scale,
        const glm::vec3& color);

    // Update internal orthographic projection (call on window resize).
    void setScreenSize(int width, int height);

    // Optional explicit cleanup (also done in destructor).
    void shutdown();

private:
    struct Glyph {
        unsigned int tex = 0;      // GL texture id
        glm::ivec2   size{ 0 };      // bitmap size
        glm::ivec2   bearing{ 0 };   // baseline offset
        unsigned int advance = 0;  // 1/64 pixels (FreeType units)
    };

    // --- FreeType
    FT_LibraryRec_* ft_ = nullptr;

    // One glyph map per loaded font key
    std::unordered_map<std::string, std::map<char, Glyph>> fonts_;

    // --- GL objects
    unsigned int vao_ = 0;
    unsigned int vbo_ = 0;
    unsigned int program_ = 0;   // text shader program

    // Uniform locations
    int uProjection_ = -1;
    int uTextColor_ = -1;
    int uSampler_ = -1;

    // Cached ortho projection
    glm::mat4 projection_{ 1.0f };

    // Internal helpers (defined in .cpp)
    static unsigned int compileShader(unsigned int type, const std::string& src);
    static unsigned int linkProgramFromFiles(const std::string& vsPath, const std::string& fsPath);
    static std::string  readFileText(const std::string& path);
};
