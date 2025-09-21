#pragma once

#include "Shader.h"

class Renderer {
public:
    void Init();
    void Render();
    void Cleanup();

private:
    unsigned int VAO, VBO;
    Shader* shader = nullptr;
};
