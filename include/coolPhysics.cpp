#include "coolPhysics.hpp"

#include <vector>

void Dynamic::setMass( float m ) {
    mass = m;
    inverseMass = (m > 0.0f) ? 1.0f / m : 0.0f;
}

void Dynamic::addForce( const Vector2& f ) {
    if ( !isStatic ) {
        force += f;
    }
}

Physics::Physics() : gravity{ 0.0f, 9.81f } {}

void Physics::addBody( Dynamic* body ) {
    bodies.push_back( body );
}

void Physics::setGravity( Vector2 g ) {
    Physics::gravity = g;
}

void Physics::step( float deltaTime ) {
    for ( auto* body: bodies ) {
        if ( body->isStatic ) {
            continue;
        }

        body->acceleration = ( body->force * body->inverseMass ) + gravity;

        body->velocity += body->acceleration * deltaTime;

        body->position.position += body->velocity * deltaTime;

        body->force = { 0.0f, 0.0f };
    }
}


