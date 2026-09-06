#ifndef ENTITY_HPP
#define ENTITY_HPP

#include "sprite.hpp"
#include "coolMath.hpp"
#include "coolPhysics.hpp"
#include <SDL3/SDL.h>

struct Entity {
    GlobalPosition position;
    Sprite* sprite = nullptr;

    Collision Collision;

    bool hasCollision = false;

    void updatePosition() {
        // Position is updated by Physics::step()
    }

    void setCollisionBox(float halfWidth, float halfHeight) {
        Vector2 center = position.position;
        Collision.BottemLeft = { center.x - halfWidth, center.y - halfHeight };
        Collision.TopRight = { center.x + halfWidth, center.y + halfHeight };
        Collision.TopLeft = { center.x - halfWidth, center.y + halfHeight };
        Collision.BottemRight = { center.x + halfWidth, center.y - halfHeight };
    }

    void draw(SDL_Renderer* renderer) {
        if (sprite != nullptr) {
            sprite->draw(renderer, position.position.x, position.position.y);
        }
    }
};

#endif