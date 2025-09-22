#pragma once
#include <vector>

namespace Framework {
    class Mesh {
    public:
        Mesh(const std::vector<float>& vertices);
        ~Mesh();

        void Draw() const;
        void Bind() const;
        void Unbind() const;
        void UpdateVertices(const std::vector<float>& newVertices);

    private:
        unsigned int VAO, VBO;
        unsigned int vertexCount;
        std::vector<float> vertices;
    };
}