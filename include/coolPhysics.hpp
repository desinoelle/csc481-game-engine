#ifndef COOL_PHYSICS
#define COOL_PHYSICS

#include "coolMath.hpp"
// Forward declare Entity to avoid circular dependency
struct Entity;

#include <vector>

/**
    Collision struct for axis aligned boxes
    Checkes collision based on min and max x and y between 2 boxes
*/
struct Collision {
    Vector2 TopLeft;
    Vector2 TopRight;
    Vector2 BottemLeft;
    Vector2 BottemRight;


    bool checkOverlap( Collision * other );

    void movePosition( const Vector2 p );

};

/**
    Dynamic struct for physics based entities
    Stores the entity, velocity, acceleration, force, and mass
    Able to set mass and add force to dynamic entities
*/
struct Dynamic {
    Entity* entity = nullptr;  // Use pointer to avoid circular dependency
    Vector2 velocity;
    Vector2 acceleration;
    Vector2 force;
    Vector2 friction = { 1.0f, 1.0f };
    Vector2 gravity = { 0.0f, 9.81f };

    float mass = 1.0f;
    float inverseMass;
    bool isStatic = false;


    void setMass( float m );

    void addForce( const Vector2& f );

    void addVelocity( const Vector2& f);

    void setFriction( const Vector2& f );

    void setGravity( const Vector2& g );
};

/**
    Physics struct for general physics
    Has gravity variable and list of bodies 
    Able to add bodies and update their physics
*/
class Physics {
    private:
        std::vector< Dynamic* > bodies;

    public:
        Physics();

        void addBody( Dynamic * body );
        void step( float deltaTime );

};

#endif