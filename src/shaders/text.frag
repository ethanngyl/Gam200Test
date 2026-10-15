/*
===============================================================================
File:        text.frag
Author:      Sim Kah Yan
Email:       kahyan.sim@digipen.edu
Date:        2026-02-04 (yyyy-mm-dd)
Contribution: 100%
-------------------------------------------------------------------------------
Brief:
Fragment shader for text rendering: samples a single-channel (GL_RED) glyph
texture, uses red as alpha, and multiplies by uTextColor for final RGBA output.

Details:
- In: vUV. Uniforms: uTexture (binding 0, single-channel glyph atlas), uTextColor (vec3).
  alpha = texture(uTexture, vUV).r; FragColor = vec4(uTextColor, alpha).
  Glyph atlas is typically GL_RED; .r gives the mask/alpha for the character.

Notes:
- Used by TextRenderer (FreeType-based) for drawing text quads. Blending
  (SRC_ALPHA, ONE_MINUS_SRC_ALPHA) should be enabled for correct font edges.

Safety:
- UV and sampler used as provided; alpha from single channel.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
===============================================================================
*/
#version 450 core

in vec2 vUV;

layout (location = 0) out vec4 FragColor;

layout (binding = 0) uniform sampler2D uTexture;     // single-channel (RED) glyph texture
uniform vec3      uTextColor;

void main()
{
    // Glyph atlas is uploaded as GL_RED; we read the red channel as alpha.
    float alpha = texture(uTexture, vUV).r;
    FragColor = vec4(uTextColor, alpha);
}