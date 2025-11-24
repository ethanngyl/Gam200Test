/*
===============================================================================
File:        TextRenderer.cpp
Author:      Sim Kah Yan
Email:       kahyan.sim@digipen.edu
Date:        2025-11-07
Contribution: 100%
-------------------------------------------------------------------------------
Brief:
CPU-side text rendering helper using FreeType for glyph rasterization and
modern OpenGL (core) for drawing each character as a textured quad.

Details:
- Initializes FreeType, loads .ttf faces, rasterizes ASCII [0..127] glyphs.
- Uploads each glyph bitmap into a single-channel GL_R8 texture.
- Builds a per-font glyph atlas as a map<char, Glyph> keyed by a font string.
- Renders strings by pushing 6-vertex quads (two triangles) per glyph into a
  dynamic VBO, sampling a glyph texture bound to unit 0.
- Uses an orthographic projection in screen space: origin at bottom-left.

Notes:
- Uses DSA-style OpenGL calls (glCreate* / glNamedBuffer* / glTexture*).
- Text color is applied via a uniform (uTextColor) multiplied with the glyph.
- Keep blending enabled (SRC_ALPHA, ONE_MINUS_SRC_ALPHA) for proper font edges.

Safety:
- All GL resources are deleted in shutdown() and destructor.
- FreeType objects are destroyed in reverse order.
- Graceful checks for missing fonts and compile/link failures.
===============================================================================
*/
#include "TextRenderer.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include <GL/glew.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <fstream>
#include <sstream>
#include <iostream>

// -----------------------------------------------------------------------------
//  Utility helpers
// -----------------------------------------------------------------------------

// Compile a shader stage (vertex or fragment) from source text
unsigned int TextRenderer::compileShader(unsigned int type, const std::string& src)
{
    unsigned int shader = glCreateShader(type);
    const char* code = src.c_str();
    glShaderSource(shader, 1, &code, nullptr);
    glCompileShader(shader);

    // Check compile status and print log on failure
    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[1024];
        glGetShaderInfoLog(shader, 1024, nullptr, log);
        std::cerr << "Shader compile error:\n" << log << std::endl;
    }
    return shader;
}

// Read a whole text file into a string (used for shader sources)
std::string TextRenderer::readFileText(const std::string& path)
{
    std::ifstream file(path);
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

// Create a GL program by compiling and linking vertex/fragment shaders from files
unsigned int TextRenderer::linkProgramFromFiles(const std::string& vsPath, const std::string& fsPath)
{
    // Read text files
    std::string vsrc = readFileText(vsPath);
    std::string fsrc = readFileText(fsPath);

    // Compile stages
    unsigned int vs = compileShader(GL_VERTEX_SHADER, vsrc);
    unsigned int fs = compileShader(GL_FRAGMENT_SHADER, fsrc);

    // Link into a program
    unsigned int program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);

    // Check link status
    int success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char log[1024];
        glGetProgramInfoLog(program, 1024, nullptr, log);
        std::cerr << "Program link error:\n" << log << std::endl;
    }

    // Stages no longer needed after linking
    glDeleteShader(vs);
    glDeleteShader(fs);
    return program;
}

// -----------------------------------------------------------------------------
//  Lifecycle
// -----------------------------------------------------------------------------

// Constructor: default-initialize members
TextRenderer::TextRenderer() = default;

// Destructor: call shutdown() to free GL and FreeType resources
TextRenderer::~TextRenderer()
{
    shutdown();
}
// ------------------------------------------------------------------------------
// shutdown: Release all GL/FreeType resources owned by TextRenderer.
// ------------------------------------------------------------------------------
void TextRenderer::shutdown()
{
    // Free all glyph textures for every loaded font
    for (auto& font : fonts_) {
        for (auto& kv : font.second)
            glDeleteTextures(1, &kv.second.tex);
    }
    fonts_.clear();

    // Delete GL program and buffers
    if (program_) glDeleteProgram(program_);
    if (vbo_) glDeleteBuffers(1, &vbo_);
    if (vao_) glDeleteVertexArrays(1, &vao_);

    // Destroy FreeType library
    if (ft_) {
        FT_Done_FreeType(reinterpret_cast<FT_Library>(ft_));
        ft_ = nullptr;
    }
}

// -----------------------------------------------------------------------------
//  Initialization
// -----------------------------------------------------------------------------
// init: Create GL program, set orthographic projection, and prepare VAO/VBO
void TextRenderer::init(int screenWidth, int screenHeight,
    const std::string& vertPath,
    const std::string& fragPath)
{
    // Enable alpha blending for font edges
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Initialize FreeType
    FT_Library ft;
    if (FT_Init_FreeType(&ft)) {
        std::cerr << "ERROR: Could not initialize FreeType library.\n";
        return;
    }
    ft_ = reinterpret_cast<FT_LibraryRec_*>(ft);

    // Compile and link text shader
    program_ = linkProgramFromFiles(vertPath, fragPath);

    // Set up orthographic projection
    projection_ = glm::ortho(0.0f, static_cast<float>(screenWidth),
        0.0f, static_cast<float>(screenHeight));
    // Cache uniform locations once
    glUseProgram(program_);
    uProjection_ = glGetUniformLocation(program_, "uProjection");
    uTextColor_ = glGetUniformLocation(program_, "uTextColor");
    uSampler_ = glGetUniformLocation(program_, "uTexture");

    glUniformMatrix4fv(uProjection_, 1, GL_FALSE, glm::value_ptr(projection_));

    // Configure VAO/VBO for 6-vertex quads
    glCreateVertexArrays(1, &vao_);
    glCreateBuffers(1, &vbo_);
    glNamedBufferData(vbo_, sizeof(float) * 6 * 4, nullptr, GL_DYNAMIC_DRAW);

    glVertexArrayVertexBuffer(vao_, 0, vbo_, 0, sizeof(float) * 4);
    glEnableVertexArrayAttrib(vao_, 0);
    glVertexArrayAttribFormat(vao_, 0, 4, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(vao_, 0, 0);
}

// -----------------------------------------------------------------------------
//  Load font
// -----------------------------------------------------------------------------
// loadFont: Rasterize ASCII glyphs of a TTF at a given pixel height and cache textures
bool TextRenderer::loadFont(const std::string& key,
    const std::string& ttfPath,
    unsigned int pixelHeight)
{
    if (!ft_) {
        std::cerr << "ERROR: FreeType not initialized before loadFont().\n";
        return false;
    }
    // Create face from .ttf file
    FT_Face face;
    if (FT_New_Face(reinterpret_cast<FT_Library>(ft_), ttfPath.c_str(), 0, &face)) {
        std::cerr << "ERROR: Failed to load font: " << ttfPath << std::endl;
        return false;
    }

    // Set target pixel height for rasterization (width = auto)
    FT_Set_Pixel_Sizes(face, 0, pixelHeight);

    // Make sure 1-byte alignment for single-channel uploads
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    // Per-font glyph table: ASCII 0..127
    std::map<char, Glyph> glyphs;

    for (unsigned char c = 0; c < 128; ++c) {
        // Render glyph bitmap for character c into face->glyph->bitmap
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            std::cerr << "Warning: Failed to load glyph " << int(c) << std::endl;
            continue;
        }
        // Create a single-channel (GL_R8) texture and upload the glyph bitmap
        unsigned int tex;
        glCreateTextures(GL_TEXTURE_2D, 1, &tex);
        glTextureStorage2D(tex, 1, GL_R8,
            face->glyph->bitmap.width,
            face->glyph->bitmap.rows);
        glTextureSubImage2D(tex, 0, 0, 0,
            face->glyph->bitmap.width,
            face->glyph->bitmap.rows,
            GL_RED, GL_UNSIGNED_BYTE,
            face->glyph->bitmap.buffer);

        // Clamp edges to avoid bleeding and set linear filtering
        glTextureParameteri(tex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(tex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTextureParameteri(tex, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(tex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Record glyph metrics for layout during draw()
        Glyph g;
        g.tex = tex;
        g.size = { face->glyph->bitmap.width, face->glyph->bitmap.rows };
        g.bearing = { face->glyph->bitmap_left,  face->glyph->bitmap_top };
        g.advance = static_cast<unsigned int>(face->glyph->advance.x);

        glyphs.emplace(static_cast<char>(c), g);
    }
    // Store this font under the provided key (e.g., "Sans48")
    fonts_[key] = std::move(glyphs);

    FT_Done_Face(face);// Free the face now that glyphs are cached
    return true;
}

// -----------------------------------------------------------------------------
//  Drawing
// -----------------------------------------------------------------------------
// draw: Render ASCII text at screen-space (x, y), scaled and tinted by color
void TextRenderer::draw(const std::string& fontKey,
    const std::string& text,
    float x, float y,
    float scale,
    const glm::vec3& color)
{
    // Find the font; if missing, silently skip
    auto it = fonts_.find(fontKey);
    if (it == fonts_.end()) return;

    glUseProgram(program_);

    // Set RGB color for this draw (alpha comes from glyph texture)
    glUniform3f(uTextColor_, color.r, color.g, color.b);

    // Iterate characters and draw one quad per glyph
    for (char c : text) {
        const auto& glyphMap = it->second;
        auto gIt = glyphMap.find(c);
        if (gIt == glyphMap.end()) continue;
        const Glyph& ch = gIt->second;

        // Compute the glyph quad position in screen space
        // x is the current pen position, bearing moves the bitmap relative to baseline
        float xpos = x + ch.bearing.x * scale;
        float ypos = y - (ch.size.y - ch.bearing.y) * scale;

        // Quad size in pixels, scaled by 'scale'
        float w = ch.size.x * scale;
        float h = ch.size.y * scale;

        // 6 vertices (2 triangles) with (x, y, u, v)
        // Bottom-left origin; texture coords in [0,1]
        float verts[6][4] = {
            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos,     ypos,       0.0f, 1.0f },
            { xpos + w, ypos,       1.0f, 1.0f },
            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos + w, ypos,       1.0f, 1.0f },
            { xpos + w, ypos + h,   1.0f, 0.0f }
        };
        // Bind the glyph texture to unit 0
        glBindTextureUnit(0, ch.tex);

        // Stream this glyph's quad into the dynamic VBO
        glNamedBufferSubData(vbo_, 0, sizeof(verts), verts);
        glBindVertexArray(vao_);

        // Draw two triangles (6 verts)
        glDrawArrays(GL_TRIANGLES, 0, 6);
        // Advance pen position to next glyph:
        // FreeType advance is in 26.6 fixed-point format ? shift right by 6
        x += (ch.advance >> 6) * scale;
    }
    // Unbinds (not strictly necessary, but keeps state tidy)
    glBindVertexArray(0);
    glBindTextureUnit(0, 0);
}

// -----------------------------------------------------------------------------
//  Resize handling
// -----------------------------------------------------------------------------
// setScreenSize: Update internal orthographic projection to new viewport size
void TextRenderer::setScreenSize(int width, int height)
{
    // Rebuild the 2D screen-space projection
    projection_ = glm::ortho(0.0f, static_cast<float>(width),
        0.0f, static_cast<float>(height));
    // Upload to shader if program is valid
    if (program_) {
        glUseProgram(program_);
        glUniformMatrix4fv(uProjection_, 1, GL_FALSE, glm::value_ptr(projection_));
    }
}
