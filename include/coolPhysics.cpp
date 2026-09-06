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

void Collision::movePosition( const Vector2& p ) {
    TopLeft += p;
    TopRight += p;
    BottemLeft += p;
    BottemRight += p;
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
    Adds velocity to dynamic objects and ignors static objects
*/
void Dynamic::addVelocity( const Vector2& v ) {
    if ( !isStatic ) {
        velocity += v;
    }
}

/**
    Default physics
*/
Physics::Physics() {}

/**
    Adds body to physics list of physics objects
*/
void Physics::addBody( Dynamic* body ) {
    bodies.push_back( body );
}

/**
    Sets phyiscs gravity
*/
void Dynamic::setGravity( const Vector2& g ) {
    gravity = g;
}

/**
    Sets physics friction
*/
void Dynamic::setFriction( const Vector2& f ) {
    friction = f;
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

        body->acceleration = ( body->force * body->inverseMass ) + body->gravity;
        //printf("%lf \n", body->acceleration);
        body->velocity += body->acceleration * deltaTime;

        // Only update position if entity exists
        if (body->entity != nullptr) {
            body->entity->position.position += body->velocity * deltaTime;
        }

        body->velocity *= body->friction;

        body->force = { 0.0f, 0.0f };
    }
}

