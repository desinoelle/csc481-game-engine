#include "coolPhysics.hpp"
#include "entity.hpp"

#include <algorithm>
#include <vector>

/**
    Check overlap function checks overlap between two axis aligned boxes
    and returns a boolean
*/
bool Collision::checkOverlap( Collision * other ) {
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

void Collision::movePosition( const fpVec2 p ) {
    TopLeft += p;
    TopRight += p;
    BottemLeft += p;
    BottemRight += p;
}


/**
    Sets mass for dynamic objects
    also precomputes inverse mass for acceleration
*/
void Dynamic::setMass( fp m ) {
    mass = m;
    inverseMass = (m > fp( 0.0f ) ) ? fp( 1.0f ) / m : fp( 0.0f );
}

/**
    Adds force to dynamic objects and ignores static objects
*/
void Dynamic::addForce( const fpVec2& f ) {
    if ( !isStatic ) {
        force += f;
    }
}

/**
    Adds velocity to dynamic objects and ignors static objects
*/
void Dynamic::addVelocity( const fpVec2& v ) {
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
void Dynamic::setGravity( const fpVec2& g ) {
    gravity = g;
}

/**
    Sets physics friction
*/
void Dynamic::setFriction( const fpVec2& f ) {
    friction = f;
}

/**
    Updates all nonstatic phyiscs objects accelration, velocity, and position
    Also applies gravity to all physics objects
*/
void Physics::step( fp deltaTime ) {
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
            if ( body->entity->hasCollision ) {
                body->entity->Collision.movePosition( body->velocity * deltaTime );
            }
        }

        body->velocity *= body->friction;

        body->force = fpVec2{ 0.0f, 0.0f };
    }
}

