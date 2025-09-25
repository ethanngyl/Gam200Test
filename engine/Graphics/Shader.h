#pragma once
#include <string>
#include <gl/glew.h>

class Shader {
public:
    Shader() = default;
    Shader(const std::string& vsPath, const std::string& fsPath);
    ~Shader();

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;
    Shader(Shader&&) noexcept;
    Shader& operator=(Shader&&) noexcept;

    void use() const;
    GLint uniformLoc(const char* name) const;
    GLuint id() const { return m_prog; }

private:
    GLuint m_prog = 0;

    static std::string readFile(const std::string& path);
    static GLuint compile(GLenum type, const std::string& src);
};
