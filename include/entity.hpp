#ifndef ENTITY_HPP
#define ENTITY_HPP

#include "sprite.hpp"
#include "coolMath.hpp"
#include <SDL3/SDL.h>

struct Entity {
    GlobalPosition position;
    Sprite* sprite = nullptr;

    void updatePosition() {
        // Position is updated by Physics::step()
    }

    void draw(SDL_Renderer* renderer) {
        if (sprite != nullptr) {
            sprite->draw(renderer, position.position.x, position.position.y);
        }
    }
};

#endif