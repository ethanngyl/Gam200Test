#version 450 core

in vec2 TexCoord;
out vec4 FragColor;

layout (binding=0) uniform sampler2D uTexture;
uniform vec3 uColor;
uniform vec4 uUVRect;
uniform bool uUseAlphaDiscard; // New: control alpha-based discard

void main()
{
    vec2 uv = vec2(
        mix(uUVRect.x, uUVRect.z, TexCoord.x),
        mix(uUVRect.y, uUVRect.w, TexCoord.y)
    );

    vec4 texColor = texture(uTexture, uv);

    // Only discard based on alpha if uUseAlphaDiscard is true
    // This allows opaque rendering modes to show all pixels including black
    if (uUseAlphaDiscard && texColor.a < 0.1)
        discard;

    FragColor = texColor * vec4(uColor,1.0);
}
