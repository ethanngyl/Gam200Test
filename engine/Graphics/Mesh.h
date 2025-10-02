#pragma once
#include "Precompiled.h"
#include "GL/glew.h"
#include "GL/gl.h"

namespace Framework {

    class Mesh {
    public:
        // Constructor for quick non-indexed meshes
        Mesh(const std::vector<float>& vertices, GLenum drawMode = GL_TRIANGLES, bool hasTexCoords = false);

        // Empty constructor for later Initialize()
        Mesh();
        ~Mesh();

        // Draw call
        void Draw() const;

        // Update vertex buffer (for dynamic meshes)
        void UpdateVertices(const std::vector<float>& newVertices);

        // Flexible initialization (indexed + attribute sizes)
        void Initialize(const std::vector<float>& vertices,
            const std::vector<unsigned int>& indices,
            const std::vector<int>& attribSizes);

        unsigned int GetVertexCount() const { return vertexCount; }

    private:
        void Bind() const;
        void Unbind() const;

        GLuint VAO = 0, VBO = 0, EBO = 0;
        std::vector<float> vertices;
        unsigned int vertexCount = 0;
        unsigned int indexCount = 0;
        GLenum drawMode = GL_TRIANGLES;
        bool hasTexCoords = false;
        bool useIndices = false; // flag to check if EBO was set
    };

}
