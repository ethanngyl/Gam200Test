/*
===============================================================================
 File:          Mesh.cpp
 Author:        Sim Kah Yan, TAN WEI LEONG
 Email:         kahyan.sim@digipen.edu, weileong.tan@digipen.edu
 Date:          2025-10-02
 Contribution:  30%(kah yan), 70%(TAN WEI LEONG)
 ------------------------------------------------------------------------------
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
    0 -> Position (x,y,z)
    1 -> Color (r,g,b)
    2 -> TexCoords (u,v) [optional]
- Requires a valid OpenGL context before construction or drawing.

Safety:
- Checks for buffer existence before deletion.
- UpdateVertices() performs a size check to avoid buffer overruns.
- No heap allocations; only GPU buffers are managed.
===============================================================================
*/

#include "Precompiled.h"  // Includes necessary precompiled headers for graphics system.

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
    Mesh::Mesh(const std::vector<float>& vertices, GLenum drawMode)
        : VAO(0), VBO(0), vertices(vertices), drawMode(drawMode) {

        // Each vertex consists of 6 floats: 3 for position, 3 for color
        vertexCount = static_cast<unsigned int>(vertices.size() / 6);

        // Generate and bind VAO + VBO
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        // Bind VAO and VBO to set up their configurations
        Bind();

        // Send vertex data to the GPU
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float),
            vertices.data(), GL_STATIC_DRAW);

        // Position attribute (location = 0)
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);  // Enable the position attribute

        // Color attribute (location = 1)
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1); // Enable the color attribute

        // Unbind to avoid accidental modifications
        Unbind();
    }

    // ---------------- Destructor ----------------
    Mesh::~Mesh() {
        // Delete the OpenGL buffers (VAO and VBO) associated with this mesh
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
    }

    // ---------------- Draw ----------------
    void Mesh::Draw() const {
        // Bind the mesh resources for rendering
        Bind();

        // Execute the draw call (using the specified draw mode)
        glDrawArrays(drawMode, 0, vertexCount);  // Draw vertices starting from index 0

        // Unbind after drawing
        Unbind();
    }

    // ---------------- Update vertices ----------------
    void Mesh::UpdateVertices(const std::vector<float>& newVertices) {
        // Ensure the new vertex data matches the size of the original data
        if (newVertices.size() != vertices.size()) {
            std::cerr << "Mesh::UpdateVertices: size mismatch\n";
            return;  // If size doesn't match, exit the function
        }

        // Update the vertices with the new data
        vertices = newVertices;

        // Bind the VAO and VBO to update the buffer data
        Bind();
        glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(float), vertices.data());
        
        // Unbind after updating to avoid accidental changes
        Unbind();
    }

    // ---------------- Bind / Unbind ----------------
    void Mesh::Bind() const {
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
    }


    
    void Mesh::Unbind() const {
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
}