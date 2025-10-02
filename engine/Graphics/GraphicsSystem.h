#pragma once
#include "Precompiled.h"
#include <glm/glm.hpp>
#include "ECSEntityManager.h"
// Forward declarations
struct GLFWwindow;

namespace Framework {
    class Shader;
    class Mesh;
    class Texture;
}

namespace Framework {
    class GraphicsSystem : public InterfaceSystem {
    public:
         GraphicsSystem();
        virtual ~GraphicsSystem();

        virtual void Initialize() override;
        virtual void Update(float dt) override;
        virtual void RenderEntities();
        virtual void SendEngineMessage(Message* message) override;
        void SetEntityManager(EntityManager* em) { entityManager = em; }
        void SetWindow(GLFWwindow* win) { window = win; }

    private:
        void BeginFrame();
        void EndFrame();
        void ProcessInput();
        void SetCurrentMeshColor();
        Mesh* GetMeshForSprite(const std::string& spriteName);

        GLFWwindow* window;

        Shader* shader;
        Mesh* triangleMesh;
        EntityManager* entityManager;
        std::vector<Mesh*> meshes;
        std::vector<glm::vec3> meshColors;
        int currentMeshIndex = 0;
        // Interpolation
        float colorLerpTime = 0.0f;
        float colorLerpSpeed = 0.25f;
        bool interpolateColor = true; // Toggle if you want

        Texture* backgroundTexture = nullptr;
        Mesh* backgroundQuad = nullptr;
    };
}