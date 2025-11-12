#version 450 core

in vec2 TexCoord;
out vec4 FragColor;

layout (binding=0) uniform sampler2D uTexture;
uniform vec3 uColor;
uniform vec4 uUVRect;

void main()
{
    vec2 uv = vec2(
        mix(uUVRect.x, uUVRect.z, TexCoord.x),
        mix(uUVRect.y, uUVRect.w, TexCoord.y)
    );
    
    vec4 texColor = texture(uTexture, uv);

    // Discard fully black pixels
    if (texColor.r == 0.0 && texColor.g == 0.0 && texColor.b == 0.0)
    discard;

    if (texColor.a < 0.1)
    discard;

    FragColor = texColor * vec4(uColor,1.0);
}
