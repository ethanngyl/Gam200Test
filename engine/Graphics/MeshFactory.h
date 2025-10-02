#pragma once
#include "Precompiled.h"


namespace Framework {
    class Mesh;

    Mesh* CreateTriangle();
    Mesh* CreateQuad();
    Mesh* CreateLine();
    Mesh* CreateCircle(int segments, float radius);

}
