#include "MeshFactory.h"

namespace Framework {

    Mesh* CreateTriangle() {
        std::vector<float> vertices = {
             0.0f,  0.8f, 0.0f,
            -0.8f, -0.8f, 0.0f,
             0.8f, -0.8f, 0.0f
        };
        return new Mesh(vertices, GL_TRIANGLES);
    }

    Mesh* CreateQuad() {
        std::vector<float> vertices = {
            // Triangle 1
            -0.5f,  0.5f, 0.0f,
             0.5f,  0.5f, 0.0f,
            -0.5f, -0.5f, 0.0f,
            // Triangle 2
             0.5f,  0.5f, 0.0f,
             0.5f, -0.5f, 0.0f,
            -0.5f, -0.5f, 0.0f
        };
        return new Mesh(vertices, GL_TRIANGLES);
    }

    Mesh* CreateLine() {
        std::vector<float> vertices = {
            -0.5f, 0.0f, 0.0f,
             0.5f, 0.0f, 0.0f
        };
        return new Mesh(vertices, GL_LINES);
    }

}
