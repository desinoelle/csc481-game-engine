#include "coolPhysics.hpp"
#include "entity.hpp"
#include "sillyPropertyTypes.hpp"
#include <algorithm>
#include <vector>

/**
    Check overlap function checks overlap between two axis aligned boxes
    and returns a boolean
*/
bool collision::checkOverlap( collision * other ) {
    fp firstBoxMaxX = this->TopRight.x;
    fp firstBoxMaxY = this->TopRight.y;
    fp firstBoxMinX = this->BottemLeft.x;
    fp firstBoxMinY = this->BottemLeft.y;

    fp secondBoxMaxX = other->TopRight.x;
    fp secondBoxMaxY = other->TopRight.y;
    fp secondBoxMinX = other->BottemLeft.x;
    fp secondBoxMinY = other->BottemLeft.y;

    return {
        firstBoxMinX <= secondBoxMaxX &&
        firstBoxMaxX >= secondBoxMinX &&
        firstBoxMinY <= secondBoxMaxY &&
        firstBoxMaxY >= secondBoxMinY
    };
    
}

void collision::movePosition( const fpVec2 p ) {
    TopLeft += p;
    TopRight += p;
    BottemLeft += p;
    BottemRight += p;
}


/**
    Sets mass for physics data
    also precomputes inverse mass for acceleration
*/
void PhysicsData::setMass( fp m ) {
    mass = m;
    inverseMass = (m > fp( 0.0f ) ) ? fp( 1.0f ) / m : fp( 0.0f );
}

/**
    Adds force to physics data and ignores static objects
*/
void PhysicsData::addForce( const fpVec2& f ) {
    if ( !isStatic ) {
        force += f;
    }
}

/**
    Adds velocity to physics data and ignors static objects
*/
void PhysicsData::addVelocity( const fpVec2& v ) {
    if ( !isStatic ) {
        velocity += v;
    }
}

/**
    Sets physics gravity
*/
void PhysicsData::setGravity( const fpVec2& g ) {
    gravity = g;
}

/**
    Sets physics friction
*/
void PhysicsData::setFriction( const fpVec2& f ) {
    friction = f;
}

/**
    Default physics
*/
Physics::Physics() {}

/**
    Ids of every entity that has the physics property
*/
const std::vector<std::string>& Physics::getBodies() const {
    return Properties().objectsWith( PROP_PHYSICS );
}

/**
    Updates all nonstatic entities that have the physics property:
    acceleration, velocity, and position (and their collision box if they have one).
    Also applies gravity to all of them.
*/
void Physics::step( fp deltaTime ) {
    std::lock_guard<std::mutex> lock(bodiesMutex);

    for ( const std::string& id : Properties().objectsWith( PROP_PHYSICS ) ) {
        PhysicsData& body = GetProperty<PhysicsData>( PROP_PHYSICS, id );

        if ( body.isStatic ) {
            continue;
        }

        body.acceleration = ( body.force * body.inverseMass ) + body.gravity;
        body.velocity += body.acceleration * deltaTime;

        // Only move things that actually have a position
        if ( Properties().has( PROP_POSITION, id ) ) {
            GetProperty<fpVec2>( PROP_POSITION, id ) += body.velocity * deltaTime;
            if ( Properties().has( "collision", id ) ) {
                GetProperty<collision>( "collision", id ).movePosition( body.velocity * deltaTime );
            }
        }

        body.velocity *= body.friction;

        body.force = fpVec2{ 0.0f, 0.0f };
    }
}

