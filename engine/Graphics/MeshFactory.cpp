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

    Mesh* CreateCircle(int segments, float radius) {
        std::vector<float> vertices;

        // Center of the circle
        vertices.push_back(0.0f); // x
        vertices.push_back(0.0f); // y
        vertices.push_back(0.0f); // z

        // Create the circle edge points
        for (int i = 0; i <= segments; ++i) {
            float angle = glm::two_pi<float>() * static_cast<float>(i) / segments;
            float x = radius * glm::cos(angle);
            float y = radius * glm::sin(angle);

            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(0.0f); // z
        }

        return new Mesh(vertices, GL_TRIANGLE_FAN);
    }

}
