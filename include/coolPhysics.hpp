#ifndef COOL_PHYSICS
#define COOL_PHYSICS

#include "coolMath.hpp"
// Forward declare Entity to avoid circular dependency
struct Entity;

#include <vector>
#include <mutex>

/**
    Collision struct for axis aligned boxes
    Checkes collision based on min and max x and y between 2 boxes
*/
struct Collision {
    fpVec2 TopLeft;
    fpVec2 TopRight;
    fpVec2 BottemLeft;
    fpVec2 BottemRight;


    bool checkOverlap( Collision * other );

    void movePosition( const fpVec2 p );


};

/**
    Dynamic struct for physics based entities
    Stores the entity, velocity, acceleration, force, and mass
    Able to set mass and add force to dynamic entities
*/
struct Dynamic {
    Entity* entity = nullptr;  // Use pointer to avoid circular dependency
    fpVec2 velocity;
    fpVec2 acceleration;
    fpVec2 force;
    fpVec2 friction = fpVec2{ fp( 1.0f ), fp( 1.0f ) };
    fpVec2 gravity = fpVec2{ fp( 0.0f ), fp( 9.81f ) };

    fp mass = fp( 1.0f );
    fp inverseMass;
    bool isStatic = false;


    void setMass( fp m );

    void addForce( const fpVec2& f );

    void addVelocity( const fpVec2& f);

    void setFriction( const fpVec2& f );

    void setGravity( const fpVec2& g );
};

/**
    Physics struct for general physics
    Has gravity variable and list of bodies 
    Able to add bodies and update their physics
*/
class Physics {
    private:
        std::vector< Dynamic* > bodies;
        std::mutex bodiesMutex;

    public:
        Physics();

        void addBody( Dynamic * body );
        void step( fp deltaTime );

        // Thread-safe getters/setters
        std::lock_guard<std::mutex> getBodiesMutexLock() {
            return std::lock_guard<std::mutex>(bodiesMutex);
        }

        const std::vector<Dynamic*>& getBodies() const {
            return bodies;
        }

};

#endif