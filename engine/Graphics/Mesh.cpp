#include "Precompiled.h"
#include "Mesh.h"
#include <iostream>

namespace Framework {

    Mesh::Mesh(const std::vector<float>& vertices, GLenum drawMode, bool hasTexCoords)
        : VAO(0), VBO(0), vertices(vertices), drawMode(drawMode), hasTexCoords(hasTexCoords)
    {
        int stride = hasTexCoords ? 8 : 6;
        vertexCount = static_cast<unsigned int>(vertices.size() / stride);

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        Bind();

        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float),
            vertices.data(), GL_STATIC_DRAW);

        // Position attribute (location = 0)
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        // Color attribute (location = 1)
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        if (hasTexCoords) {
            // TexCoord attribute (location = 2)
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride * sizeof(float), (void*)(6 * sizeof(float)));
            glEnableVertexAttribArray(2);
        }

        Unbind();
    }

    Mesh::~Mesh() {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
    }

    void Mesh::Draw() const {
        Bind();
        glDrawArrays(drawMode, 0, vertexCount);
        Unbind();
    }

    void Mesh::UpdateVertices(const std::vector<float>& newVertices) {
        if (newVertices.size() != vertices.size()) {
            std::cerr << "Mesh::UpdateVertices: size mismatch\n";
            return;
        }

        vertices = newVertices;
        Bind();
        glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(float), vertices.data());
        Unbind();
    }

    void Mesh::Bind() const {
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
    }

    void Mesh::Unbind() const {
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

}
