/*
===============================================================================
File:        basic2.frag
Author:      Sim Kah Yan
Email:       kahyan.sim@digipen.edu
Date:        2026-02-04 (yyyy-mm-dd)
Contribution: 100%
-------------------------------------------------------------------------------
Brief:
Fragment shader for simple textured quads: samples uTexture, discards fully
transparent pixels (alpha == 0), outputs texColor with no tinting.

Details:
- In: vertexColor, TexCoord. Uniform: uTexture (binding 0). Samples texture at
  TexCoord; if texColor.a == 0.0 then discard; else FragColor = texColor.
  No uColor or UV rect; used for passes that need raw texture only.

Notes:
- Alternative to basic.frag when tint/UV rect/grayscale/alpha discard control
  are not needed. Referenced as "color" shader in GraphicsSystemV2 (Shader2).

Safety:
- Standard texture sample and discard; no extra clamping.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
===============================================================================
*/
#version 450 core

in vec3 vertexColor;
in vec2 TexCoord;

out vec4 FragColor;

layout(binding = 0) uniform sampler2D uTexture;

void main()
{
    //FragColor = texture(uTexture, TexCoord);  // just the image

    vec4 texColor = texture(uTexture, TexCoord);
    
    // Discard transparent pixels
    if (texColor.a == 0.0)
       discard;
    
    FragColor = texColor;  // No tinting, just the texture
}