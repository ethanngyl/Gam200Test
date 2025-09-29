#pragma once
#include "Interface.h"

// Forward declaration
struct GLFWwindow;

namespace Framework {
    class WindowSystem : public InterfaceSystem
    {
    public:
        WindowSystem();
        virtual ~WindowSystem();

        // ISystem interface
        virtual void Initialize() override;
        virtual void Update(float dt) override;
        virtual void SendEngineMessage(Message* message) override;

        GLFWwindow* GetWindow() const { return window; }
        bool ShouldClose() const;

    private:
        GLFWwindow* window;
        bool WindowOpen;
    };
}
