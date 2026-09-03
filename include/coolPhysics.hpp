#ifndef COOL_PHYSICS
#define COOL_PHYSICS

#include "coolMath.hpp"

#include <vector>

struct GlobalPosition {
    Vector2 position;
};

struct Collision {
    Vector2 TopLeft;
    Vector2 TopRight;
    Vector2 BottemLeft;
    Vector2 BottemRight;

    /** 
    bool checkOverlap( Collision other ) {

    }*/
};

struct Dynamic {
    GlobalPosition position;
    Vector2 velocity;
    Vector2 acceleration;
    Vector2 force;

    float mass = 1.0f;
    float inverseMass;
    bool isStatic = false;

    void setMass( float m );

    void addForce( const Vector2& f );
};

class Physics {
    private:
        std::vector< Dynamic* > bodies;
        Vector2 gravity;

    public:
        Physics();

        void addBody( Dynamic* body );
        void setGravity( Vector2 g );
        void step( float deltaTime );
};

#endif