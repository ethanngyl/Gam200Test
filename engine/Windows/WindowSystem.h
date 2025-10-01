#pragma once
#include "Precompiled.h"

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

        // new setters
        void SetWindowSize(int w, int h) { windowWidth = w; windowHeight = h; }
        void SetWindowTitle(const std::string& title) { windowTitle = title; }

    private:
        GLFWwindow* window;
        bool WindowOpen;

        // new config variables
        int windowWidth;
        int windowHeight;
        std::string windowTitle;
    };
}
