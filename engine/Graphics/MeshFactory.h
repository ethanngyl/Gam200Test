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

    /*
    ------------------------------------------------------------------------------
    CreateTriangle: Generates a simple Mesh representing a filled triangle.
    @return Pointer to the newly created Mesh object.
    ------------------------------------------------------------------------------
    */
    Mesh* CreateTriangle();

    /*
    ------------------------------------------------------------------------------
    CreateQuad: Generates a Mesh representing a unit quad (square) centered at
                the origin, typically used for sprites or backgrounds.
    @return Pointer to the newly created Mesh object.
    ------------------------------------------------------------------------------
    */
    Mesh* CreateQuad();

    /*
    ------------------------------------------------------------------------------
    CreateLine: Generates a Mesh representing a straight line segment.
    Commonly used for debugging and wireframe visualization.
    @return Pointer to the newly created Mesh object.
    ------------------------------------------------------------------------------
    */
    Mesh* CreateLine();

    /*
    ------------------------------------------------------------------------------
    CreateCircle: Generates a Mesh approximating a circle using a triangle fan.
    @param segments Number of segments used to approximate the circle.
    @param radius   Radius of the circle in world units.
    @return Pointer to the newly created Mesh object.
    ------------------------------------------------------------------------------
    */
    Mesh* CreateCircle(int segments, float radius);

}
