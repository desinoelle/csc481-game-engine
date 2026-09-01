#ifndef COOLMATH
#define COOLMATH

struct Vector2 {
    float x = 0.0f;
    float y = 0.0f;

    Vector2 operator+(const Vector2& v) const { return {x + v.x, y + v.y}; }
    Vector2 operator*(float scalar) const { return {x * scalar, y * scalar}; }
    void operator+=(const Vector2& v) { x += v.x; y += v.y; }
};

#endif