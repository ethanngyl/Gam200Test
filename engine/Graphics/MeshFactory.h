/*
===============================================================================
 File:          MeshFactory.h
 Author:        TAN WEI LEONG
 Email:         weileong.tan@digipen.edu
 Date:          2025-10-02
 Contribution:  100%
 ------------------------------------------------------------------------------
 Declaration of the MeshFactory functions for mesh creation.

 Description:
 -------------
 This file provides declarations for functions used in the creation of basic
 geometric meshes such as triangles, quads, lines, and circles. These meshes
 are intended to be rendered by the `GraphicsSystem`. The functions in this 
 header simplify the creation of these shapes for rendering in an OpenGL context.
 Each mesh creation function returns a pointer to a `Mesh` object, which can then
 be configured with appropriate transformations, shaders, and colors.

 Responsibilities:
 -----------------
 - `CreateTriangle()`: Creates a mesh object representing a triangle.
 - `CreateQuad()`: Creates a mesh object representing a square or rectangular quad.
 - `CreateLine()`: Creates a mesh object representing a line.
 - `CreateCircle(int segments, float radius)`: Creates a mesh object representing
   a circle, approximated by line segments. The number of segments and radius
   can be specified.

 Platform-specific Notes:
 -------------------------
 - This header is part of a graphics system that interacts with OpenGL.
 - The mesh creation functions return dynamically allocated `Mesh` objects, which
   will need to be managed (deallocated) by the calling system.
 - The functions assume that the necessary OpenGL context and resources (e.g., shaders)
   are already initialized when these meshes are created and used.

 Safety:
 --------
 - As part of a broader graphics system, the functions declared here do not include
   internal error handling for allocation or OpenGL issues. Error handling is expected
   to be managed elsewhere in the system.
 - This header is intended for use in systems that ensure the proper setup and teardown
   of OpenGL contexts, and functions like `CreateCircle()` should be used with the
   understanding that dynamic memory allocation is involved.
===============================================================================
*/

#pragma once
#include "Precompiled.h"  // Includes essential precompiled headers for the graphics system.

namespace Framework {

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