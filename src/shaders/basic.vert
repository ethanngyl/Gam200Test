#version 450 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aColor;
layout(location=2) in vec2 aTexCoord;

layout(binding = 3, std430) readonly buffer ssbo1 
{
    mat4 aInstanceModel[];
};

//uniform mat4 uModel;
uniform mat4 uProjection;
uniform mat4 uView;

out vec3 vertexColor;
out vec2 TexCoord;

void main() {
    gl_Position = uProjection * uView * aInstanceModel[gl_InstanceID] * vec4(aPos, 1.0);
    vertexColor = aColor;
    TexCoord = aTexCoord;
}