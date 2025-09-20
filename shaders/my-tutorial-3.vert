#version 450 core

layout(location = 0) in vec2 aVertexPosition;
layout(location = 1) in vec3 aVertexColor;

out vec3 fragColor;

uniform bool uUseInstancing;
uniform mat3 uModel_to_NDC;

struct InstanceData {
    vec3 translation;
    float w;
    float rotation;
    vec3 scale;
    float pad;
};

layout(std430, binding = 0) buffer TranslationBuffer {
    InstanceData translations[];
};

mat3 computeModelToNDC(InstanceData data) {
    float c = cos(data.rotation);
    float s = sin(data.rotation);

    mat3 S = mat3(data.scale.x, 0.0, 0.0,
                  0.0, data.scale.y, 0.0,
                  0.0, 0.0, 1.0);

    mat3 R = mat3(c, s, 0.0,
                 -s, c, 0.0,
                  0.0, 0.0, 1.0);

    mat3 T = mat3(1.0, 0.0, 0.0,
                  0.0, 1.0, 0.0,
                  data.translation.xy, 1.0);

    float w = 2.0 / 1280.0;
    float h = 2.0 / 720.0;
    mat3 toNDC = mat3(w, 0.0, 0.0,
                      0.0, h, 0.0,
                      0.0, 0.0, 1.0);

    return toNDC * T * R * S;
}


void main() {
    mat3 modelToNDC;
    if (uUseInstancing) {
        modelToNDC = computeModelToNDC(translations[gl_InstanceID]);
    } else {
        modelToNDC = uModel_to_NDC;
    }

    vec3 pos = modelToNDC * vec3(aVertexPosition, 1.0);
    gl_Position = vec4(pos.xy, 0.0, 1.0);
    fragColor = aVertexColor;
}