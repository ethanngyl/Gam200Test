#pragma once
#include "Mesh.h"
#include <glm/gtc/constants.hpp>
#include <glm/trigonometric.hpp>

namespace Framework {

    Mesh* CreateTriangle();
    Mesh* CreateQuad();
    Mesh* CreateLine();
    Mesh* CreateCircle(int segments, float radius);
}
