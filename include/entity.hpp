#ifndef ENTITY_HPP
#define ENTITY_HPP

#include "sprite.hpp"
#include "coolMath.hpp"
#include "coolPhysics.hpp"
#include <SDL3/SDL.h>

struct Entity {
    GlobalPosition position;
    Sprite* sprite = nullptr;
    bool spriteLoaded = false;
    SDL_FRect defaultRectangle;
    int color[4] = { 0, 0, 0, 255 };
    float rectangleWidth = 0.0f;
    float rectangleHeight = 0.0f;

    Collision Collision;

    bool hasCollision = false;

    void setColor( int r, int g, int b, int a ) {
        color[0] = r;
        color[1] = g;
        color[2] = b;
        color[3] = a;
    }

    void setRectangleHeightAndWidth( float h, float w ) {
        rectangleHeight = h;
        rectangleWidth = w;
    }


    void setCollisionBox(float halfWidth, float halfHeight) {
        Vector2 center = position.position;
        Collision.BottemLeft = { center.x - halfWidth, center.y - halfHeight };
        Collision.TopRight = { center.x + halfWidth, center.y + halfHeight };
        Collision.TopLeft = { center.x - halfWidth, center.y + halfHeight };
        Collision.BottemRight = { center.x + halfWidth, center.y - halfHeight };
        rectangleWidth = halfWidth * 2.0f;
        rectangleHeight = halfHeight * 2.0f;
    }

    void draw(SDL_Renderer* renderer) {
        if (sprite != nullptr && spriteLoaded ) {
            sprite->draw(renderer, position.position.x, position.position.y);
        } else {
            defaultRectangle = { position.position.x - rectangleWidth / 2.0f, position.position.y - rectangleHeight / 2.0f, 
                                 rectangleWidth, rectangleHeight};
            SDL_SetRenderDrawColor( renderer, color[0], color[1], color[2], color[3] );
            SDL_RenderFillRect( renderer, &defaultRectangle );
        }
    }
};

#endif