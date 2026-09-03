#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>
#include <cstdio>
#include "input.hpp"

const int WINDOW_WIDTH = 1920;  /*  Width of the game window to be created */
const int WINDOW_HEIGHT = 1080; /*  Height of the game window to be created */
const int FRAME_COUNT = 8;     /*  Number of frames in the spritesheet */
const int FRAME_WIDTH = 512;   /*  Width of the frame in the spritesheet */
const int FRAME_HEIGHT = 512;  /*  Height of the frame in the spritesheet */
const int ANIMATION_DELAY = 100;/* Number of iterations between the animation frames (determines delay) */

/* Struct to store the current state*/
struct AppState {
    SDL_Texture * Texture = nullptr;
    int currentFrame = 0;
    Uint32 lastFrameTime = 0;
};

int main ( int argc, char *argv[] ) {
    // Initialize the SDL library
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return 1;
    }

    SDL_Window * window = nullptr;
    SDL_Renderer * renderer = nullptr;

    // Initialize the window and renderer using SDL method
    if (!SDL_CreateWindowAndRenderer("Cool engine", WINDOW_WIDTH, WINDOW_HEIGHT, 0, &window, &renderer)) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    InputSystem inputSystem;

    bool running = true;

    SDL_Event event;

    while (running) {
        // Only use SDL_PollEvent for window close
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
        }

        // Update input state
        inputSystem.update();

        if (inputSystem.isKeyPressed(SDL_SCANCODE_W)) {
            SDL_Log("W pressed - move up");
            // Todo
        }

        if (inputSystem.isKeyPressed(SDL_SCANCODE_A)) {
            SDL_Log("A pressed - move left");
            // Todo
        }

        if (inputSystem.isKeyPressed(SDL_SCANCODE_S)) {
            SDL_Log("S pressed - move down");
            // Todo
        }

        if(inputSystem.isKeyPressed(SDL_SCANCODE_D)) {
            SDL_Log("D pressed - move right");
            // Todo
        }  

        // Set Background color to white
        SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
        // Clear screen
        SDL_RenderClear(renderer);
        // Update screen with new frame
        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}