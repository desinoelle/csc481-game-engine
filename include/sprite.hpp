#ifndef SPRITE_HPP
#define SPRITE_HPP

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <string>

class Sprite {
    public:
        Sprite();
        ~Sprite();

        // Load spritesheet
        bool loadSprite(
            std::string file,
            SDL_Renderer* renderer,
            int frameCount,
            int frameWidth,
            int frameHeight,
            Uint32 animationDelay
        );

        void update();

        void draw(SDL_Renderer* renderer, float x, float y);

        bool deloadSprite();

    private:
        struct AppState {
            SDL_Texture* Texture = nullptr;
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
};

#endif