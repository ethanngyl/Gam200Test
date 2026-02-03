/*
===============================================================================
File:        text.vert
Author:      Sim Kah Yan
Email:       kahyan.sim@digipen.edu
Date:        2026-02-04 (yyyy-mm-dd)
Contribution: 100%
-------------------------------------------------------------------------------
Brief:
Vertex shader for text rendering: single vec4 attribute (xy = position, zw = UV),
orthographic uProjection; outputs clip position and vUV for glyph sampling.

Details:
- In: aVertex (vec4) — .xy = position, .zw = texture coordinates. Uniform:
  uProjection (orthographic, screen space). gl_Position = uProjection * vec4(aVertex.xy, 0.0, 1.0);
  vUV = aVertex.zw. Used with text.frag for drawing text quads.

Notes:
- Paired with TextRenderer and text.frag. Origin typically bottom-left;
  projection set up for screen-space text in GraphicsSystemV2/TextRenderer.

Safety:
- No divergent control flow; attribute and uniform used as provided.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
===============================================================================
*/
#version 450 core

layout (location = 0) in vec4 aVertex;   // x, y, u, v

uniform mat4 uProjection;

out vec2 vUV;

void main()
{
    gl_Position = uProjection * vec4(aVertex.xy, 0.0, 1.0);
    vUV = aVertex.zw;
}