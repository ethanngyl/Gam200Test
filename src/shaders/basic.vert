#version 450 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aColor;
layout(location=2) in vec2 aTexCoord;

// AMD FIX: Use vertex attributes for instance matrices instead of SSBO
// This is the standard OpenGL approach and works across all GPU vendors
layout(location=3) in vec4 aInstanceModel0;  // First column of model matrix
layout(location=4) in vec4 aInstanceModel1;  // Second column
layout(location=5) in vec4 aInstanceModel2;  // Third column
layout(location=6) in vec4 aInstanceModel3;  // Fourth column

//uniform mat4 uModel;
uniform mat4 uProjection;
uniform mat4 uView;

out vec3 vertexColor;
out vec2 TexCoord;

void main() {
    // Reconstruct the model matrix from the 4 vec4 attributes
    mat4 instanceModel = mat4(
        aInstanceModel0,
        aInstanceModel1,
        aInstanceModel2,
        aInstanceModel3
    );

    gl_Position = uProjection * uView * instanceModel * vec4(aPos, 1.0);
    vertexColor = aColor;
    TexCoord = aTexCoord;
}