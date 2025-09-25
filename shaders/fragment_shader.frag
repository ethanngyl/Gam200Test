#version 330 core

// Input from vertex shader
in vec3 fragColor;

// Final output color
out vec4 outColor;

void main() {
    outColor = vec4(fragColor, 1.0);
}
