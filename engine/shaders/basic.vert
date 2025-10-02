#version 450 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;

uniform mat4 uModel; 
uniform mat4 uProjection;

out vec3 vertexColor;

void main() {
    gl_Position = uProjection * uModel * vec4(aPos, 1.0);
    vertexColor = aColor;
}
