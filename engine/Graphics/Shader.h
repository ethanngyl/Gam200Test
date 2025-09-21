#pragma once
#include <string>
#include <gl/glew.h>

class Shader {
public:
    Shader(const char* vertexPath, const char* fragmentPath);
    void Use() const;
    GLuint ID;

private:
    std::string LoadFile(const char* path);
    void CheckCompileErrors(GLuint shader, std::string type);
};
