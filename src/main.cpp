#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>
#include <cstdio>
#include <cmath>
#include <thread>
#include <vector>
#include "input.hpp"
#include "entity.hpp"
#include "coolPhysics.hpp"
#include "timeMagic.hpp"
#include "niceSharedData.hpp"
#include "niceJobSystem.hpp"

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

    SDL_Window * window = nullptr;
    SDL_Renderer * renderer = nullptr;
    if (!SDL_CreateWindowAndRenderer("Cool engine", WINDOW_WIDTH, WINDOW_HEIGHT, 0, &window, &renderer)) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    SDL_Log("Window created!");

    InputSystem inputSystem;
    Physics physics;
    Timeline timeline;
    timeline.init();

    // Initialize shared data
    SharedData sharedData;
    sharedData.inputSystem = &inputSystem;
    sharedData.physics = &physics;
    sharedData.timeline = &timeline;

    bool running = true;
    SDL_Event event;
    const int numWorkerThreads = 2;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
        }

        // Prepare job queue for this frame
        JobQueue jobQueue;

        // Job 1: Update input system
        jobQueue.push_back([&sharedData]() {
            sharedData.inputSystem->update();
        });

        // Job 2: Update timeline
        jobQueue.push_back([&sharedData]() {
            sharedData.timeline->update();
        });

        // Reset job index for this frame
        sharedData.nextJobIndex = 0;

        // Spawn worker threads to process jobs
        std::vector<std::thread> threads;
        for (int i = 0; i < numWorkerThreads; ++i) {
            threads.emplace_back(worker, std::ref(sharedData), std::ref(jobQueue));
        }

        // Wait for all workers to finish
        for (auto& t : threads) {
            t.join();
        }

        // Handle pause control (main thread)
        if (inputSystem.isKeyPressed(SDL_SCANCODE_RETURN)) {
            if (timeline.isPaused()) {
                timeline.setPause(false);
                SDL_Log("Game unpaused!");
            } else {
                timeline.setPause(true);
                SDL_Log("Game paused!");
            }
        }

        SDL_SetRenderDrawColor( renderer, 0, 0, 255, 255 );
        SDL_RenderClear( renderer );

        SDL_RenderPresent( renderer );
    }

    SDL_DestroyRenderer( renderer );
    SDL_DestroyWindow( window );
    SDL_Quit();
    return 0;
}