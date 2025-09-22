#pragma once
#include <string>

namespace Framework {

    class Shader {
    public:
        Shader(const std::string& vertexPath, const std::string& fragmentPath);
        ~Shader();

        void Bind() const;
        void Unbind() const;

    private:
        unsigned int id;

        std::string LoadFile(const std::string& path);
        unsigned int Compile(unsigned int type, const std::string& source);
    };

}
