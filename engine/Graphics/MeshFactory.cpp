#include "Precompiled.h"

/*
===============================================================================
File:        MeshFactory.cpp
Author:      Sim Kah Yan
Email:       kahyan.sim@digipen.edu
Date:        2025-10-02
Contribution: 10%(kah yan)
-------------------------------------------------------------------------------
Brief:
Implementation of mesh creation helper functions for generating basic geometric
primitives (triangle, quad, line, and circle) as Mesh objects. These are used
by the engine to create simple renderable shapes without external model files.

Details:
- Each function constructs vertex data (and indices where needed) with positions,
  per-vertex colors, and texture coordinates.
- Primitives are centered at the origin and use the y-up coordinate system.
- Circle meshes are generated procedurally using a triangle fan with rainbow colors.
- Mesh objects are created on the heap and returned to the caller.

Notes:
- Caller is responsible for managing the lifetime of returned Mesh pointers.
- Vertex format used: position (3), color (3), texcoord (2) = 8 floats per vertex.
- Used mainly during GraphicsSystem initialization to populate the mesh list.

Safety:
- All functions assume a valid OpenGL context for Mesh initialization.
- Returns valid Mesh pointers; caller must delete to avoid leaks.

===============================================================================
*/

namespace Framework {

    /**
    * @brief Creates a triangle mesh with 3 vertices and color attributes.
    *
    * This function creates a simple triangle using a set of 3 vertices. Each vertex
    * has an associated color. The triangle is rendered using the `GL_TRIANGLES`
    * OpenGL primitive type.
    *
    * @return A pointer to the created `Mesh` object representing a triangle.
    */
    Mesh* CreateTriangle() {
        // Define vertices for a triangle with positions and colors
        std::vector<float> vertices = {
            // pos         // color        // texcoord
             0.0f,  0.5f, 0.0f,  1.0f, 0.0f, 0.0f,  0.5f, 1.0f,  // top
            -0.5f, -0.5f, 0.0f,  0.0f, 1.0f, 0.0f,  0.0f, 0.0f,  // bottom left
             0.5f, -0.5f, 0.0f,  0.0f, 0.0f, 1.0f,  1.0f, 0.0f   // bottom right
        };

        return new Mesh(vertices, GL_TRIANGLES, true);
    }

    /**
     * @brief Creates a quad (square or rectangle) mesh with 6 vertices and color attributes.
     *
     * This function creates a quadrilateral mesh using 6 vertices. The quadrilateral is
     * split into two triangles. Each vertex has an associated position and color.
     * The mesh is rendered using the `GL_TRIANGLES` OpenGL primitive type.
     *
     * @return A pointer to the created `Mesh` object representing a quad.
     */
    Mesh* CreateQuad() {
        // Interleaved vertex data: pos, color, texcoord
        std::vector<float> vertices = {
            // pos             // color           // texcoord
            -0.5f, -0.5f, 0.0f,  1.0f, 0.0f, 0.0f,   0.0f, 0.0f, // bottom left
             0.5f, -0.5f, 0.0f,  0.0f, 1.0f, 0.0f,   1.0f, 0.0f, // bottom right
             0.5f,  0.5f, 0.0f,  0.0f, 0.0f, 1.0f,   1.0f, 1.0f, // top right
            -0.5f,  0.5f, 0.0f,  1.0f, 1.0f, 0.0f,   0.0f, 1.0f  // top left
        };

        // Two triangles forming a quad
        std::vector<unsigned int> indices = {
            0, 1, 2,  // first triangle
            2, 3, 0   // second triangle
        };

        // Attribute layout sizes: position(3), color(3), texcoord(2)
        std::vector<int> attribSizes = { 3, 3, 2 };

        // Create the mesh using indexed drawing
        Mesh* quad = new Mesh();
        quad->Initialize(vertices, indices, attribSizes);
        return quad;
    }

    /**
     * @brief Creates a line mesh with 2 vertices and color attributes.
     *
     * This function creates a simple line mesh by defining two vertices. Each vertex
     * has a position and color. The line is rendered using the `GL_LINES` OpenGL primitive type.
     *
     * @return A pointer to the created `Mesh` object representing a line.
     */
    Mesh* CreateLine() {
        // Define vertices for a line with positions and colors
        std::vector<float> vertices = {
            // pos          // color        // texcoord
            -0.5f, 0.0f, 0.0f,   1.0f, 0.0f, 1.0f,   0.0f, 0.0f,
             0.5f, 0.0f, 0.0f,   0.0f, 1.0f, 1.0f,   1.0f, 0.0f
        };

        return new Mesh(vertices, GL_LINES, true);
    }

    /**
     * @brief Creates a circle mesh approximated with line segments.
     *
     * This function creates a circle mesh using a given number of segments (lines).
     * The circle is approximated as a polygon with each segment representing a small
     * part of the circumference. The color of the circle's edge is generated dynamically
     * with a smooth rainbow gradient.
     *
     * @param segments The number of segments to approximate the circle.
     * @param radius The radius of the circle.
     *
     * @return A pointer to the created `Mesh` object representing a circle.
     */
    Mesh* CreateCircle(int segments, float radius) {
        std::vector<float> vertices;

        // center point
        vertices.insert(vertices.end(),
            { 0.0f, 0.0f, 0.0f,   1.0f, 1.0f, 1.0f,   0.5f, 0.5f });

        // perimeter points
        for (int i = 0; i <= segments; i++) {
            float theta = (2.0f * 3.1415926f * i) / segments;
            float x = radius * cos(theta);
            float y = radius * sin(theta);

            // rainbow colors for fun
            float r = (cos(theta) + 1.0f) * 0.5f;
            float g = (sin(theta) + 1.0f) * 0.5f;
            float b = 1.0f - r;

            // texcoords mapped into [0,1]
            float u = (x / radius + 1.0f) * 0.5f;
            float v = (y / radius + 1.0f) * 0.5f;

            vertices.insert(vertices.end(), { x, y, 0.0f,  r, g, b,  u, v });
        }

        return new Mesh(vertices, GL_TRIANGLE_FAN, true);
    }

}  // namespace Framework