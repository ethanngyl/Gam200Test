#version 450 core

in vec2 TexCoord;
out vec4 FragColor;

layout (binding=0) uniform sampler2D uTexture;
uniform vec3 uColor;
uniform vec4 uUVRect;
uniform bool uUseAlphaDiscard;  // Control alpha-based discard
uniform bool uForceOpaqueAlpha; // Force alpha to 1.0, ignore texture alpha
uniform float uGrayAmount;      // 0 = normal, 1 = grayscale

void main()
{
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

    FragColor = texColor * vec4(uColor,1.0);
}
