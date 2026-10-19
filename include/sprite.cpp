#include "sprite.hpp"
#include "sillyPropertyTypes.hpp"
#include <map>
#include <iostream>

using namespace std;
static std::map< std::string, SDL_Texture* > textureCache;

bool loadSprite(
    const std::string& id,
    const std::string& file,
    SDL_Renderer* renderer,
    int frameCount,
    int frameWidth,
    int frameHeight,
    Uint32 animationDelay
) {
    // Load texture only if we haven't loaded this file before
    if ( textureCache.count( file ) == 0 ) {
        SDL_Texture* tex = IMG_LoadTexture( renderer, file.c_str() );
        if ( tex == nullptr ) {
            cerr << "Failed to load texture '" << file << "': " << SDL_GetError() << endl;
            return false;
        }
        textureCache[ file ] = tex;
    }

    SpriteData data;
    data.texture = textureCache[ file ];
    data.frameCount = frameCount;
    data.frameWidth = frameWidth;
    data.frameHeight = frameHeight;
    data.animationDelay = animationDelay;
    data.currentFrame = 0;
    data.lastFrameTime = SDL_GetTicks();

    SetProperty( "sprite", id, data );
    return true;
}

void updateSprites() {
    for ( const std::string& id : Properties().objectsWith( "sprite" ) ) {
        SpriteData& s = GetProperty< SpriteData >( "sprite", id );

        if ( s.texture == nullptr ) continue;

        Uint32 currentTime = SDL_GetTicks();

        if ( currentTime - s.lastFrameTime >= s.animationDelay ) {
            s.currentFrame = ( s.currentFrame + 1 ) % s.frameCount;
            s.lastFrameTime = currentTime;
        }
    }
}

void drawSprite(const std::string& id, SDL_Renderer* renderer, float x, float y) {
    if ( !Properties().has( "sprite", id ) ) return;
    SpriteData& s = GetProperty< SpriteData >( "sprite", id );
    if (s.texture == nullptr) return;

    SDL_FRect srcRect, dstRect;

    // Calculate which frame in the spritesheet to draw
    srcRect.x = s.currentFrame * s.frameWidth;
    srcRect.y = 0.0f;

    // Set destination position
    dstRect.x = x;
    dstRect.y = y;

    srcRect.w = s.frameWidth;
    srcRect.h = s.frameHeight;
    dstRect.w = s.frameWidth;
    dstRect.h = s.frameHeight;

    // Draw the sprite
    SDL_RenderTexture(renderer, s.texture, &srcRect, &dstRect);
}

void unloadSprites() {
    for ( auto& entry : textureCache ) {
        SDL_DestroyTexture( entry.second );
    }
    textureCache.clear();
}