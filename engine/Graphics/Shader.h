#pragma once
#include <string>
#include <unordered_map>
#include <../glm-src/glm/glm.hpp>

class Shader {
public:
	Shader() = default;
    Shader(const std::string& vertexPath, const std::string& fragmentPath);

    void Bind() const;
    void Unbind() const;

    // Uniform setters
    void SetUniform1i(const std::string& name, int value);
    void SetUniform1f(const std::string& name, float value);
    void SetUniformVec3(const std::string& name, const glm::vec3& value);
    void SetUniformMat4(const std::string& name, const glm::mat4& matrix);

private:
    unsigned int m_RendererID = 0; // OpenGL program ID

    std::string LoadFile(const std::string& path);
    unsigned int CompileShader(unsigned int type, const std::string& source);
    unsigned int CreateProgram(const std::string& vertexSrc, const std::string& fragmentSrc);
    int GetUniformLocation(const std::string& name);

    // Cache for uniform locations
    mutable std::unordered_map<std::string, int> m_UniformLocationCache;
};