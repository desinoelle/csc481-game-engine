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
#include "evilNetworking.hpp"
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

    // Get client ID from command line (default to 0)
    int clientId = 0;
    if (argc > 1) {
        clientId = std::stoi(argv[1]);
        SDL_Log("Client ID: %d", clientId);
    }

    InputSystem inputSystem;
    Physics physics;
    Timeline timeline;
    timeline.init();

    // Initialize networking with client ID
    NetworkClient networkClient(clientId);
    if (!networkClient.connectToServer("localhost")) {
        SDL_Log("Failed to connect to server!");
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    SDL_Log("Connected to server with client ID %d", clientId);

    // Initialize shared data
    SharedData sharedData;
    sharedData.inputSystem = &inputSystem;
    sharedData.physics = &physics;
    sharedData.timeline = &timeline;

    bool running = true;
    SDL_Event event;
    const int numWorkerThreads = 2;
    int frameCount = 0;

    while (running) {
        frameCount++;

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
                SDL_Log("[Client %d] Game unpaused!", clientId);
            } else {
                timeline.setPause(true);
                SDL_Log("[Client %d] Game paused!", clientId);
            }
        }

        // Send this client's demo position to server
        // (For demo: just send a fixed position based on clientId)
        fpVec2 demoPosition = fpVec2(fp((float)WINDOW_WIDTH / 2.0f + clientId * 100.0f), fp((float)WINDOW_HEIGHT / 2.0f));
        networkClient.sendPosition(demoPosition);

        // Poll server for all clients' positions
        networkClient.pollState();
        const auto& players = networkClient.getPlayers();

        // Clear screen
        SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
        SDL_RenderClear(renderer);

        // Draw all players' positions from server (all clients see same thing)
        for (const auto& entry : players) {
            SDL_FRect rect = {
                (float)entry.second.x - 25.0f,
                (float)entry.second.y - 25.0f,
                50.0f,
                50.0f
            };
            
            // Draw this client's player in orange, others in red
            if (entry.second.id == networkClient.getMyId()) {
                SDL_SetRenderDrawColor(renderer, 255, 165, 0, 255);  // Orange
            } else {
                SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);    // Red
            }
            
            SDL_RenderFillRect(renderer, &rect);
        }

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer( renderer );
    SDL_DestroyWindow( window );
    SDL_Quit();
    return 0;
}