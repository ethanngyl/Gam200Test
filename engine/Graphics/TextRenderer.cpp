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
unsigned int TextRenderer::compileShader(unsigned int type, const std::string& src)
{
    unsigned int shader = glCreateShader(type);
    const char* code = src.c_str();
    glShaderSource(shader, 1, &code, nullptr);
    glCompileShader(shader);

    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[1024];
        glGetShaderInfoLog(shader, 1024, nullptr, log);
        std::cerr << "Shader compile error:\n" << log << std::endl;
    }
    return shader;
}

std::string TextRenderer::readFileText(const std::string& path)
{
    std::ifstream file(path);
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

unsigned int TextRenderer::linkProgramFromFiles(const std::string& vsPath, const std::string& fsPath)
{
    std::string vsrc = readFileText(vsPath);
    std::string fsrc = readFileText(fsPath);

    unsigned int vs = compileShader(GL_VERTEX_SHADER, vsrc);
    unsigned int fs = compileShader(GL_FRAGMENT_SHADER, fsrc);

    unsigned int program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);

    int success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char log[1024];
        glGetProgramInfoLog(program, 1024, nullptr, log);
        std::cerr << "Program link error:\n" << log << std::endl;
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
    return program;
}

// -----------------------------------------------------------------------------
//  Lifecycle
// -----------------------------------------------------------------------------
TextRenderer::TextRenderer() = default;

TextRenderer::~TextRenderer()
{
    shutdown();
}

void TextRenderer::shutdown()
{
    for (auto& font : fonts_) {
        for (auto& kv : font.second)
            glDeleteTextures(1, &kv.second.tex);
    }
    fonts_.clear();

    if (program_) glDeleteProgram(program_);
    if (vbo_) glDeleteBuffers(1, &vbo_);
    if (vao_) glDeleteVertexArrays(1, &vao_);

    if (ft_) {
        FT_Done_FreeType(reinterpret_cast<FT_Library>(ft_));
        ft_ = nullptr;
    }
}

// -----------------------------------------------------------------------------
//  Initialization
// -----------------------------------------------------------------------------
void TextRenderer::init(int screenWidth, int screenHeight,
    const std::string& vertPath,
    const std::string& fragPath)
{
    // Enable blending (should already be done by your GraphicsSystem)
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
    uProjection_ = glGetUniformLocation(program_, "uProjection");
    uTextColor_ = glGetUniformLocation(program_, "uTextColor");
    uSampler_ = glGetUniformLocation(program_, "uText");

    // Set up orthographic projection
    projection_ = glm::ortho(0.0f, static_cast<float>(screenWidth),
        0.0f, static_cast<float>(screenHeight));
    glUseProgram(program_);
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
bool TextRenderer::loadFont(const std::string& key,
    const std::string& ttfPath,
    unsigned int pixelHeight)
{
    if (!ft_) {
        std::cerr << "ERROR: FreeType not initialized before loadFont().\n";
        return false;
    }

    FT_Face face;
    if (FT_New_Face(reinterpret_cast<FT_Library>(ft_), ttfPath.c_str(), 0, &face)) {
        std::cerr << "ERROR: Failed to load font: " << ttfPath << std::endl;
        return false;
    }

    FT_Set_Pixel_Sizes(face, 0, pixelHeight);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    std::map<char, Glyph> glyphs;

    for (unsigned char c = 0; c < 128; ++c) {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            std::cerr << "Warning: Failed to load glyph " << int(c) << std::endl;
            continue;
        }

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

        glTextureParameteri(tex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(tex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTextureParameteri(tex, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(tex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        Glyph g;
        g.tex = tex;
        g.size = { face->glyph->bitmap.width, face->glyph->bitmap.rows };
        g.bearing = { face->glyph->bitmap_left,  face->glyph->bitmap_top };
        g.advance = static_cast<unsigned int>(face->glyph->advance.x);

        glyphs.emplace(static_cast<char>(c), g);
    }

    fonts_[key] = std::move(glyphs);

    FT_Done_Face(face);
    return true;
}

// -----------------------------------------------------------------------------
//  Drawing
// -----------------------------------------------------------------------------
void TextRenderer::draw(const std::string& fontKey,
    const std::string& text,
    float x, float y,
    float scale,
    const glm::vec3& color)
{
    auto it = fonts_.find(fontKey);
    if (it == fonts_.end()) return;

    glUseProgram(program_);
    glUniform3fv(uTextColor_, 1, glm::value_ptr(color));
    glUniform1i(uSampler_, 0);

    glBindVertexArray(vao_);

    for (char c : text) {
        const auto& glyphMap = it->second;
        auto gIt = glyphMap.find(c);
        if (gIt == glyphMap.end()) continue;
        const Glyph& ch = gIt->second;

        float xpos = x + ch.bearing.x * scale;
        float ypos = y - (ch.size.y - ch.bearing.y) * scale;
        float w = ch.size.x * scale;
        float h = ch.size.y * scale;

        float verts[6][4] = {
            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos,     ypos,       0.0f, 1.0f },
            { xpos + w, ypos,       1.0f, 1.0f },
            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos + w, ypos,       1.0f, 1.0f },
            { xpos + w, ypos + h,   1.0f, 0.0f }
        };

        glBindTextureUnit(0, ch.tex);
        glNamedBufferSubData(vbo_, 0, sizeof(verts), verts);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        x += (ch.advance >> 6) * scale;
    }

    glBindVertexArray(0);
    glBindTextureUnit(0, 0);
}

// -----------------------------------------------------------------------------
//  Resize handling
// -----------------------------------------------------------------------------
void TextRenderer::setScreenSize(int width, int height)
{
    projection_ = glm::ortho(0.0f, static_cast<float>(width),
        0.0f, static_cast<float>(height));
    if (program_) {
        glUseProgram(program_);
        glUniformMatrix4fv(uProjection_, 1, GL_FALSE, glm::value_ptr(projection_));
    }
}
