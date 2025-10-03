#include "Precompiled.h"
#include "Mesh.h"
#include <iostream>
/*
===============================================================================
File:        Mesh.cpp
Author:      TAN WEI LEONG
co-Author:   Sim Kah Yan
Email:       weileong.tan@digipen.edu, kahyan.sim@digipen.edu
Date:        2025-10-02
Contribution: 70%(TAN WEI LEONG), 30%(Sim Kah Yan)
-------------------------------------------------------------------------------
Brief:
Implementation of the Mesh class, which encapsulates OpenGL buffer objects and
vertex array setup for both indexed and non-indexed geometry. This class allows
easy creation, updating, and drawing of renderable primitives using VAOs, VBOs,
and optional EBOs.

Details:
- The quick constructor supports non-indexed meshes for simple shapes (e.g., triangle, line).
- The Initialize() method sets up indexed meshes with flexible attribute layouts.
- Meshes support runtime vertex buffer updates through UpdateVertices().
- Draw() binds the VAO and issues the correct draw call (arrays or elements).
- Proper cleanup of GPU buffers is performed in the destructor.

Notes:
- Vertex attributes are expected to be interleaved.
- Attribute locations follow this convention:
    0 → Position (x,y,z)
    1 → Color (r,g,b)
    2 → TexCoords (u,v) [optional]
- Requires a valid OpenGL context before construction or drawing.

Safety:
- Checks for buffer existence before deletion.
- UpdateVertices() performs a size check to avoid buffer overruns.
- No heap allocations; only GPU buffers are managed.

===============================================================================
*/
namespace Framework {

    /*
    ------------------------------------------------------------------------------
    Constructor (Non-indexed):
    Initializes a VAO and VBO for a mesh defined only by vertices.
    This is used for simple primitives (triangle, line, circle).
    Texcoords can be included if present.
    ------------------------------------------------------------------------------
    */
    // ---------------- Non-indexed constructor ----------------
    Mesh::Mesh(const std::vector<float>& vertices, GLenum drawMode, bool hasTexCoords)
        : VAO(0), VBO(0), EBO(0), vertice(vertices),
        drawMode(drawMode), hasTexCoords(hasTexCoords), useIndices(false)
    {
        // Determine vertex stride (pos+color=6, pos+color+tex=8)
        int stride = hasTexCoords ? 8 : 6;
        vertexCount = static_cast<unsigned int>(vertices.size() / stride);

        // Generate and bind VAO + VBO
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        // Bind VAO and VBO to set up their configurations
        Bind();
        // Upload vertex data to GPU
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
        // Unbind to avoid accidental modifications
        Unbind();
    }

    // ---------------- Empty constructor for Initialize() ----------------
    Mesh::Mesh()
        : VAO(0), VBO(0), EBO(0), vertexCount(0), indexCount(0),
        drawMode(GL_TRIANGLES), hasTexCoords(false), useIndices(false)
    {
    }

    // ---------------- Destructor ----------------
    Mesh::~Mesh() {
        if (VAO) glDeleteVertexArrays(1, &VAO);
        if (VBO) glDeleteBuffers(1, &VBO);
        if (EBO) glDeleteBuffers(1, &EBO);
    }

    // ---------------- Initialize with indices ----------------
    void Mesh::Initialize(const std::vector<float>& vertices,
        const std::vector<unsigned int>& indices,
        const std::vector<int>& attribSizes)
    {
        this->vertice = vertices;
        vertexCount = static_cast<unsigned int>(vertices.size());
        indexCount = static_cast<unsigned int>(indices.size());
        useIndices = true;

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);

        // VBO
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

        // EBO
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

        // Attribute setup
        int stride = 0;
        for (int size : attribSizes) stride += size;
        stride *= sizeof(float);

        size_t offset = 0;
        for (GLuint i = 0; i < attribSizes.size(); i++) {
            glVertexAttribPointer(i, attribSizes[i], GL_FLOAT, GL_FALSE, stride, (void*)offset);
            glEnableVertexAttribArray(i);
            offset += attribSizes[i] * sizeof(float);
        }

        glBindVertexArray(0);
    }

    // ---------------- Draw ----------------
    void Mesh::Draw() const {
        // Bind the mesh resources for rendering
        Bind();
        if (useIndices)
            glDrawElements(drawMode, indexCount, GL_UNSIGNED_INT, 0);
        else
            glDrawArrays(drawMode, 0, vertexCount);
        Unbind();
    }

    // ---------------- Update vertices ----------------
    /**
     * @brief Updates the vertex data for the mesh.
     *
     * This function updates the vertex buffer with new vertex data. The new data must
     * have the same size as the original vertex data. It is efficient because it
     * only updates the existing buffer rather than recreating it.
     *
     * @param newVertices A vector of floats containing the new vertex data. This should
     *                    contain positions and colors for each vertex in the same format
     *                    as the original data.
     */
    void Mesh::UpdateVertices(const std::vector<float>& newVertices) {
        // Ensure the new vertex data matches the size of the original data
        if (newVertices.size() != vertice.size()) {
            std::cerr << "Mesh::UpdateVertices: size mismatch\n";
            return;  // If size doesn't match, exit the function
        }

        // Update the vertices with the new data
        vertice = newVertices;

        // Bind the VAO and VBO to update the buffer data
        Bind();
        glBufferSubData(GL_ARRAY_BUFFER, 0, vertice.size() * sizeof(float), vertice.data());
        
        // Unbind after updating to avoid accidental changes
        Unbind();
    }

    /**
     * @brief Binds the OpenGL resources (VAO and VBO).
     *
     * This function binds the vertex array object (VAO) and vertex buffer object (VBO)
     * to the OpenGL context, making them active for subsequent drawing operations.
     */
    void Mesh::Bind() const {
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        if (useIndices) glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    }


    /**
     * @brief Unbinds the OpenGL resources (VAO and VBO).
     *
     * This function unbinds the VAO and VBO, ensuring that no OpenGL state changes
     * accidentally affect other parts of the program.
     */
    void Mesh::Unbind() const {
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        if (useIndices) glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }

}
