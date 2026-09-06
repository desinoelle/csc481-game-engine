#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>
#include <cstdio>
#include <cmath>
#include "input.hpp"
#include "entity.hpp"
#include "coolPhysics.hpp"

const int WINDOW_WIDTH = 1920;
const int WINDOW_HEIGHT = 1080;
const int FRAME_COUNT = 8;
const int FRAME_WIDTH = 512;
const int FRAME_HEIGHT = 512;
const int ANIMATION_DELAY = 100;

// Test rectangle size (much smaller than spritesheet)
const int TEST_RECT_WIDTH = 50;
const int TEST_RECT_HEIGHT = 50;

int main ( int argc, char *argv[] ) {
    // ===== INITIALIZATION =====
    
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

    // ===== CREATE PHYSICS WORLD =====
    Physics physics;

    // ===== CREATE PLAYER =====
    Dynamic player;
    player.setMass(5.0f);
    player.setFriction({0.2f, 1.0f});
    
    player.entity = new Entity();
    player.entity->sprite = new Sprite();
    
    bool spriteLoaded = player.entity->sprite->loadSprite(
        "assets/player.png",
        renderer,
        FRAME_COUNT,
        FRAME_WIDTH,
        FRAME_HEIGHT,
        ANIMATION_DELAY
    );
    
    if (!spriteLoaded) {
        SDL_Log("Sprite not loaded. Will draw test rectangle.");
        printf("WWW");
    }

    // Set starting position (centered on screen)
    player.entity->position.position = {
        WINDOW_WIDTH / 2.0f,
        WINDOW_HEIGHT / 2.0f
    };

    physics.addBody(&player);

    Entity platform;
    platform.sprite = new Sprite();


    // ===== GAME LOOP =====
    
    bool running = true;
    SDL_Event event;
    const float DELTA_TIME = 0.016f;

    while (running) {
        // Handle quit
        while (SDL_PollEvent(&event)) {
            if (inputSystem.isKeyPressed(SDL_SCANCODE_ESCAPE)) {
                running = false;
            }
        }

        inputSystem.update();

        // Apply forces based on input
        if (inputSystem.isKeyPressed(SDL_SCANCODE_W)) {
            player.addForce({0.0f, -150.0f});
        }
        if (inputSystem.isKeyPressed(SDL_SCANCODE_A)) {
            player.addVelocity({-150.0f, 0.0f});
        }
        if (inputSystem.isKeyPressed(SDL_SCANCODE_S)) {
            player.addVelocity({0.0f, 150.0f});
        }
        if (inputSystem.isKeyPressed(SDL_SCANCODE_D)) {
            player.addVelocity({150.0f, 0.0f});
        }

        // Physics step
        physics.step(DELTA_TIME);

        // Ground collision
        if (player.entity->position.position.y > WINDOW_HEIGHT - TEST_RECT_HEIGHT) {
            player.entity->position.position.y = WINDOW_HEIGHT - TEST_RECT_HEIGHT;
            player.velocity.y = 0.0f;
        }

        // Horizontal bounds
        if (player.entity->position.position.x < 0) {
            player.entity->position.position.x = 0;
            player.velocity.x = 0.0f;
        }
        if (player.entity->position.position.x > WINDOW_WIDTH - TEST_RECT_WIDTH) {
            player.entity->position.position.x = WINDOW_WIDTH - TEST_RECT_WIDTH;
            player.velocity.x = 0.0f;
        }

        // Sprite animation
        if (player.entity != nullptr && player.entity->sprite != nullptr) {
            player.entity->sprite->update();
        }

        // ===== RENDERING =====
        SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
        SDL_RenderClear(renderer);

        // Draw sprite if loaded
        if (spriteLoaded && player.entity != nullptr) {
            player.entity->draw(renderer);
        }
        // Otherwise draw test rectangle (filled, centered)
        else if (player.entity != nullptr) {
            SDL_FRect testRect = {
                player.entity->position.position.x - TEST_RECT_WIDTH / 2.0f,   // Center X
                player.entity->position.position.y - TEST_RECT_HEIGHT / 2.0f,  // Center Y
                (float)TEST_RECT_WIDTH,
                (float)TEST_RECT_HEIGHT
            };
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);  // Red
            SDL_RenderFillRect(renderer, &testRect);  // Filled rectangle
        }

        SDL_RenderPresent(renderer);
    }

    // ===== CLEANUP =====
    if (player.entity != nullptr) {
        if (player.entity->sprite != nullptr) {
            player.entity->sprite->deloadSprite();
            delete player.entity->sprite;
            player.entity->sprite = nullptr;
        }
        delete player.entity;
        player.entity = nullptr;
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}