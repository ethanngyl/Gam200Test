/*
===============================================================================
File:        basic.vert
Author:      Sim Kah Yan, Ethan Ng 
Email:       kahyan.sim@digipen.edu, n.ethanyongle@digipen.edu
Date:        2026-02-04 (yyyy-mm-dd)
Contribution: 80% (Sim Kah Yan), 20% (Ethan Ng)
-------------------------------------------------------------------------------
Brief:
Vertex shader for basic 2D textured/colored quads. Per-vertex attributes
(pos, color, texCoord); instance matrix passed as four vec4 attributes (AMD-
compatible); outputs clip position and varyings for the fragment stage.

Details:
- In: aPos (vec3), aColor (vec3), aTexCoord (vec2); aInstanceModel0..3 (vec4)
  for instanced model matrix (avoids SSBO for broad GPU compatibility).
- Uniforms: uProjection, uView. Reconstructs mat4 from the four instance
  columns; gl_Position = uProjection * uView * instanceModel * vec4(aPos, 1.0).
  Out: vertexColor, TexCoord.

Notes:
- AMD fix: instance data via vertex attributes instead of SSBO. Used with
  basic.frag by GraphicsSystemV2 for batched sprite/quad drawing.

Safety:
- No divergent control flow; attributes and uniforms used as provided.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
===============================================================================
*/
#version 450 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aColor;
layout(location=2) in vec2 aTexCoord;

// AMD FIX: Use vertex attributes for instance matrices instead of SSBO
// This is the standard OpenGL approach and works across all GPU vendors
layout(location=3) in vec4 aInstanceModel0;  // First column of model matrix
layout(location=4) in vec4 aInstanceModel1;  // Second column
layout(location=5) in vec4 aInstanceModel2;  // Third column
layout(location=6) in vec4 aInstanceModel3;  // Fourth column

//uniform mat4 uModel;
uniform mat4 uProjection;
uniform mat4 uView;

out vec3 vertexColor;
out vec2 TexCoord;

void main() {
    // Reconstruct the model matrix from the 4 vec4 attributes
    mat4 instanceModel = mat4(
        aInstanceModel0,
        aInstanceModel1,
        aInstanceModel2,
        aInstanceModel3
    );

    gl_Position = uProjection * uView * instanceModel * vec4(aPos, 1.0);
    vertexColor = aColor;
    TexCoord = aTexCoord;
}