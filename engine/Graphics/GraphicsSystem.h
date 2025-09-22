#pragma once
#include "Interface.h"

// Forward declarations
struct GLFWwindow;

namespace Framework {
    class Shader;
    class Mesh;
}

namespace Framework {
    class GraphicsSystem : public InterfaceSystem {
    public:
        GraphicsSystem();
        virtual ~GraphicsSystem();

        virtual void Initialize() override;
        virtual void Update(float dt) override;
        virtual void SendMessage(Message* message) override;

        void SetWindow(GLFWwindow* window) { this->window = window; }

    private:
        void BeginFrame();
        void EndFrame();

        GLFWwindow* window;
        Shader* shader;
        Mesh* triangleMesh;
    };
}