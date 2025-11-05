#version 450 core

const vec2 cIdxPos[4] = vec2[4](
    vec2(-0.5, -0.5),
    vec2( 0.5, -0.5),
    vec2( 0.5, 0.5),
    vec2(-0.5,  0.5)
);

out vec2 vWorld;   // world position (xy)
out vec2 vUV;       // uv

uniform vec2 uCamDim;
uniform vec2 uCamPos;

void main()
{
    vec2 vPos = cIdxPos[gl_VertexID];

    vWorld = vPos * uCamDim + uCamPos;
    vUV = vPos + vec2(0.5);
    gl_Position = vec4(vPos * 2.0, 0.0, 1.0);
}
