#pragma once
#include "Precompiled.h"
/*
===============================================================================
File:        MeshFactory.h
Author:      TAN WEI LEONG
co-Author:   Sim Kah Yan
Email:       weileong.tan@digipen.edu, kahyan.sim@digipen.edu
Date:        2025-10-02
Contribution: 90%(TAN WEI LEONG), 10%(kah yan)
-------------------------------------------------------------------------------
Brief:
Declaration of mesh creation helper functions that generate basic geometric
primitives (triangle, quad, line, and circle) as Mesh objects. These functions
are used to create simple renderable shapes at engine initialization.

Details:
- Provides functions that allocate and initialize Mesh objects for basic shapes.
- Each mesh is configured with appropriate vertex positions, colors, and UVs.
- Useful for testing, debugging, and rendering basic geometry without importing
  external model files.

Notes:
- The caller is responsible for managing the lifetime of the returned Mesh
  pointers (they are allocated on the heap).
- Coordinate system used is y-up (OpenGL standard).
- Circle generation uses a configurable segment count to approximate the shape.

Safety:
- Functions return nullptr if mesh creation fails.
- Ensure proper deletion of returned Mesh objects to avoid memory leaks.

===============================================================================
*/

namespace Framework {
    // Forward declaration to avoid circular include
    class Mesh;

    /**
    * @brief Creates a mesh object representing a triangle.
    *
    * This function returns a dynamically allocated `Mesh` representing a triangle
    * with pre-defined vertices and other attributes suitable for rendering.
    *
    * @return A pointer to the created `Mesh` object representing a triangle.
    */
    class MeshFactory {
    public:
        void MeshValueInitialize();
        Mesh* CreateTriangle();

        /**
         * @brief Creates a mesh object representing a quad (square or rectangle).
         *
         * This function returns a dynamically allocated `Mesh` representing a quadrilateral
         * (a square or rectangle), with pre-defined vertices and attributes.
         *
         * @return A pointer to the created `Mesh` object representing a quad.
         */
        Mesh* CreateQuad();

        /**
         * @brief Creates a mesh object representing a line.
         *
         * This function returns a dynamically allocated `Mesh` representing a simple line,
         * defined by two points in space. The mesh can be used for rendering basic line
         * segments in 2D or 3D space.
         *
         * @return A pointer to the created `Mesh` object representing a line.
         */
        Mesh* CreateLine();

        /**
         * @brief Creates a mesh object representing a circle.
         *
         * This function returns a dynamically allocated `Mesh` representing a circle,
         * approximated using line segments. The number of segments defines how smooth the
         * circle appears, and the radius specifies the size of the circle.
         *
         * @param segments The number of line segments to approximate the circle.
         * @param radius The radius of the circle.
         *
         * @return A pointer to the created `Mesh` object representing a circle.
         */
        Mesh* CreateCircle(int segments, float radius);

        Mesh* CreateWireframeQuad();
    private:
        float blv_x;
        float blv_y;
        float blv_z;
        float blv_r;
        float blv_g;
        float blv_b;
        float blv_u;
        float blv_v;
        float blr_x;
        float blr_y;
        float blr_z;
        float blr_r;
        float blr_g;
        float blr_b;
        float blr_u;
        float blr_v;
        float trv_x;
        float trv_y;
        float trv_z;
        float trv_r;
        float trv_g;
        float trv_b;
        float trv_u;
        float trv_v;
        float tlv_x;
        float tlv_y;
        float tlv_z;
        float tlv_r;
        float tlv_g;
        float tlv_b;
        float tlv_u;
        float tlv_v;
        float sp_x;
        float sp_y;
        float sp_z;
        float sp_r;
        float sp_g;
        float sp_b;
        float sp_u;
        float sp_v;
        float ep_x;
        float ep_y;
        float ep_z;
        float ep_r;
        float ep_g;
        float ep_b;
        float ep_u;
        float ep_v;
    };
} // namespace Framework