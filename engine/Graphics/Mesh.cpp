/*
===============================================================================
 File:          Mesh.cpp
 Author:        TAN WEI LEONG
 Email:         weileong.tan@digipen.edu
 Date:          2025-10-02
 Contribution:  100%
 ------------------------------------------------------------------------------
 Implementation of the Mesh class, which encapsulates the OpenGL logic required
 to render mesh objects with vertex data.

 Description:
 -------------
 This file implements the methods of the `Mesh` class, responsible for managing 
 vertex data and interacting with OpenGL to render 3D (or 2D) objects. The class 
 handles creating OpenGL buffers (VAO and VBO), updating vertices, and drawing the 
 mesh on screen. This code also ensures proper resource management (buffer deletion, 
 etc.) to prevent memory leaks.

 Responsibilities:
 -----------------
 - `Mesh()`: Initializes a mesh with vertex data and an optional drawing mode.
 - `~Mesh()`: Cleans up OpenGL buffers when the mesh is destroyed.
 - `Draw()`: Renders the mesh to the screen using OpenGL commands.
 - `UpdateVertices()`: Updates the vertex data of the mesh in OpenGL.
 - `Bind()`: Binds the mesh's OpenGL resources (VAO and VBO) for rendering.
 - `Unbind()`: Unbinds the OpenGL resources after rendering.

 Platform-specific Notes:
 -------------------------
 - The class uses OpenGL (with GLEW) for rendering and assumes the necessary 
   OpenGL setup is done in the main application.
 - Ensure proper OpenGL context setup before using the `Mesh` class.

 Safety:
 --------
 - Proper OpenGL resource cleanup is ensured with the destructor to avoid memory 
   leaks.
 - Buffer updates and draws are wrapped in functions that bind/unbind OpenGL 
   resources safely.
===============================================================================
*/

#include "Precompiled.h"  // Includes necessary precompiled headers for graphics system.

namespace Framework {

    /**
     * @brief Constructs a Mesh object with provided vertex data.
     *
     * The constructor generates and initializes OpenGL resources (Vertex Array Object
     * and Vertex Buffer Object) with the given vertex data. The vertex data is expected
     * to contain positions and colors for each vertex. The drawing mode (GL_TRIANGLES by
     * default) determines how the mesh is rendered (as triangles, lines, etc.).
     *
     * @param vertices A vector of floats containing vertex data (position + color).
     * @param drawMode The OpenGL drawing mode (default is GL_TRIANGLES).
     */
    Mesh::Mesh(const std::vector<float>& vertices, GLenum drawMode)
        : VAO(0), VBO(0), vertices(vertices), drawMode(drawMode) {

        // Each vertex consists of 6 floats: 3 for position, 3 for color
        vertexCount = static_cast<unsigned int>(vertices.size() / 6);

        // Generate OpenGL Vertex Array Object (VAO) and Vertex Buffer Object (VBO)
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

    /**
     * @brief Destructor for the Mesh class.
     *
     * Ensures that the OpenGL buffers (VAO and VBO) are properly deleted when
     * the Mesh object is destroyed, preventing memory leaks.
     */
    Mesh::~Mesh() {
        // Delete the OpenGL buffers (VAO and VBO) associated with this mesh
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
    }

    /**
     * @brief Renders the mesh to the screen.
     *
     * This function binds the VAO and VBO, issues an OpenGL draw call using the specified
     * drawing mode (e.g., GL_TRIANGLES), and then unbinds the OpenGL resources after
     * rendering to ensure the mesh is drawn correctly.
     */
    void Mesh::Draw() const {
        // Bind the mesh resources for rendering
        Bind();

        // Execute the draw call (using the specified draw mode)
        glDrawArrays(drawMode, 0, vertexCount);  // Draw vertices starting from index 0

        // Unbind after drawing
        Unbind();
    }

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

    /**
     * @brief Binds the OpenGL resources (VAO and VBO).
     *
     * This function binds the vertex array object (VAO) and vertex buffer object (VBO)
     * to the OpenGL context, making them active for subsequent drawing operations.
     */
    void Mesh::Bind() const {
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
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
    }
}