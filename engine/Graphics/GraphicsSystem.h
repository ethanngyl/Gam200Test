#pragma once
#include "Interface.h"
#include "Shader.h"

class Shader;
struct GLFWwindow;

namespace Framework {

    class GraphicsSystem : public InterfaceSystem {
    public:
        GraphicsSystem();
        virtual ~GraphicsSystem();
          
        // ISystem interface
        virtual void Initialize() override;
        virtual void Update(float dt) override;
        virtual void SendMessage(Message* message) override;

        void BeginFrame();
        void EndFrame();
        bool ShouldClose() const;

    private:
        GLFWwindow* window;
        Shader* shader;
        unsigned int VAO, VBO;
    };

}
