#include "coolPhysics.hpp"
#include "entity.hpp"

#include <algorithm>
#include <vector>

/**
    Check overlap function checks overlap between two axis aligned boxes
    and returns a boolean
*/
bool Collision::checkOverlap( Collision * other ) {
    float firstBoxMaxX = this->TopRight.x;
    float firstBoxMaxY = this->TopRight.y;
    float firstBoxMinX = this->BottemLeft.x;
    float firstBoxMinY = this->BottemLeft.y;

    float secondBoxMaxX = other->TopRight.x;
    float secondBoxMaxY = other->TopRight.y;
    float secondBoxMinX = other->BottemLeft.x;
    float secondBoxMinY = other->BottemLeft.y;

    return {
        firstBoxMinX <= secondBoxMaxX &&
        firstBoxMaxX >= secondBoxMinX &&
        firstBoxMinY <= secondBoxMaxY &&
        firstBoxMaxY >= secondBoxMinY
    };
    
}


/**
    Sets mass for dynamic objects
    also precomputes inverse mass for acceleration
*/
void Dynamic::setMass( float m ) {
    mass = m;
    inverseMass = (m > 0.0f) ? 1.0f / m : 0.0f;
}

/**
    Adds force to dynamic objects and ignores static objects
*/
void Dynamic::addForce( const Vector2& f ) {
    if ( !isStatic ) {
        force += f;
    }
}

/**
    Default gravity for physics
*/
Physics::Physics() : gravity{ 0.0f, 9.81f } {}

/**
    Adds body to physics list of physics objects
*/
void Physics::addBody( Dynamic* body ) {
    bodies.push_back( body );
}

/**
    Sets phyiscs gravity
*/
void Physics::setGravity( Vector2 g ) {
    Physics::gravity = g;
}

/**
    Updates all nonstatic phyiscs objects accelration, velocity, and position
    Also applies gravity to all physics objects
*/
void Physics::step( float deltaTime ) {
    for ( auto* body: bodies ) {
        if ( body->isStatic ) {
            continue;
        }

        body->acceleration = ( body->force * body->inverseMass ) + gravity;

        body->velocity += body->acceleration * deltaTime;

        body->entity.position.position += body->velocity * deltaTime;

        body->force = { 0.0f, 0.0f };
    }
}


