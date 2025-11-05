#version 450 core

in vec2 vWorld;   // from vert
in vec2 vUV;      // background image UV

out vec4 FragColor;

layout(binding = 0) uniform sampler2D uTexture;

// --- grid controls (you already set most of these in SetupBackground) ---
uniform int   uShowGrid      = 1;          // 1 = draw grid
uniform vec2  uCellSize      = vec2(1.0);   // world units per cell (x,y)
uniform float uLineWidth     = 0.02;        // thin line width (world units)
uniform vec4  uGridColor     = vec4(0.0, 0.0, 0.0, 0.85);

uniform int   uMajorEvery    = 5;           // every N cells, draw a major line
uniform float uMajorWidth    = 0.04;        // major line width (world units)
uniform vec4  uMajorColor    = vec4(0.0, 0.0, 0.0, 1.0);

// optional tint used by your material system; set to (1,1,1) for none
uniform vec3  uColor         = vec3(1.0);

void main()
{
    // --- base: draw background image ---
    vec4 base = texture(uTexture, vUV) * vec4(uColor, 1.0);

    if (uShowGrid == 0) {
        FragColor = base;
        return;
    }

    // Distance in world units to the nearest thin grid line on X/Y
    vec2 r  = mod(vWorld, uCellSize);
    vec2 d  = min(r, uCellSize - r);               // distance to nearest grid line

    if(d.x < uLineWidth || d.y < uLineWidth)
        FragColor = vec4(1.0, 0.0, 0.0, 1.0);
    else
        FragColor = base;

    //float thinMask = float(min(d.x, d.y) < 0.5 * uLineWidth);
    //
    //// Major grid: just use a bigger cell = uCellSize * uMajorEvery
    //vec2 bigCell = uCellSize * float(max(uMajorEvery, 1));
    //vec2 rM = mod(vWorld, bigCell);
    //vec2 dM = min(rM, bigCell - rM);
    //float majorMask = float(min(dM.x, dM.y) < 0.5 * uMajorWidth);
    //
    //// Combine: major overrides thin
    //float drawMask = max(thinMask, majorMask);
    //vec4  gridCol  = mix(uGridColor, uMajorColor, majorMask);
    //
    //// Composite grid over the image (respect alpha in grid color)
    //FragColor = mix(base, gridCol, drawMask * gridCol.a);
}
