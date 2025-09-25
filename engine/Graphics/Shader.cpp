#include "Precompiled.h"
#include "Shader.h"
#include "GL/glew.h"
#include "GL/gl.h"

namespace Framework {

    Shader::Shader(const std::string& vertexPath, const std::string& fragmentPath) {
        std::cout << "\n\n==============================================\n";
        std::cout << "    VERTEX & FRAGMENT SHADER SRC FILES\n";
        std::cout << "==============================================\n\n";

        std::string vertexSrc = LoadFile(vertexPath);
        std::cout << "Vertex Shader Source:\n" << vertexSrc << "\n";

        std::string fragmentSrc = LoadFile(fragmentPath);
        std::cout << "Fragment Shader Source:\n" << fragmentSrc << "\n";

        unsigned int vs = Compile(GL_VERTEX_SHADER, vertexSrc);
        unsigned int fs = Compile(GL_FRAGMENT_SHADER, fragmentSrc);

        id = glCreateProgram();
        glAttachShader(id, vs);
        glAttachShader(id, fs);
        glLinkProgram(id);

        int success;
        glGetProgramiv(id, GL_LINK_STATUS, &success);
        if (!success) {
            char infoLog[512];
            glGetProgramInfoLog(id, 512, nullptr, infoLog);
            std::cerr << "Shader link failed:\n" << infoLog << "\n";
        }

        glDeleteShader(vs);
        glDeleteShader(fs);
    }

    Shader::~Shader() {
        glDeleteProgram(id);
    }

    void Shader::Bind() const {
        glUseProgram(id);
    }

    void Shader::Unbind() const {
        glUseProgram(0);
    }

    std::string Shader::LoadFile(const std::string& path) {
        std::ifstream file(path);
        if (!file) {
            std::cerr << "ERROR: Failed to open shader file: " << path << std::endl;
            return "";
        }

        std::stringstream ss;
        ss << file.rdbuf();
        return ss.str();
    }

    unsigned int Shader::Compile(unsigned int type, const std::string& source) {
        unsigned int shader = glCreateShader(type);
        const char* src = source.c_str();
        glShaderSource(shader, 1, &src, nullptr);
        glCompileShader(shader);

        int success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            char infoLog[512];
            glGetShaderInfoLog(shader, 512, nullptr, infoLog);
            std::cerr << (type == GL_VERTEX_SHADER ? "\nVertex" : "Fragment")
                << " shader compile error:\n" << infoLog << "\n";
        }
        else {
            std::cout << (type == GL_VERTEX_SHADER ? "\nVertex" : "Fragment")
                << " shader compiled successfully\n";
        }

        return shader;
    }

}
