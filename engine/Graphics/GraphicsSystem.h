#pragma once
#include "Interface.h"
#include <glm/glm.hpp>

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
        virtual void SendEngineMessage(Message* message) override;

        void SetWindow(GLFWwindow* window) { this->window = window; }

    private:
        void BeginFrame();
        void EndFrame();
        void ProcessInput();

        void SetCurrentMeshColor();

        GLFWwindow* window;

        Shader* shader;

        Mesh* triangleMesh;
        std::vector<Mesh*> meshes;
        std::vector<glm::vec3> meshColors;
        int currentMeshIndex = 0;

        // Interpolation
        float colorLerpTime = 0.0f;
        float colorLerpSpeed = 0.25f;
        bool interpolateColor = true; // Toggle if you want
    };
}