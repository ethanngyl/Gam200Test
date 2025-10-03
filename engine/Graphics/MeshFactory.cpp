/*
===============================================================================
 File:          MeshFactory.cpp
 Author:        Sim Kah Yan, TAN WEI LEONG
 Email:         kahyan.sim@digipen.edu, weileong.tan@digipen.edu
 Date:          2025-10-02
 Contribution:  10%(kah yan), 90%(TAN WEI LEONG)
 ------------------------------------------------------------------------------
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

#include "Precompiled.h"  // Includes necessary precompiled headers for the graphics system.

namespace Framework {

    // =====================================================
  // Create a simple triangle (position + color + texcoord)
  // =====================================================
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

    // =====================================================
    // Create a quad using indices (position + color + texcoord)
    // =====================================================
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

    // =====================================================
    // Create a line (two points, with color + dummy texcoords)
    // =====================================================
    Mesh* CreateLine() {
        // Define vertices for a line with positions and colors
        std::vector<float> vertices = {
            // Position         // Color
           -0.5f, 0.0f, 0.0f,   1.0f, 0.0f, 1.0f,  // Magenta color
            0.5f, 0.0f, 0.0f,   0.0f, 1.0f, 1.0f   // Cyan color
        };
        return new Mesh(vertices, GL_LINES);
    }

    // =====================================================
    // Create a circle (triangle fan, with color + texcoords)
    // =====================================================
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