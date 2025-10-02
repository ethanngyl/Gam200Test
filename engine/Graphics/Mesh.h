#pragma once
#include "Precompiled.h"
#include "GL/glew.h"
#include "GL/gl.h"
/*
===============================================================================
File:        Mesh.h
Author:      Sim Kah Yan
Email:       kahyan.sim@digipen.edu
Date:        2025-10-02
Contribution: 30%(kah yan)
-------------------------------------------------------------------------------
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
- Proper cleanup of GPU resources in the destructor.
- No heap allocations; only GPU buffers are managed.
- All OpenGL calls assume a valid rendering context.

===============================================================================
*/

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
        Mesh(const std::vector<float>& vertices, GLenum drawMode = GL_TRIANGLES, bool hasTexCoords = false);
       
        /*
        ------------------------------------------------------------------------------
        Default Constructor:
        Creates an empty Mesh object that can later be initialized via Initialize().
        Useful for indexed meshes (e.g., quads) that require more flexible setup.
        ------------------------------------------------------------------------------
        */
        // Empty constructor for later Initialize()
        Mesh();

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
        // Draw call
        void Draw() const;

        /*
        ------------------------------------------------------------------------------
        UpdateVertices:
        Replaces the vertex buffer data with a new set of vertices. Useful for
        dynamic meshes that need to change shape at runtime (e.g., deforming objects).
        ------------------------------------------------------------------------------
        */
        // Update vertex buffer (for dynamic meshes)
        void UpdateVertices(const std::vector<float>& newVertices);

        /*
        ------------------------------------------------------------------------------
        Initialize:
        Flexible setup function for indexed meshes. Takes a vertex array, index array,
        and a list of attribute sizes (e.g., {3,3,2} for position, color, texcoord).
        Sets up VAO, VBO, and EBO accordingly.
        ------------------------------------------------------------------------------
        */
        // Flexible initialization (indexed + attribute sizes)
        void Initialize(const std::vector<float>& vertices,
            const std::vector<unsigned int>& indices,
            const std::vector<int>& attribSizes);

        /*
        ------------------------------------------------------------------------------
        GetVertexCount:
        Returns the number of vertices stored in the mesh.
        ------------------------------------------------------------------------------
        */
        unsigned int GetVertexCount() const { return vertexCount; }

    private:
        // Helper functions to bind/unbind VAO during drawing
        void Bind() const;
        void Unbind() const;

        // OpenGL buffer handles
        GLuint VAO = 0, VBO = 0, EBO = 0;// Vertex Array Object, Buffer Object, and Element Buffer Object (optional)
        
        // Mesh data
        std::vector<float> vertices;
        unsigned int vertexCount = 0;
        unsigned int indexCount = 0;
        GLenum drawMode = GL_TRIANGLES;
        bool hasTexCoords = false;
        bool useIndices = false; // Flag to determine glDrawArrays vs glDrawElements
    };

}
