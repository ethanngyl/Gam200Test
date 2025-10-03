/*
===============================================================================
 File:          Mesh.h
 Author:        Sim Kah Yan, TAN WEI LEONG
 Email:         kahyan.sim@digipen.edu, weileong.tan@digipen.edu
 Date:          2025-10-02
 Contribution:  30%(kah yan), 70%(TAN WEI LEONG)
 ------------------------------------------------------------------------------
 Brief:
 Declaration of the Mesh class, which encapsulates OpenGL Vertex Array Objects
 (VAOs), Vertex Buffer Objects (VBOs), and optional Element Buffer Objects (EBOs)
 to represent renderable geometric shapes.

 This class provides functionality for creating, updating, and drawing both
 indexed and non-indexed meshes with optional texture coordinates.

 Details:
 - Supports construction of simple non-indexed meshes (e.g., triangle, line, circle)
   through the quick constructor.
 - Allows flexible initialization for indexed meshes using vertex attributes and
   index buffers (e.g., for quads).
 - Provides runtime vertex buffer updates for dynamic meshes.
 - Encapsulates OpenGL resource management (VAO, VBO, EBO) to simplify rendering.


 Notes:
- Vertex format is expected to be interleaved (e.g., position, color, texcoord).
- Attribute sizes must match the vertex layout when using Initialize().
- The Draw() method automatically binds the VAO and issues glDrawArrays or
  glDrawElements depending on whether indices were set.

 Safety:
 --------
 - Proper cleanup of GPU resources in the destructor.
 - No heap allocations; only GPU buffers are managed.
 - All OpenGL calls assume a valid rendering context.
===============================================================================
*/

#pragma once
#include "Precompiled.h"  // Includes precompiled headers necessary for the graphics system.
#include "GL/glew.h"      // OpenGL Extension Wrangler (GLEW) for managing OpenGL extensions.
#include "GL/gl.h"        // Core OpenGL functions and constants.

namespace Framework {


    class Mesh {
    public:
        /*
        ------------------------------------------------------------------------------
        Constructor: Quick constructor for non-indexed meshes.
        Creates and uploads a vertex buffer with the given interleaved vertex data.
        Used by simple shapes like triangles, lines, or circles.
        ------------------------------------------------------------------------------
        */
        // Constructor for quick non-indexed meshes
        Mesh(const std::vector<float>& vertices, GLenum drawMode = GL_TRIANGLES);

        /*
        ------------------------------------------------------------------------------
        Destructor:
        Releases OpenGL buffers (VAO, VBO, EBO) to avoid GPU memory leaks.
        ------------------------------------------------------------------------------
        */
        ~Mesh();

        /*
        ------------------------------------------------------------------------------
        Draw:
        Binds the VAO and issues a draw call using glDrawArrays or glDrawElements
        depending on whether indices were provided.
        ------------------------------------------------------------------------------
        */
        void Draw() const;

        /*
        ------------------------------------------------------------------------------
        UpdateVertices:
        Replaces the vertex buffer data with a new set of vertices. Useful for
        dynamic meshes that need to change shape at runtime (e.g., deforming objects).
        ------------------------------------------------------------------------------
        */
        void UpdateVertices(const std::vector<float>& newVertices);

        // Helper functions to bind/unbind VAO during drawing
        void Bind() const;

        void Unbind() const;

        // Mesh data
        unsigned int GetVertexCount() const { return vertexCount; }

    private:
        GLuint VAO;                  ///< The OpenGL Vertex Array Object used to store state.
        GLuint VBO;                  ///< The OpenGL Vertex Buffer Object used to store vertex data.
        std::vector<float> vertices; ///< The vertex data for the mesh (position, color, etc.).
        unsigned int vertexCount;    ///< The number of vertices in the mesh.
        GLenum drawMode;             ///< The OpenGL drawing mode (e.g., GL_TRIANGLES, GL_LINES).
    };

}  // namespace Framework