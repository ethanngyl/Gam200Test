#version 450 core
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D uTexture;   // <--- new
uniform vec3 uColor;          // still keep your mesh color

void main() {
    vec4 texColor = texture(uTexture, TexCoord);
    FragColor = texColor * vec4(uColor, 1.0); // combine texture with color
}
