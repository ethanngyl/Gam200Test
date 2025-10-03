#pragma once
#include "Precompiled.h"
/*
===============================================================================
File:        MeshFactory.h
Author:      Sim Kah Yan
Email:       kahyan.sim@digipen.edu
Date:        2025-10-02
Contribution: 10%(kah yan)
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

} // namespace Framework