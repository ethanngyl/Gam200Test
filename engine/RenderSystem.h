#pragma once
#include "Interface.h"
#include "GLRenderer.h"

namespace Framework {
    class RenderSystem : public InterfaceSystem {
    public:
        RenderSystem() = default;
        ~RenderSystem() override = default;

        void Initialize() override;
        void Update(float dt) override;
        void SendEngineMessage(Message* message) override;

    private:
        GLRenderer renderer;
    };
}
