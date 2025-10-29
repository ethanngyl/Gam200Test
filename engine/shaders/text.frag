#version 450 core

in vec2 vUV;

layout (location = 0) out vec4 FragColor;

uniform sampler2D uText;     // single-channel (RED) glyph texture
uniform vec3      uTextColor;

void main()
{
    // Glyph atlas is uploaded as GL_RED; we read the red channel as alpha.
    float alpha = texture(uText, vUV).r;
    FragColor = vec4(uTextColor, alpha);
}