#pragma once
#include "Interface.h"
#include <vector>

namespace Framework
{
    // Simple color structure
    struct Color
    {
        float r, g, b, a;
        Color(float red = 1.0f, float green = 1.0f, float blue = 1.0f, float alpha = 1.0f)
            : r(red), g(green), b(blue), a(alpha) {
        }
    };

    // Simple 2D position
    struct Vector2
    {
        float x, y;
        Vector2(float xPos = 0.0f, float yPos = 0.0f) : x(xPos), y(yPos) {}
    };

    // Basic shape types
    enum ShapeType
    {
        RECTANGLE,
        CIRCLE,
        TRIANGLE
    };

    // Simple drawable shape
    struct Shape
    {
        ShapeType type;
        Vector2 position;
        Vector2 size;      // width/height for rectangle, radius for circle
        Color color;

        Shape(ShapeType t, Vector2 pos, Vector2 sz, Color col)
            : type(t), position(pos), size(sz), color(col) {
        }
    };

    class GraphicsSystem : public ISystem
    {
    public:
        GraphicsSystem();
        virtual ~GraphicsSystem();

        // ISystem interface
        virtual void Initialize() override;
        virtual void Update(float dt) override;
        virtual void SendEngineMessage(Message* message) override;

        // Graphics functions
        void AddShape(const Shape& shape);
        void ClearShapes();
        void SetBackgroundColor(const Color& color);

    private:
        std::vector<Shape> Shapes;
        Color BackgroundColor;

        // Window/OpenGL handles
        void* WindowHandle;  // Will be HWND on Windows
        void* DeviceContext; // Will be HDC on Windows
        void* RenderContext; // Will be HGLRC on Windows

        // Internal functions
        bool CreateWindow();
        bool InitializeOpenGL();
        void Render();
        void RenderShape(const Shape& shape);
        void Cleanup();
    };
}