#ifndef COOLMATH
#define COOLMATH

#include <intrin.h>
#include <cstdint>
#include <cmath>

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


class fp {
    private:
        int64_t raw_value;

        static const int FRACTIONAL_BITS = 32;

        static const int64_t SCALE = 1LL << FRACTIONAL_BITS;

        explicit fp( int64_t raw, bool ) : raw_value( raw ) {}

    public:
        fp() : raw_value( 0 ) {}

        explicit fp( int64_t int_value ) {
            raw_value = int_value << FRACTIONAL_BITS;
        }

        explicit fp( float float_value ) {
            raw_value = static_cast< int64_t >( std::round( float_value * SCALE ) );
        }

        explicit fp( double double_value ) {
            raw_value = static_cast< int64_t >( std::round( double_value * SCALE ) );
        }

        float toFloat() const {
            return static_cast< float >( raw_value ) / SCALE;
        }

        double toDouble() const {
            return static_cast< double >( raw_value ) / SCALE;
        }

        int64_t toInt() const {
            return raw_value >> FRACTIONAL_BITS;
        }

        fp operator+( const fp& other ) const {
            return fp( raw_value + other.raw_value, true );
        }

        fp operator-( const fp& other ) const {
            return fp( raw_value - other.raw_value, true );
        }

        fp operator*( const fp& other ) const {
            __int128 product = static_cast< __int128 >( raw_value ) * other.raw_value;
            return fp( static_cast< int64_t >( product >> FRACTIONAL_BITS ), true );
        }

        fp operator/( const fp& other ) const {
            __int128 dividend = static_cast<__int128>(raw_value) << FRACTIONAL_BITS;
            return fp(static_cast<int64_t>(dividend / other.raw_value), true);
        }

        fp& operator+=( const fp& other ) {
            raw_value += other.raw_value;
            return *this;
        }

        fp& operator-=( const fp& other ) {
            raw_value -= other.raw_value;
            return *this;
        }

        fp& operator*=( const fp& other ) {
            *this = *this * other;
            return *this;
        }

        fp operator*( int scalar ) const {
            return fp( raw_value * scalar, true );
        }

        fp& operator*=( int scalar ) {
            raw_value *= scalar;
            return *this;
        }

        fp& operator=( const fp& other ) {
            if ( this != &other ) {
                raw_value = other.raw_value;
            }
            return *this;
        }

        fp& operator=( int64_t intValue ) {
            raw_value = intValue << FRACTIONAL_BITS;
            return *this;
        }

        fp& operator=( float floatValue ) {
            raw_value = static_cast< int64_t >( std::round( floatValue * SCALE ) );
            return *this;
        }



        




        bool operator==( const fp& other ) const { return raw_value == other.raw_value; }
        bool operator!=( const fp& other ) const { return raw_value != other.raw_value; }
        bool operator<( const fp& other ) const  { return raw_value < other.raw_value; }
        bool operator>( const fp& other ) const  { return raw_value > other.raw_value; }
        bool operator<=( const fp& other ) const { return raw_value <= other.raw_value; }
        bool operator>=( const fp& other ) const { return raw_value >= other.raw_value; }
};



struct fpVec2 {
    fp x = fp(0.0f); 
    fp y = fp(0.0f);

    fpVec2() : x(), y() {}
    
    fpVec2(fp _x, fp _y) : x(_x), y(_y) {}

    fpVec2(float _x, float _y) : x(fp(_x)), y(fp(_y)) {}
    fpVec2(double _x, double _y) : x(fp(_x)), y(fp(_y)) {}


    fpVec2 operator+( const fpVec2& v ) const { return {x + v.x, y + v.y}; }
    fpVec2 operator*( fp scalar ) const { return {x * scalar, y * scalar}; }
    void operator+=( const fpVec2& v ) { x += v.x; y += v.y; }
    void operator*=( const fpVec2& v ) { x *= v.x; y*= v.y; }
};

/**
    Global position struct for storing global x and y
*/
struct GlobalPosition {
    fpVec2 position;
};

#endif