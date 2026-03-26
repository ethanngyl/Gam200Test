#include "Precompiled.h"
#include "ConfigReader.h"
#include "MeshFactory.h"
/*
===============================================================================
File:        MeshFactory.cpp
Author:      TAN WEI LEONG
co-Author:   Sim Kah Yan
Email:       weileong.tan@digipen.edu, kahyan.sim@digipen.edu
Date:        2025-10-02
Contribution: 90%(TAN WEI LEONG), 10%(kah yan)
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
    void MeshFactory::MeshValueInitialize() {
        ConfigReader::LoadConfig(
            ConfigReader::GetProjectPath("value_loader", "assets/valueloader.txt"));
        blv_x = ConfigReader::GetFloat("blv_x", 0);
        blv_y = ConfigReader::GetFloat("blv_y", 0);
        blv_z = ConfigReader::GetFloat("blv_z", 0);
        blv_r = ConfigReader::GetFloat("blv_r", 0);
        blv_g = ConfigReader::GetFloat("blv_g", 0);
        blv_b = ConfigReader::GetFloat("blv_b", 0);
        blv_u = ConfigReader::GetFloat("blv_u", 0);
        blv_v = ConfigReader::GetFloat("blv_v", 0);
        blr_x = ConfigReader::GetFloat("blr_x", 0);
        blr_y = ConfigReader::GetFloat("blr_y", 0);
        blr_z = ConfigReader::GetFloat("blr_z", 0);
        blr_r = ConfigReader::GetFloat("blr_r", 0);
        blr_g = ConfigReader::GetFloat("blr_g", 0);
        blr_b = ConfigReader::GetFloat("blr_b", 0);
        blr_u = ConfigReader::GetFloat("blr_u", 0);
        blr_v = ConfigReader::GetFloat("blr_v", 0);
        trv_x = ConfigReader::GetFloat("trv_x", 0);
        trv_y = ConfigReader::GetFloat("trv_y", 0);
        trv_z = ConfigReader::GetFloat("trv_z", 0);
        trv_r = ConfigReader::GetFloat("trv_r", 0);
        trv_g = ConfigReader::GetFloat("trv_g", 0);
        trv_b = ConfigReader::GetFloat("trv_b", 0);
        trv_u = ConfigReader::GetFloat("trv_u", 0);
        trv_v = ConfigReader::GetFloat("trv_v", 0);
        tlv_x = ConfigReader::GetFloat("tlv_x", 0);
        tlv_y = ConfigReader::GetFloat("tlv_y", 0);
        tlv_z = ConfigReader::GetFloat("tlv_z", 0);
        tlv_r = ConfigReader::GetFloat("tlv_r", 0);
        tlv_g = ConfigReader::GetFloat("tlv_g", 0);
        tlv_b = ConfigReader::GetFloat("tlv_b", 0);
        tlv_u = ConfigReader::GetFloat("tlv_u", 0);
        tlv_v = ConfigReader::GetFloat("tlv_v", 0);
        sp_x = ConfigReader::GetFloat("sp_x", 0);
        sp_y = ConfigReader::GetFloat("sp_y", 0);
        sp_z = ConfigReader::GetFloat("sp_z", 0);
        sp_r = ConfigReader::GetFloat("sp_r", 0);
        sp_g = ConfigReader::GetFloat("sp_g", 0);
        sp_b = ConfigReader::GetFloat("sp_b", 0);
        sp_u = ConfigReader::GetFloat("sp_u", 0);
        sp_v = ConfigReader::GetFloat("sp_v", 0);
        ep_x = ConfigReader::GetFloat("ep_x", 0);
        ep_y = ConfigReader::GetFloat("ep_y", 0);
        ep_z = ConfigReader::GetFloat("ep_z", 0);
        ep_r = ConfigReader::GetFloat("ep_r", 0);
        ep_g = ConfigReader::GetFloat("ep_g", 0);
        ep_b = ConfigReader::GetFloat("ep_b", 0);
        ep_u = ConfigReader::GetFloat("ep_u", 0);
        ep_v = ConfigReader::GetFloat("ep_v", 0);
    }
    //To remove**
    Mesh* MeshFactory::CreateTriangle() {
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
    Mesh* MeshFactory::CreateQuad() {
        // Interleaved vertex data: pos, color, texcoord
        std::vector<float> vertices = {
            // pos             // color           // texcoord
            blv_x, blv_y, blv_z,  blv_r, blv_g, blv_b,   blv_u, blv_v, // bottom left
             blr_x, blr_y, blr_z,  blr_r, blr_g, blr_b,   blr_u, blr_v, // bottom right
             trv_x,  trv_y, trv_z,  trv_r, trv_g, trv_b,   trv_u, trv_v, // top right
             tlv_x,  tlv_y, tlv_z,  tlv_r, tlv_g, tlv_b,   tlv_u, tlv_v  // top left
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
    Mesh* MeshFactory::CreateLine() {
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
    Mesh* MeshFactory::CreateCircle(int segments, float radius) {
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

    Mesh* MeshFactory::CreateWireframeQuad() {
        std::vector<float> vertices = {
            // pos             // color           // texcoord
            -0.5f, -0.5f, 0.0f,  1.0f, 1.0f, 1.0f,   0.0f, 0.0f, // bottom left
             0.5f, -0.5f, 0.0f,  1.0f, 1.0f, 1.0f,   1.0f, 0.0f, // bottom right
             0.5f,  0.5f, 0.0f,  1.0f, 1.0f, 1.0f,   1.0f, 1.0f, // top right
            -0.5f,  0.5f, 0.0f,  1.0f, 1.0f, 1.0f,   0.0f, 1.0f  // top left
        };

        // Only the 4 edges - no diagonals!
        std::vector<unsigned int> indices = {
            0, 1,  // bottom edge
            1, 2,  // right edge
            2, 3,  // top edge
            3, 0   // left edge
        };

        std::vector<int> attribSizes = { 3, 3, 2 };

        Mesh* wireframe = new Mesh();
        wireframe->Initialize(vertices, indices, attribSizes);
        // Make sure this draws as GL_LINES, not GL_TRIANGLES
        return wireframe;
    }

}  // namespace Framework