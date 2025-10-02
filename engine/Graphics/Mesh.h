/*
===============================================================================
 File:          Mesh.h
 Author:        TAN WEI LEONG
 Email:         weileong.tan@digipen.edu
 Date:          2025-10-02
 Contribution:  100%
 ------------------------------------------------------------------------------
 Declaration of the Mesh class, which represents a graphical mesh object for
 rendering in OpenGL.

 Description:
 -------------
 This file declares the `Mesh` class, which encapsulates the OpenGL logic needed
 to create, manage, and render 3D (or 2D) mesh objects. The `Mesh` class stores 
 vertex data, manages buffers, and provides methods for rendering and updating
 vertices. This class is essential for rendering graphical objects such as 
 triangles, quads, lines, and circles in the graphics system.

 Responsibilities:
 -----------------
 - `Mesh()`: Constructor that initializes the mesh with vertex data and an optional
   OpenGL draw mode. It sets up necessary OpenGL buffers for efficient rendering.
 - `~Mesh()`: Destructor that releases OpenGL resources (such as the Vertex Array
   Object and Vertex Buffer Object) when the mesh is destroyed.
 - `Draw()`: Renders the mesh by binding the appropriate OpenGL buffers and calling
   OpenGL drawing commands.
 - `UpdateVertices()`: Updates the vertex data of the mesh, reallocating buffers as needed.
 - `Bind()`: Binds the mesh's OpenGL buffers (VAO and VBO) to be used for rendering.
 - `Unbind()`: Unbinds the mesh's OpenGL buffers, ensuring they are not used by other
   OpenGL calls.
 - `GetVertexCount()`: Returns the number of vertices currently stored in the mesh.

 Platform-specific Notes:
 -------------------------
 - This class uses OpenGL (with GLEW) for rendering, so ensure that the system
   is correctly set up with OpenGL support.
 - The `GL/glew.h` and `GL/gl.h` headers are required for the necessary OpenGL
   functions and constants.
 - The `Mesh` class assumes that the `vertices` data passed to the constructor
   is a flat array of floating-point numbers, where each vertex has position 
   and color (or other attributes), depending on the specific mesh being created.

 Safety:
 --------
 - The destructor ensures proper cleanup of OpenGL resources to avoid memory leaks.
 - The class handles OpenGL resource binding/unbinding safely to prevent conflicts
   during rendering.
===============================================================================
*/

#pragma once
#include "Precompiled.h"  // Includes precompiled headers necessary for the graphics system.
#include "GL/glew.h"      // OpenGL Extension Wrangler (GLEW) for managing OpenGL extensions.
#include "GL/gl.h"        // Core OpenGL functions and constants.

namespace Framework {

    /**
    * @brief Represents a mesh in OpenGL with vertex data and a drawing mode.
    *
    * The `Mesh` class manages vertex data and OpenGL buffers necessary to render
    * 3D (or 2D) objects. It is used to create various shapes such as triangles,
    * quads, and circles, and it provides methods for updating vertex data,
    * drawing, and managing OpenGL resources efficiently.
    */
    class Mesh {
    public:
        /**
        * @brief Constructor for initializing a mesh with vertex data.
        *
        * The constructor initializes the mesh by generating a Vertex Array Object (VAO)
        * and a Vertex Buffer Object (VBO) in OpenGL. It also accepts a draw mode to
        * determine how the mesh should be rendered (default is `GL_TRIANGLES`).
        *
        * @param vertices A vector of floats containing the vertex data (position, color, etc.).
        * @param drawMode The OpenGL drawing mode (e.g., `GL_TRIANGLES`, `GL_LINES`).
        */
        Mesh(const std::vector<float>& vertices, GLenum drawMode = GL_TRIANGLES);

        /**
         * @brief Destructor to clean up OpenGL resources.
         *
         * This destructor releases the resources (VAO and VBO) allocated for the mesh
         * when the mesh is no longer needed.
         */
        ~Mesh();

        /**
        * @brief Draws the mesh to the screen.
        *
        * This method binds the mesh's VAO and VBO, then issues the appropriate OpenGL
        * command to render the mesh. The exact rendering behavior depends on the draw mode.
        */
        void Draw() const;

        /**
         * @brief Updates the vertex data of the mesh.
         *
         * This method allows the mesh's vertex data to be updated by reallocating
         * and re-uploading the new vertex buffer to OpenGL. It can be used for
         * dynamic meshes that change over time.
         *
         * @param newVertices The new vertex data to replace the existing mesh data.
         */
        void UpdateVertices(const std::vector<float>& newVertices);

        /**
         * @brief Binds the mesh's OpenGL buffers for use in rendering.
         *
         * This method binds the Vertex Array Object (VAO) and Vertex Buffer Object (VBO)
         * to the current OpenGL context, preparing them for rendering.
         */
        void Bind() const;

        /**
         * @brief Unbinds the mesh's OpenGL buffers after use.
         *
         * This method unbinds the mesh's VAO and VBO, ensuring that no OpenGL state
         * remains bound to the mesh when it is no longer in use.
         */
        void Unbind() const;

        /**
         * @brief Gets the number of vertices in the mesh.
         *
         * This method returns the number of vertices currently stored in the mesh.
         * It is useful for determining the size of the mesh and how many vertices
         * need to be processed for rendering.
         *
         * @return The number of vertices in the mesh.
         */
        unsigned int GetVertexCount() const { return vertexCount; }

    private:
        GLuint VAO;                  ///< The OpenGL Vertex Array Object used to store state.
        GLuint VBO;                  ///< The OpenGL Vertex Buffer Object used to store vertex data.
        std::vector<float> vertices; ///< The vertex data for the mesh (position, color, etc.).
        unsigned int vertexCount;    ///< The number of vertices in the mesh.
        GLenum drawMode;             ///< The OpenGL drawing mode (e.g., GL_TRIANGLES, GL_LINES).
    };

}  // namespace Framework