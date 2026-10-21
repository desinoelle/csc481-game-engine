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
    Integrates ONE entity that has the physics property: acceleration, velocity, and
    position (and its collision box if it has one). Applies gravity too.
*/
void Physics::integrate( const std::string& id, fp deltaTime ) {
    if ( !Properties().has( PROP_PHYSICS, id ) ) {
        return;
    }
    PhysicsData& body = GetProperty<PhysicsData>( PROP_PHYSICS, id );

    if ( body.isStatic ) {
        return;
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

/**
    Updates all nonstatic entities that have the physics property.
*/
void Physics::step( fp deltaTime ) {
    std::lock_guard<std::mutex> lock(bodiesMutex);

    for ( const std::string& id : Properties().objectsWith( PROP_PHYSICS ) ) {
        integrate( id, deltaTime );
    }
}

/**
    Updates just one entity (by id). Each actor can own its own Physics object, so two
    actors can be stepped on two threads without locking each other.
*/
void Physics::stepBody( const std::string& id, fp deltaTime ) {
    std::lock_guard<std::mutex> lock(bodiesMutex);
    integrate( id, deltaTime );
}
