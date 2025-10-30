#version 450 core

in vec3 vertexColor;
in vec2 TexCoord;

out vec4 FragColor;

layout (binding=0) uniform sampler2D uTexture;
uniform vec3 uColor;

void main() {
    vec4 texColor = texture(uTexture, TexCoord);
    FragColor = vec4(0.0, 1.0, 0.0, 1.0);
}