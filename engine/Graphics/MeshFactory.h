#pragma once
#include "Precompiled.h"


namespace Framework {

    Mesh* CreateTriangle();
    Mesh* CreateQuad();
    Mesh* CreateLine();
    Mesh* CreateCircle(int segments, float radius);
}
