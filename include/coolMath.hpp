#ifndef COOLMATH
#define COOLMATH

/**
    Vector2 struct for storing x and y
    Also has add, multiply and += opperations
*/
struct Vector2 {
    float x = 0.0f;
    float y = 0.0f;

    Vector2 operator+(const Vector2& v) const { return {x + v.x, y + v.y}; }
    Vector2 operator*(float scalar) const { return {x * scalar, y * scalar}; }
    void operator+=(const Vector2& v) { x += v.x; y += v.y; }
    void operator*=(const Vector2& v) { x *= v.x; y*= v.y; }
};

/**
    Global position struct for storing global x and y
*/
struct GlobalPosition {
    Vector2 position;
};

#endif