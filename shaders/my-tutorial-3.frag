// Fragment shader code goes here...

#version 450 core

in vec3 fragColor;
out vec4 fFragColor;

void main() {
    fFragColor = vec4(fragColor, 1.0);
}
