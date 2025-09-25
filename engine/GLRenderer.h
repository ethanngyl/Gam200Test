#ifndef GLRENDERER_H
#define GLRENDERER_H

#include "Platform.h"
#include "Graphics/Shader.h"

class GLRenderer {
public:
    GLRenderer();
    ~GLRenderer();

    bool initialize();
    void render();
    void cleanup();

private:
    bool createBuffers();

    // Modern Shader wrapper
    Shader shader;

    // Buffers
    GLuint vbo;
    GLuint vao;
};

#endif // GLRENDERER_H
