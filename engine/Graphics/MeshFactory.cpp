/*
===============================================================================
 File:          MeshFactory.cpp
 Author:        TAN WEI LEONG
 Email:         weileong.tan@digipen.edu
 Date:          2025-10-02
 Contribution:  100%
 ------------------------------------------------------------------------------
 Implementation of the MeshFactory functions for mesh creation.

 Description:
 -------------
 This file implements functions that create basic geometric meshes (triangle,
 quad, line, and circle) for the graphics system. These meshes are returned as
 dynamically allocated `Mesh` objects that are ready to be rendered by the
 graphics system using OpenGL.

 Each mesh creation function initializes a vector of vertices, which define
 the geometry and color of the shape. These vertices are then passed to the
 `Mesh` constructor to create the corresponding mesh for rendering.

 Responsibilities:
 -----------------
 - `CreateTriangle()`: Creates a triangle mesh with 3 vertices and color attributes.
 - `CreateQuad()`: Creates a quadrilateral mesh (square or rectangle) with 6 vertices and color attributes.
 - `CreateLine()`: Creates a simple line mesh with 2 vertices and color attributes.
 - `CreateCircle(int segments, float radius)`: Creates a circle mesh approximated with a given number of line segments.
   The color of the circle's edge is dynamically calculated with a rainbow gradient.

 Platform-specific Notes:
 -------------------------
 - This implementation assumes that OpenGL is correctly set up in the system.
 - The `Mesh` class used here is expected to handle the rendering of the mesh
   based on the passed vertex data and primitive type (e.g., `GL_TRIANGLES`, `GL_LINES`).
 - The `glm` library is used for mathematical operations such as trigonometric calculations.

 Safety:
 --------
 - Memory allocation is done dynamically when creating meshes. The caller
   must ensure proper memory management by deleting the meshes when they
   are no longer needed.
 - The color generation for the circle assumes a simple HSL-to-RGB color 
   conversion approach based on angle, which results in a smooth rainbow gradient.
===============================================================================
*/

#include "Precompiled.h"  // Includes necessary precompiled headers for the graphics system.

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
            // Position          // Color
            0.0f,  0.1f, 0.0f,   0.0f, 0.0f, 0.0f,  // Red vertex
           -0.1f, -0.1f, 0.0f,   0.0f, 0.0f, 0.0f,  // Green vertex
            0.1f, -0.1f, 0.0f,   0.0f, 0.0f, 1.0f   // Blue vertex
        };
        return new Mesh(vertices, GL_TRIANGLES);
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
        // Define vertices for a quad with positions and colors
        std::vector<float> vertices = {
            // Position           // Color
           -0.5f,  0.5f, 0.0f,    1.0f, 0.0f, 0.0f,  // Top Left - Red
            0.5f,  0.5f, 0.0f,    0.0f, 1.0f, 0.0f,  // Top Right - Green
           -0.5f, -0.5f, 0.0f,    0.0f, 0.0f, 1.0f,  // Bottom Left - Blue

            0.5f,  0.5f, 0.0f,    0.0f, 1.0f, 0.0f,  // Top Right - Green
            0.5f, -0.5f, 0.0f,    1.0f, 1.0f, 0.0f,  // Bottom Right - Yellow
           -0.5f, -0.5f, 0.0f,    0.0f, 0.0f, 1.0f   // Bottom Left - Blue
        };
        return new Mesh(vertices, GL_TRIANGLES);
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
            // Position         // Color
           -0.5f, 0.0f, 0.0f,   1.0f, 0.0f, 1.0f,  // Magenta color
            0.5f, 0.0f, 0.0f,   0.0f, 1.0f, 1.0f   // Cyan color
        };
        return new Mesh(vertices, GL_LINES);
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

        // Add the center of the circle (white color)
        vertices.push_back(0.0f); // x
        vertices.push_back(0.0f); // y
        vertices.push_back(0.0f); // z
        vertices.push_back(1.0f); // r
        vertices.push_back(1.0f); // g
        vertices.push_back(1.0f); // b

        // Add the circle edge points with a rainbow gradient
        for (int i = 0; i <= segments; ++i) {
            // Angle around the circle
            float angle = glm::two_pi<float>() * static_cast<float>(i) / segments;
            float x = radius * glm::cos(angle);  // X position on the circle
            float y = radius * glm::sin(angle);  // Y position on the circle

            // Generate color based on angle (simple HSL-to-RGB approximation)
            float r = (glm::cos(angle) + 1.0f) / 2.0f;
            float g = (glm::cos(angle + glm::two_pi<float>() / 3.0f) + 1.0f) / 2.0f;
            float b = (glm::cos(angle + 2.0f * glm::two_pi<float>() / 3.0f) + 1.0f) / 2.0f;

            // Add the vertex data for the edge of the circle
            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(0.0f);  // z position (flat circle)
            vertices.push_back(r);
            vertices.push_back(g);
            vertices.push_back(b);
        }

        // Return the mesh representing the circle
        return new Mesh(vertices, GL_TRIANGLE_FAN);
    }

}  // namespace Framework