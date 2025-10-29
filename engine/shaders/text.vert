#version 450 core

layout (location = 0) in vec4 aVertex;   // x, y, u, v

uniform mat4 uProjection;

out vec2 vUV;

void main()
{
    gl_Position = uProjection * vec4(aVertex.xy, 0.0, 1.0);
    vUV = aVertex.zw;
}