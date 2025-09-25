#include "Precompiled.h"
#include "RenderSystem.h"
#include "Message.h"
#include <iostream>

namespace Framework {

    void RenderSystem::Initialize() {
        std::cout << "RenderSystem: Initializing...\n";
        if (!renderer.initialize()) {
            std::cerr << "RenderSystem: Failed to initialize GLRenderer!\n";
        }
    }

    void RenderSystem::Update(float dt) {
        std::cout << "[RenderSystem] Calling render()" << std::endl;
        renderer.render();
    }

    void RenderSystem::SendEngineMessage(Message* message) {
        if (message->MessageId == Status::Quit) {
            std::cout << "RenderSystem: Received quit, cleaning up...\n";
            renderer.cleanup();
        }
    }
}
