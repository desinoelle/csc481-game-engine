#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>
#include <cstdio>
#include <cmath>
#include "input.hpp"
#include "entity.hpp"
#include "coolPhysics.hpp"
#include "timeMagic.hpp"

const int WINDOW_WIDTH = 1920;
const int WINDOW_HEIGHT = 1080;
const int FRAME_COUNT = 8;
const int FRAME_WIDTH = 512;
const int FRAME_HEIGHT = 512;
const int ANIMATION_DELAY = 100;


int main ( int argc, char *argv[] ) {

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return 1;
    }

    // Initializes sdl window and renderer
    SDL_Window * window = nullptr;
    SDL_Renderer * renderer = nullptr;
    if (!SDL_CreateWindowAndRenderer("Cool engine", WINDOW_WIDTH, WINDOW_HEIGHT, 0, &window, &renderer)) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    SDL_Log("Window created!");

    InputSystem inputSystem;
    Timeline timeline;
    timeline.init();

    bool running = true;
    SDL_Event event;

    /**
     * Main loop of running the game
     */
    while (running) {

        SDL_PollEvent(&event);

        inputSystem.update();
        double deltaTime = timeline.update();
        if ( inputSystem.isKeyPressed( SDL_SCANCODE_ESCAPE ) ) {
            running = false;
        } else if (inputSystem.isKeyPressed( SDL_SCANCODE_SPACE )) {
            if (timeline.isPaused()) {
                timeline.setPaused(true);
                SDL_Log("Paused");
            } else {
                timeline.setPaused(false);
                SDL_Log("Unpaused");
            }
        }

        // Clears screen to blue and draws and renders it
        SDL_SetRenderDrawColor( renderer, 0, 0, 255, 255 );
        SDL_RenderClear( renderer );      

        SDL_RenderPresent( renderer );
    }

    // Cleans up windows and renderer
    SDL_DestroyRenderer( renderer );
    SDL_DestroyWindow( window );
    SDL_Quit();
    return 0;
}