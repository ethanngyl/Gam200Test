struct Vec2 {
    float x{0.0f};
    float y{0.0f};
};

enum class ShapeType { Circle, Rect };

struct Circle { float radius {0.0f};};
struct Rect   { float width{0.0f}, height{0.0f};};

struct Collider {
    ShapeType shapeType{ShapeType::Circle};
    Vec2 position{};

    Circle circle{};
    Rect rect{};

    static Collider CreateCircle (float radius, Vec2 position){
        Collider c;
        c.shapeType = ShapeType::Circle;
        c.circle.radius = radius;
        c.position = position;
        return c;
    }
    static Collider CreateRect (float width, float height, Vec2 position){
        Collider c;
        c.shapeType = ShapeType::Rect;
        c.rect.width = width;
        c.rect.height = height;
        c.position = position;
        return c;
    }
};

bool Circle_To_Circle (const Collider& a, const Collider& b);
bool Rect_To_Rect     (const Collider& a, const Collider& b);
bool Circle_To_Rect   (const Collider& a, const Collider& b);

bool CheckCollision (const Collider& a, const Collider& b);
