#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <string>
#include <iostream>

using namespace std;

public:
    bool loadSprite( string file, SDL_Renderer* renderer, int frameCount, int frameWidth, int frameHeight, Uint32 animationDelay ) {
        if (state.Texture != nullptr) {
            SDL_DestroyTexture(state.Texture);
            state.Texture = nullptr;
        }

        state.Texture = IMG_LoadTexture(renderer, file.c_str());
        
        if (state.Texture == nullptr) {
            cerr << "Failed to load texture '" << file << "': " << SDL_GetError() << endl;
            return false;
        }

        this->frameCount = frameCount;
        this->frameWidth = frameWidth;
        this->frameHeight = frameHeight;
        this->animationDelay = animationDelay;

        srcRect.w = frameWidth;
        srcRect.h = frameHeight;

        dstRect.w = frameWidth;
        dstRect.h = frameHeight;

        state.currentFrame = 0;
        state.lastFrameTime = SDL_GetTicks();

        return true;
    }

    void update() {
        if (state.Texture == nullptr) return;

        Uint32 currentTime = SDL_GetTicks();
        if ( currentTime - state.lastFrameTime >= animationDelay ) {
            state.currentFrame = ( state.currentFrame + 1 ) % frameCount;
            state.lastFrameTime = currentTime;
        }
    }



    void draw( SDL_Renderer * renderer, float x, float y ) {
        if ( state.Texture == nullptr ) return;

        srcRect.x = state.currentFrame * frameWidth;
        srcRect.y = 0.0f;

        dstRect.x = x;
        dstRect.y = y;


        SDL_RenderTexture( renderer, state.Texture, &srcRect, &dstRect );
    }

    bool deloadSprite() {
        if ( state.Texture != nullptr ) {
            SDL_DestroyTexture( state.Texture );
            state.Texture = nullptr;
            return true;
        }
        return false;
    }

    ~Sprite() {
        deloadSprite();
    }


private:
    struct AppState {
        SDL_Texture * Texture = nullptr;
        int currentFrame = 0;
        Uint32 lastFrameTime = 0;
    };

    AppState state;

    SDL_FRect srcRect;

    SDL_FRect dstRect;

    int frameCount = 0;
    int frameWidth = 0;
    int frameHeight = 0;
    Uint32 animationDelay = 0;