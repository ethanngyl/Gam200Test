#include "Graphics/Shader.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>

// Utility: read shader source from file into string
std::string Shader::readFile(const std::string& path) {
    std::ifstream ifs(path, std::ios::in);
    if (!ifs) {
        throw std::runtime_error("Failed to open shader file: " + path);
    }
    std::ostringstream oss;
    oss << ifs.rdbuf();
    return oss.str();
}

// Utility: compile one shader (vertex or fragment)
GLuint Shader::compile(GLenum type, const std::string& src) {
    GLuint shader = glCreateShader(type);
    const char* csrc = src.c_str();
    glShaderSource(shader, 1, &csrc, nullptr);
    glCompileShader(shader);

    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GLint len = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
        std::string log(len, '\0');
        glGetShaderInfoLog(shader, len, nullptr, log.data());
        std::cerr << "Shader compile error:\n" << log << std::endl;
        throw std::runtime_error("Shader compilation failed");
    }
    return shader;
}

// Constructor: load vertex + fragment shaders, link into program
Shader::Shader(const std::string& vsPath, const std::string& fsPath) {
    std::string vsCode = readFile(vsPath);
    std::string fsCode = readFile(fsPath);

    GLuint vs = compile(GL_VERTEX_SHADER, vsCode);
    GLuint fs = compile(GL_FRAGMENT_SHADER, fsCode);

    m_prog = glCreateProgram();
    glAttachShader(m_prog, vs);
    glAttachShader(m_prog, fs);
    glLinkProgram(m_prog);

    GLint ok = 0;
    glGetProgramiv(m_prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        GLint len = 0;
        glGetProgramiv(m_prog, GL_INFO_LOG_LENGTH, &len);
        std::string log(len, '\0');
        glGetProgramInfoLog(m_prog, len, nullptr, log.data());
        std::cerr << "Program link error:\n" << log << std::endl;
        throw std::runtime_error("Shader program linking failed");
    }
    std::cout << "[Shader] Program compiled and linked OK!" << std::endl;
    // no longer need the individual shader objects once linked
    glDeleteShader(vs);
    glDeleteShader(fs);
}

// Destructor: cleanup program
Shader::~Shader() {
    if (m_prog) {
        glDeleteProgram(m_prog);
        m_prog = 0;
    }
}

// Move constructor
Shader::Shader(Shader&& other) noexcept {
    m_prog = other.m_prog;
    other.m_prog = 0;
}

// Move assignment
Shader& Shader::operator=(Shader&& other) noexcept {
    if (this != &other) {
        if (m_prog) glDeleteProgram(m_prog);
        m_prog = other.m_prog;
        other.m_prog = 0;
    }
    return *this;
}

// Activate shader program
void Shader::use() const {
    glUseProgram(m_prog);
}

// Query uniform location
GLint Shader::uniformLoc(const char* name) const {
    return glGetUniformLocation(m_prog, name);
}
