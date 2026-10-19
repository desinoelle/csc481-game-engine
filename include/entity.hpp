#ifndef ENTITY_HPP
#define ENTITY_HPP

#include "sprite.hpp"
#include "coolMath.hpp"
#include "coolPhysics.hpp"
#include "sillyPropertyTypes.hpp"
#include <SDL3/SDL.h>
#include <string>

struct Entity {
    SDL_FRect defaultRectangle;

    collision& box() { return GetProperty<collision>( "collision", id ); }

    std::string id;

    fpVec2& position() { return GetProperty<fpVec2>( "position", id ); }
    const fpVec2& position() const { return GetProperty<fpVec2>( "position", id ); }

    Entity( ) {
        static int counter = 0;
        id = "entity_" + std::to_string( counter++ );
        SetProperty( "position", id, fpVec2() );
    }

    void setColor( int r, int g, int b, int a ) {
        SDL_Color c;
        c.r = r;
        c.g = g;
        c.b = b;
        c.a = a;
        SetProperty( "color", id, c );
    }

    void setRectangleHeightAndWidth( fp h, fp w ) {
        SetProperty( "size", id, fpVec2( w, h ) );
    }


    void setCollisionBox( fp halfWidth, fp halfHeight) {
        fpVec2 center = position();
        collision c;
        c.BottemLeft = fpVec2{ center.x - halfWidth, center.y - halfHeight };
        c.TopRight = fpVec2{ center.x + halfWidth, center.y + halfHeight };
        c.TopLeft = fpVec2{ center.x - halfWidth, center.y + halfHeight };
        c.BottemRight = fpVec2{ center.x + halfWidth, center.y - halfHeight };
        SetProperty( "collision", id, c );
        SetProperty( "size", id, fpVec2( halfWidth * fp( 2.0f ), halfHeight * fp( 2.0f ) ) );
    }

    void draw(SDL_Renderer* renderer) {
        float drawX = position().x.toFloat();
        float drawY = position().y.toFloat();
        float w = 0.0f;
        float h = 0.0f;
        if ( Properties().has( "size", id ) ) {
            fpVec2 size = GetProperty<fpVec2>( "size", id );
            w = size.x.toFloat();
            h = size.y.toFloat();
        }

        if ( Properties().has( "sprite", id ) ) {
            SpriteData& s = GetProperty< SpriteData >( "sprite", id );
            drawSprite( id, renderer, drawX - ( s.frameWidth / 2.0f ), drawY - ( s.frameHeight / 2.0f ) );
        } else {
            defaultRectangle = SDL_FRect{ drawX - ( w / 2.0f ), drawY - ( h / 2.0f ), w, h };

            SDL_Color c = { 0, 0, 0, 255 };
            if( Properties().has( "color", id ) ) {
                c = GetProperty<SDL_Color>( "color", id );
            }
            
            SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a );
            SDL_RenderFillRect(renderer, &defaultRectangle);
        }
    }
};

#endif
