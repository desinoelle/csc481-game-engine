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
const int GROUND_Y = WINDOW_HEIGHT - 200;

int main(int argc, char* argv[]) {
    // ===== INITIALIZATION =====
    
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL Init failed: %s", SDL_GetError());
        return 1;
    }

    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;

    if (!SDL_CreateWindowAndRenderer("Engine Demonstration", WINDOW_WIDTH, WINDOW_HEIGHT, 0, &window, &renderer)) {
        SDL_Log("Window creation failed: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Log("=== Engine Demonstration ===");
    SDL_Log("Controls: A/D to move, SPACE to jump");
    SDL_Log("Orange = Player (controllable, affected by gravity)");
    SDL_Log("Brown = Platform (static, no movement)");
    SDL_Log("Red = Enemy (auto-moving, patrols back and forth)");

    InputSystem inputSystem;
    Physics physics;

    // ===== ENTITY 1: PLAYER (Controllable, affected by gravity) =====
    Dynamic player;
    player.setMass(5.0f);
    player.setGravity({0.0f, 5.0f});  // weak gravity for slow falling
    player.setFriction({0.95f, 1.0f});
    
    player.entity = new Entity();
    player.entity->sprite = new Sprite();
    player.entity->position.position = {400.0f, (float)GROUND_Y};
    player.entity->hasCollision = true;
    player.entity->setCollisionBox(25.0f, 25.0f);
    
    physics.addBody(&player);
    SDL_Log("Player created at (%.0f, %.0f)", player.entity->position.position.x, player.entity->position.position.y);

    // ===== ENTITY 2: PLATFORM (Static - no movement) =====
    Dynamic platform;
    platform.setMass(0.0f);
    platform.isStatic = true;
    platform.entity = new Entity();
    platform.entity->position.position = {WINDOW_WIDTH / 2.0f, (float)(GROUND_Y + 50)};
    platform.entity->hasCollision = true;
    platform.entity->setCollisionBox(WINDOW_WIDTH / 2.0f, 25.0f);
    
    physics.addBody(&platform);
    SDL_Log("Platform created at (%.0f, %.0f)", platform.entity->position.position.x, platform.entity->position.position.y);

    // ===== ENTITY 3: ENEMY (Auto-moving patrol) =====
    Dynamic enemy;
    enemy.setMass(3.0f);
    enemy.setGravity({0.0f, 0.0f});  // No gravity
    enemy.isStatic = false;
    enemy.entity = new Entity();
    enemy.entity->position.position = {1300.0f, (float)GROUND_Y};  // On ground level
    enemy.entity->hasCollision = true;
    enemy.entity->setCollisionBox(40.0f, 40.0f);
    
    physics.addBody(&enemy);
    SDL_Log("Enemy created at (%.0f, %.0f)", enemy.entity->position.position.x, enemy.entity->position.position.y);

    // ===== GAME LOOP =====
    
    bool running = true;
    SDL_Event event;
    const float DELTA_TIME = 0.016f;  // ~60 FPS
    int frameCount = 0;
    bool spaceLastFrame = false;
    bool movingEnemyRight = true;
    const float JUMP_FORCE = 14000.0f;  // stronger jump to overcome gravity

    while (running) {
        frameCount++;
        
        // ----- QUIT HANDLING -----
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
        }

        inputSystem.update();

        // ----- TASK 4: INPUT HANDLING (A/D movement) -----
        if (inputSystem.isKeyPressed(SDL_SCANCODE_A)) {
            player.addForce({-150.0f, 0.0f}); 
        }
        if (inputSystem.isKeyPressed(SDL_SCANCODE_D)) {
            player.addForce({150.0f, 0.0f});  
        }

        // ----- TASK 3: PHYSICS STEP (gravity applied here) -----
        physics.step(DELTA_TIME);
        
        // ----- JUMP (AFTER physics.step so velocity doesn't get overwritten) -----
        bool spaceNow = inputSystem.isKeyPressed(SDL_SCANCODE_SPACE);
        
        if (spaceNow && !spaceLastFrame) {
            player.velocity.y = -60.0f;  // medium jump height
        }
        spaceLastFrame = spaceNow;

        // ----- ENEMY AUTO-MOVEMENT (continuous patrol in small area) -----
        // Move enemy back and forth in a small zone near the right side of the screen
        if (movingEnemyRight) {
            enemy.entity->position.position.x += 1.0f * DELTA_TIME;
            if (enemy.entity->position.position.x > 1400.0f) {
                movingEnemyRight = false;
            }
        } else {
            enemy.entity->position.position.x -= 1.0f * DELTA_TIME;  
            if (enemy.entity->position.position.x < 1200.0f) {
                movingEnemyRight = true;
            }
        }

        // Update enemy collision box after moving
        enemy.entity->setCollisionBox(40.0f, 40.0f);
        
        // Also update platform collision box to ensure it's in correct position
        platform.entity->setCollisionBox(WINDOW_WIDTH / 2.0f, 25.0f);
        
        // Update player collision box after all movement
        player.entity->setCollisionBox(25.0f, 25.0f);

        // ----- BOUNDARY CLAMPING -----
        if (player.entity->position.position.x < 0) {
            player.entity->position.position.x = 0;
        }
        if (player.entity->position.position.x > WINDOW_WIDTH) {
            player.entity->position.position.x = WINDOW_WIDTH;
        }
        
        // Keep player from going below ground - only reset if moving downward
        if (player.entity->position.position.y >= GROUND_Y) {
            player.entity->position.position.y = GROUND_Y;
            // Only reset velocity if moving downward (falling/landing)
            if (player.velocity.y > 0.0f) {
                player.velocity.y = 0.0f;
            }
        }
        
        // Keep player from going above screen
        if (player.entity->position.position.y < 0) {
            player.entity->position.position.y = 0;
            player.velocity.y = 0.0f;
        }

        // ----- TASK 5: COLLISION DETECTION -----
        // Check collisions every frame for accurate detection

        if (player.entity->Collision.checkOverlap(&enemy.entity->Collision)) {
            SDL_Log("Collision: Player hit enemy!");
        }

        // ----- TASK 1: RENDERING -----
        SDL_SetRenderDrawColor(renderer, 100, 149, 237, 255);  // blue (sky)
        SDL_RenderClear(renderer);

        // Draw platform (brown - static)
        {
            SDL_FRect platform_rect = {
                platform.entity->position.position.x - WINDOW_WIDTH / 2.0f,
                platform.entity->position.position.y - 25.0f,
                (float)WINDOW_WIDTH,
                50.0f
            };
            SDL_SetRenderDrawColor(renderer, 139, 69, 19, 255); 
            SDL_RenderFillRect(renderer, &platform_rect);
        }

        // Draw player (orange - controllable)
        {
            SDL_FRect player_rect = {
                player.entity->position.position.x - 25.0f,
                player.entity->position.position.y - 25.0f,
                50.0f,
                50.0f
            };
            SDL_SetRenderDrawColor(renderer, 255, 165, 0, 255); 
            SDL_RenderFillRect(renderer, &player_rect);
        }

        // Draw enemy (red - auto-moving)
        {
            SDL_FRect enemy_rect = {
                enemy.entity->position.position.x - 40.0f,
                enemy.entity->position.position.y - 40.0f,
                80.0f,
                80.0f
            };
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
            SDL_RenderFillRect(renderer, &enemy_rect);
        }

        SDL_RenderPresent(renderer);
    }

    // ===== CLEANUP =====
    if (player.entity != nullptr) {
        if (player.entity->sprite != nullptr) {
            player.entity->sprite->deloadSprite();
            delete player.entity->sprite;
        }
        delete player.entity;
    }

    if (platform.entity != nullptr) {
        delete platform.entity;
    }

    if (enemy.entity != nullptr) {
        delete enemy.entity;
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    SDL_Log("=== Engine Demonstration Ended ===");
    return 0;
}