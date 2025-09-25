#pragma once
#include "Precompiled.h"
#include "GL/glew.h"
#include "GL/gl.h"

namespace Framework {

    class Mesh {
    public:
        Mesh(const std::vector<float>& vertices, GLenum drawMode = GL_TRIANGLES);
        ~Mesh();

        void Draw() const;
        void UpdateVertices(const std::vector<float>& newVertices);
        void Bind() const;
        void Unbind() const;

        unsigned int GetVertexCount() const { return vertexCount; }

    private:
        GLuint VAO, VBO;
        std::vector<float> vertices;
        unsigned int vertexCount;
        GLenum drawMode;
    };

}