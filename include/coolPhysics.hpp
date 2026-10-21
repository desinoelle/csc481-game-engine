#ifndef COOL_PHYSICS
#define COOL_PHYSICS

#include "coolMath.hpp"
#include <string>
#include <vector>
#include <mutex>

// Property name for physics data (lives here so sillyPropertyTypes.hpp can use it
// without a circular include; it already includes this header for `collision`)
inline const std::string PROP_PHYSICS = "physics";

/**
    Collision struct for axis aligned boxes
    Checkes collision based on min and max x and y between 2 boxes
*/
struct collision {
    fpVec2 TopLeft;
    fpVec2 TopRight;
    fpVec2 BottemLeft;
    fpVec2 BottemRight;


    bool checkOverlap( collision * other );

    void movePosition( const fpVec2 p );


};

/**
    PhysicsData is the value of the "physics" property.
    An entity "has physics" if its id is registered under PROP_PHYSICS.
    Bundles velocity, acceleration, force, friction, gravity, and mass.
    The entity id is the key in the property manager, so it is not stored here.
*/
struct PhysicsData {
    fpVec2 velocity;
    fpVec2 acceleration;
    fpVec2 force;
    fpVec2 friction = fpVec2{ fp( 1.0f ), fp( 1.0f ) };
    fpVec2 gravity = fpVec2{ fp( 0.0f ), fp( 9.81f ) };

    fp mass = fp( 1.0f );
    fp inverseMass = fp( 1.0f );
    bool isStatic = false;


    void setMass( fp m );

    void addForce( const fpVec2& f );

    void addVelocity( const fpVec2& f);

    void setFriction( const fpVec2& f );

    void setGravity( const fpVec2& g );
};


/**
    Physics system.
    Owns no bodies. Each step it asks the property manager which entities
    have the "physics" property and updates those.
*/
class Physics {
    private:
        std::mutex bodiesMutex;

    public:
        Physics();

        void step( fp deltaTime );

        // Thread-safe lock for callers that touch physics data from other threads
        std::lock_guard<std::mutex> getBodiesMutexLock() {
            return std::lock_guard<std::mutex>(bodiesMutex);
        }

        // Ids of every entity that currently has the physics property
        const std::vector<std::string>& getBodies() const;

};

#endif
