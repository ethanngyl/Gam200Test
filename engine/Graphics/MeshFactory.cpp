#include "Precompiled.h"

namespace Framework {

    Mesh* CreateTriangle() {
        std::vector<float> vertices = {
             // Position          // Color
             0.0f,  0.1f, 0.0f,   0.0f, 0.0f, 0.0f,  // red
            -0.1f, -0.1f, 0.0f,   0.0f, 0.0f, 0.0f,  // green
             0.1f, -0.1f, 0.0f,   0.0f, 0.0f, 1.0f   // blue
        };
        return new Mesh(vertices, GL_TRIANGLES);
    }

    Mesh* CreateQuad() {
        std::vector<float> vertices = {
            // pos.xyz           color.rgb       uv
            -1.0f, -1.0f, 0.0f,  1,1,1,          0.0f, 0.0f, // bottom-left
             1.0f, -1.0f, 0.0f,  1,1,1,          1.0f, 0.0f, // bottom-right
             1.0f,  1.0f, 0.0f,  1,1,1,          1.0f, 1.0f, // top-right

            -1.0f, -1.0f, 0.0f,  1,1,1,          0.0f, 0.0f, // bottom-left
             1.0f,  1.0f, 0.0f,  1,1,1,          1.0f, 1.0f, // top-right
            -1.0f,  1.0f, 0.0f,  1,1,1,          0.0f, 1.0f  // top-left
        };

        // pass hasTexCoords = true
        return new Mesh(vertices, GL_TRIANGLES, true);
    }


    Mesh* CreateLine() {
        std::vector<float> vertices = {
            // Position         // Color
           -0.5f, 0.0f, 0.0f,   1.0f, 0.0f, 1.0f,  // Magenta
            0.5f, 0.0f, 0.0f,   0.0f, 1.0f, 1.0f   // Cyan
        };
        return new Mesh(vertices, GL_LINES);
    }

    Mesh* CreateCircle(int segments, float radius) {
        std::vector<float> vertices;

        // Center of the circle (white)
        vertices.push_back(0.0f); // x
        vertices.push_back(0.0f); // y
        vertices.push_back(0.0f); // z
        vertices.push_back(1.0f); // r
        vertices.push_back(1.0f); // g
        vertices.push_back(1.0f); // b


        // Circle edge points with rainbow gradient
        for (int i = 0; i <= segments; ++i) {
            float angle = glm::two_pi<float>() * static_cast<float>(i) / segments;
            float x = radius * glm::cos(angle);
            float y = radius * glm::sin(angle);

            // Generate color based on angle (HSL to RGB approximation)
            float r = (glm::cos(angle) + 1.0f) / 2.0f;
            float g = (glm::cos(angle + glm::two_pi<float>() / 3.0f) + 1.0f) / 2.0f;
            float b = (glm::cos(angle + 2.0f * glm::two_pi<float>() / 3.0f) + 1.0f) / 2.0f;

            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(0.0f);
            vertices.push_back(r);
            vertices.push_back(g);
            vertices.push_back(b);
        }

        return new Mesh(vertices, GL_TRIANGLE_FAN);
    }

}
