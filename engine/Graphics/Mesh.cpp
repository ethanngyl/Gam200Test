#include "Mesh.h"
#include <glad/glad.h>
#include <iostream>

namespace Framework {

    Mesh::Mesh(const std::vector<float>& vertices)
        : VAO(0), VBO(0), vertices(vertices)
    {
        vertexCount = static_cast<unsigned int>(vertices.size() / 3); // Fixed conversion

        std::cout << "Creating mesh with " << vertexCount << " vertices\n";

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        Bind();

        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float),
            vertices.data(), GL_DYNAMIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        Unbind();

        // Check for errors
        GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
            std::cerr << "OpenGL error in Mesh constructor: " << error << "\n";
        }

        std::cout << "Mesh created successfully\n";
    }

    Mesh::~Mesh() {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
    }

    void Mesh::Draw() const {
        Bind();
        glDrawArrays(GL_TRIANGLES, 0, vertexCount);

        // Check for drawing errors
        GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
            std::cerr << "OpenGL error in Mesh::Draw: " << error << "\n";
        }

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

    void Mesh::UpdateVertices(const std::vector<float>& newVertices) {
        if (newVertices.size() != vertices.size()) {
            std::cerr << "Mesh: Cannot update vertices - size mismatch. "
                << "Expected: " << vertices.size()
                << ", Got: " << newVertices.size() << "\n";
            return;
        }

        vertices = newVertices;
        Bind();
        glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(float), vertices.data());
        Unbind();
    }
}