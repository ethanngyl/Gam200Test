/*
===============================================================================
File:        basic.frag
Author:      Sim Kah Yan
Email:       kahyan.sim@digipen.edu
Date:        2026-02-04 (yyyy-mm-dd)
Contribution: 100%
-------------------------------------------------------------------------------
Brief:
Fragment shader for basic 2D textured quads: samples uTexture with UV rect,
applies tint (uColor), optional grayscale (uGrayAmount), alpha discard, and
force-opaque-alpha for textures with zero alpha.

Details:
- In: TexCoord. Uniforms: uTexture (binding 0), uColor, uUVRect (xy=min, zw=max),
  uUseAlphaDiscard (discard if alpha < 0.1), uForceOpaqueAlpha (set a=1),
  uGrayAmount (0=normal, 1=grayscale via luminance dot).
- UV computed by mixing uUVRect with TexCoord; sample texture; apply grayscale
  mix; optionally force alpha to 1.0; discard low alpha when enabled; output
  texColor * vec4(uColor, 1.0).

Notes:
- Used by GraphicsSystemV2 for sprites and batched draws. Alpha discard
  disabled in opaque blend mode so black pixels are visible.

Safety:
- uGrayAmount clamped to [0,1]. uUVRect and sampler used as provided.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
===============================================================================
*/
#version 450 core

in vec2 TexCoord;
out vec4 FragColor;

layout (binding=0) uniform sampler2D uTexture;
uniform vec4 uColor;
uniform vec4 uUVRect;
uniform bool uUseAlphaDiscard;  // Control alpha-based discard
uniform bool uForceOpaqueAlpha; // Force alpha to 1.0, ignore texture alpha
uniform bool uUseTexture;       // false = no texture, render solid color from uColor
uniform float uGrayAmount;      // 0 = normal, 1 = grayscale

void main()
{
    // No texture bound - render as a solid colored shape using uColor
    if (!uUseTexture) {
        FragColor = uColor;
        return;
    }

    vec2 uv = vec2(
        mix(uUVRect.x, uUVRect.z, TexCoord.x),
        mix(uUVRect.y, uUVRect.w, TexCoord.y)
    );

    vec4 texColor = texture(uTexture, uv);

    float gray = dot(texColor.rgb, vec3(0.299, 0.587, 0.114));
    texColor.rgb = mix(texColor.rgb, vec3(gray), clamp(uGrayAmount, 0.0, 1.0));

    // Force alpha to fully opaque if requested (fixes black pixels with alpha=0)
    if (uForceOpaqueAlpha) {
        texColor.a = 1.0;
    }

    // Only discard based on alpha if uUseAlphaDiscard is true
    // This allows opaque rendering modes to show all pixels including black
    if (uUseAlphaDiscard && texColor.a < 0.1)
        discard;

    FragColor = texColor * uColor;
}
