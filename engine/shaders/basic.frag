#version 450 core

in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D uTexture;
uniform vec3 uColor;

uniform float u0;
uniform float v0;
uniform float u1;
uniform float v1;

void main()
{
    vec2 uv = vec2(
        mix(u0, u1, TexCoord.x),
        mix(v0, v1, TexCoord.y)
    );

    vec4 texColor = texture(uTexture, uv);
    FragColor = texColor * vec4(uColor, 1.0);
}
