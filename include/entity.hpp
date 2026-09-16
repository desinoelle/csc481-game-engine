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
    fp rectangleWidth = fp( 0.0f );
    fp rectangleHeight = fp( 0.0f );

    Collision Collision;

    bool hasCollision = false;

    void setColor( int r, int g, int b, int a ) {
        color[0] = r;
        color[1] = g;
        color[2] = b;
        color[3] = a;
    }

    void setRectangleHeightAndWidth( fp h, fp w ) {
        rectangleHeight = h;
        rectangleWidth = w;
    }


    void setCollisionBox( fp halfWidth, fp halfHeight) {
        fpVec2 center = position.position;
        Collision.BottemLeft = fpVec2{ center.x - halfWidth, center.y - halfHeight };
        Collision.TopRight = fpVec2{ center.x + halfWidth, center.y + halfHeight };
        Collision.TopLeft = fpVec2{ center.x - halfWidth, center.y + halfHeight };
        Collision.BottemRight = fpVec2{ center.x + halfWidth, center.y - halfHeight };
        rectangleWidth = halfWidth * fp( 2.0f );
        rectangleHeight = halfHeight * fp( 2.0f );
    }

    void draw(SDL_Renderer* renderer) {
        float drawX = position.position.x.toFloat();
        float drawY = position.position.y.toFloat();
        float w = rectangleWidth.toFloat();
        float h = rectangleHeight.toFloat();

        if ( sprite != nullptr && spriteLoaded ) {
            sprite->draw( renderer, drawX, drawY );
        } else {
            defaultRectangle = SDL_FRect{ drawX - ( w / 2.0f ), drawY - ( h / 2.0f ), w, h };
            
            SDL_SetRenderDrawColor(renderer, color[0], color[1], color[2], color[3]);
            SDL_RenderFillRect(renderer, &defaultRectangle);
        }
    }
};

#endif