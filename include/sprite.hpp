#ifndef SPRITE_HPP
#define SPRITE_HPP

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <string>

bool loadSprite( 
    const std::string& id, 
    const std::string& file, 
    SDL_Renderer* renderer,
    int frameCount,
    int frameWidth,
    int frameHeight,
    Uint32 animationDelay
);

void updateSprites();

void drawSprite( const std::string& id, SDL_Renderer* renderer, float x, float y );

void unloadSprites();

#endif