#include "sprite.hpp"
#include <iostream>

using namespace std;

Sprite::Sprite() {}

Sprite::~Sprite() {
    deloadSprite();
}

bool Sprite::loadSprite(
    std::string file,
    SDL_Renderer* renderer,
    int frameCount,
    int frameWidth,
    int frameHeight,
    Uint32 animationDelay
) {
    // Clean up any existing texture
    if (state.Texture != nullptr) {
        SDL_DestroyTexture(state.Texture);
        state.Texture = nullptr;
    }

    // Load the texture from file
    state.Texture = IMG_LoadTexture(renderer, file.c_str());
    
    if (state.Texture == nullptr) {
        cerr << "Failed to load texture '" << file << "': " << SDL_GetError() << endl;
        return false;
    }

    // Store animation parameters
    this->frameCount = frameCount;
    this->frameWidth = frameWidth;
    this->frameHeight = frameHeight;
    this->animationDelay = animationDelay;

    // Set up rectangles for rendering
    srcRect.w = frameWidth;
    srcRect.h = frameHeight;

    dstRect.w = frameWidth;
    dstRect.h = frameHeight;

    // Reset animation
    state.currentFrame = 0;
    state.lastFrameTime = SDL_GetTicks();

    return true;
}

void Sprite::update() {
    if (state.Texture == nullptr) return;

    // Get current time in milliseconds
    Uint32 currentTime = SDL_GetTicks();
    
    // If enough time has passed, advance to next frame
    if (currentTime - state.lastFrameTime >= animationDelay) {
        state.currentFrame = (state.currentFrame + 1) % frameCount;
        state.lastFrameTime = currentTime;
    }
}

void Sprite::draw(SDL_Renderer* renderer, float x, float y) {
    if (state.Texture == nullptr) return;

    // Calculate which frame in the spritesheet to draw
    srcRect.x = state.currentFrame * frameWidth;
    srcRect.y = 0.0f;

    // Set destination position
    dstRect.x = x;
    dstRect.y = y;

    // Draw the sprite
    SDL_RenderTexture(renderer, state.Texture, &srcRect, &dstRect);
}

bool Sprite::deloadSprite() {
    if (state.Texture != nullptr) {
        SDL_DestroyTexture(state.Texture);
        state.Texture = nullptr;
        return true;
    }
    return false;
}