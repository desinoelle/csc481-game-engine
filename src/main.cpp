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
#include "saucyPeerToPeer.hpp"

const int WINDOW_WIDTH = 1920;
const int WINDOW_HEIGHT = 1080;
const int FRAME_COUNT = 8;
const int FRAME_WIDTH = 512;
const int FRAME_HEIGHT = 512;
const int ANIMATION_DELAY = 100;


const int P2P_WINDOW_WIDTH = 800;
const int P2P_WINDOW_HEIGHT = 600;
const int P2P_SQUARE_SIZE = 50;
const int P2P_SPEED = 5;
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


    bool isPeer2Peer = true;


    if(!isPeer2Peer) {
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
    }


    if(isPeer2Peer) {
        bool isPlayer1 = true;
        for ( int i = 1; i < argc; i++ ) {
            std::string arg = argv[i];
            if ( arg == "1" ) {
                isPlayer1 = true;
                break;
            }
            if ( arg == "2" ) {
                isPlayer1 = false;
                break;
            }
        }

        const char* playerName = isPlayer1 ? "Player 1" : "Player 2";

        SDL_SetWindowTitle( window, playerName );
        SDL_SetWindowSize( window, P2P_WINDOW_WIDTH, P2P_WINDOW_HEIGHT );
        SDL_SetWindowPosition( window, isPlayer1 ? 50 : 50 + P2P_WINDOW_WIDTH + 20, 100 );

        SDL_SetRenderVSync( renderer, 1 );

        int localPort = isPlayer1 ? 7000 : 7001;
        int peerPort  = isPlayer1 ? 7001 : 7000;
        LockstepPeer peer( localPort, peerPort );

        SDL_Log( "%s waiting for the other player...", playerName );
        if ( !peer.handshake( 1, 30000 ) ) {
            SDL_Log( "Handshake failed, other player never showed up" );
            SDL_DestroyRenderer( renderer );
            SDL_DestroyWindow( window );
            SDL_Quit();
            return 1;
        }
        SDL_Log( "Connected! Shared seed %u", peer.sharedSeed() );

        int squareX[ 2 ] = { 200, 550 };
        int squareY[ 2 ] = { 275, 275 };

        KeyboardSnapshot localKeys;
        KeyboardSnapshot remoteKeys;
        bool waitingForPeer = false;
        bool running = true;
        uint32_t frame = 0;
        SDL_Event event;

        while ( running ) {
            timeline.setPause(true);
            while ( SDL_PollEvent( &event ) ) {
                if ( event.type == SDL_EVENT_QUIT ) {
                    running = false;
                }
            }
            inputSystem.update();

            if ( !waitingForPeer ) {
                localKeys.copyFrom( inputSystem );
                peer.sendInput( localKeys );
                waitingForPeer = true;
            }

            if ( waitingForPeer && peer.receiveInput( remoteKeys ) ) { 
                // put code in here to run 
                // put code in here to run 
                // put code in here to run 
                // put code in here to run 
                // put code in here to run 
                // put code in here to run 
                // delta time progress in here 
                timeline.setPause(false);
                const KeyboardSnapshot* keys[ 2 ] = {
                    isPlayer1 ? &localKeys : &remoteKeys,
                    isPlayer1 ? &remoteKeys : &localKeys
                };

                for ( int p = 0; p < 2; p++ ) {
                    if ( keys[ p ]->isKeyPressed( SDL_SCANCODE_W ) ) squareY[ p ] -= P2P_SPEED;
                    if ( keys[ p ]->isKeyPressed( SDL_SCANCODE_S ) ) squareY[ p ] += P2P_SPEED;
                    if ( keys[ p ]->isKeyPressed( SDL_SCANCODE_A ) ) squareX[ p ] -= P2P_SPEED;
                    if ( keys[ p ]->isKeyPressed( SDL_SCANCODE_D ) ) squareX[ p ] += P2P_SPEED;

                }

                frame++;
                waitingForPeer = false;

                if ( frame % 60 == 0 ) {
                    SDL_Log( "[%s] frame %u  p1 (%d, %d)  p2 (%d, %d)", playerName, frame,
                            squareX[ 0 ], squareY[ 0 ], squareX[ 1 ], squareY[ 1 ] );
                }
                timeline.setPause(true);
            }

            if ( peer.peerHasLeft() ) {
                SDL_Log( "Other player left" );
                running = false;
            }

            SDL_SetRenderDrawColor( renderer, 30, 30, 30, 255 );
            SDL_RenderClear( renderer );

            for ( int p = 0; p < 2; p++ ) {
                SDL_FRect rect = { (float)squareX[ p ], (float)squareY[ p ],
                    (float)P2P_SQUARE_SIZE, (float)P2P_SQUARE_SIZE };
                if ( p == 0 ) {
                    SDL_SetRenderDrawColor( renderer, 220, 60, 60, 255 );
                } else {
                    SDL_SetRenderDrawColor( renderer, 60, 120, 220, 255 );
                }
                SDL_RenderFillRect( renderer, &rect );
            }

            SDL_RenderPresent( renderer );
        }

        peer.disconnect();


    }

    SDL_DestroyRenderer( renderer );
    SDL_DestroyWindow( window );
    SDL_Quit();
    return 0;
}
