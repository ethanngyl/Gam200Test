#pragma once
/**
===============================================================================
File:        TextRenderer.h
Author:      Sim Kah Yan
Email:       kahyan.sim@digipen.edu
Date:        2025-11-07
Course:      CSD3151 (StructSquad)
-------------------------------------------------------------------------------
Brief:
Header for the TextRenderer class -- a lightweight system that uses FreeType
and modern OpenGL (4.5 core profile) to render 2D text on screen. Each font
is loaded from a TrueType (.ttf) file, where all ASCII glyphs (0-127) are
rasterized and uploaded into individual GL textures.

The renderer draws text directly in pixel coordinates using an orthographic
projection, making it suitable for UI and debug overlays.

Usage Example:
    TextRenderer text;
    text.init(screenW, screenH, "shaders/text.vert", "shaders/text.frag");
    text.loadFont("Sans48",  "assets/fonts/Roboto-Regular.ttf", 48);
    text.draw("Sans48", "Hello World!", 24.f, 64.f, 1.0f, {1,1,1});
    text.setScreenSize(newW, newH); // when window is resized

Notes:
  - Origin is at the bottom-left corner of the screen.
  - Glyphs are uploaded as GL_RED textures (single-channel alpha).
  - Works best with blending: (SRC_ALPHA, ONE_MINUS_SRC_ALPHA).
===============================================================================
*/


#include <map>
#include <string>
#include <unordered_map>
#include <glm/glm.hpp>

// Forward declare FreeType structs to avoid heavy includes in the header.
struct FT_LibraryRec_;
struct FT_FaceRec_;

class TextRenderer {
public:
    // Constructor / Destructor
    TextRenderer();
    ~TextRenderer();

    // ------------------------------------------------------------------------
    // Initialization
    // ------------------------------------------------------------------------

    // Initializes the renderer with the given screen dimensions and shader paths.
    // Must be called after an OpenGL context is active.
    void init(int screenWidth, int screenHeight,
        const std::string& vertPath,
        const std::string& fragPath);

    // Loads ASCII glyphs (0-127) for the specified TrueType font at pixelHeight.
    // Returns true on success, false if the file cannot be loaded.
    bool loadFont(const std::string& fontKey,
        const std::string& ttfPath,
        unsigned int pixelHeight = 48);

    // ------------------------------------------------------------------------
    // Rendering
    // ------------------------------------------------------------------------

    // Draws a UTF-8 string (ASCII subset) on screen.
    // (x, y) are pixel coordinates measured from the bottom-left corner.
    // 'scale' adjusts text size; 'color' is an RGB triplet.
    void draw(const std::string& fontKey,
        const std::string& text,
        float x, float y,
        float scale,
        const glm::vec3& color);

    // Updates the orthographic projection matrix when the window is resized.
    void setScreenSize(int width, int height);

    // Explicit cleanup for GL and FreeType resources (automatically done in destructor).
    void shutdown();

private:
    // ------------------------------------------------------------------------
    // Internal glyph data
    // ------------------------------------------------------------------------
    struct Glyph {
        unsigned int tex = 0;      // GL texture id
        glm::ivec2   size{ 0 };      // bitmap size
        glm::ivec2   bearing{ 0 };   // baseline offset
        unsigned int advance = 0;  // 1/64 pixels (FreeType units)
    };

    // ------------------------------------------------------------------------
    // FreeType and Font Data
    // ------------------------------------------------------------------------
    FT_LibraryRec_* ft_ = nullptr;// FreeType library instance

    // One glyph map per loaded font key
    std::unordered_map<std::string, std::map<char, Glyph>> fonts_;

    // ------------------------------------------------------------------------
    // OpenGL Objects
    // ------------------------------------------------------------------------
    unsigned int vao_ = 0;          // Vertex Array Object for rendering quads
    unsigned int vbo_ = 0;          // Vertex Buffer Object (6 vertices per glyph)
    unsigned int program_ = 0;      // Compiled text shader program

    // Uniform locations cached after shader linking
    int uProjection_ = -1;
    int uTextColor_ = -1;
    int uSampler_ = -1;

    // Cached orthographic projection matrix for pixel-space rendering
    glm::mat4 projection_{ 1.0f };

    // ------------------------------------------------------------------------
    // Internal Helper Functions (defined in .cpp)
    // ------------------------------------------------------------------------
    static unsigned int compileShader(unsigned int type, const std::string& src);
    static unsigned int linkProgramFromFiles(const std::string& vsPath, const std::string& fsPath);
    static std::string  readFileText(const std::string& path);
};
