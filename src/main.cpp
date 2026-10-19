/**
    ./coolEngine client [host]    networked scene
    run ./coolServer first
    ./coolEngine 1 / 2            p2p
*/

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cmath>
#include <functional>
#include <map>
#include <mutex>
#include <string>
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

// Networked scene
const int SCENE_WIDTH = 800;
const int SCENE_HEIGHT = 600;
const int PLAYER_SIZE = 40;
const fp PLAYER_SPEED = fp( int64_t( 300 ) );

const int CLOCK_ID = 0;

using MoveHandler = std::function< void( const std::string& ) >;

/**
    Moves an object with WASD
*/
void moveWithKeyboard( const std::string& id, const InputSystem& input, fp dt ) {
    fpVec2 direction;
    if ( input.isKeyPressed( SDL_SCANCODE_W ) ) direction.y -= fp( int64_t( 1 ) );
    if ( input.isKeyPressed( SDL_SCANCODE_S ) ) direction.y += fp( int64_t( 1 ) );
    if ( input.isKeyPressed( SDL_SCANCODE_A ) ) direction.x -= fp( int64_t( 1 ) );
    if ( input.isKeyPressed( SDL_SCANCODE_D ) ) direction.x += fp( int64_t( 1 ) );

    GetProperty< fpVec2 >( PROP_POSITION, id ) += direction * ( PLAYER_SPEED * dt );
}

/**
    Data the main thread and network thread share
    main thread - writes the position and requests sends, reads the server clock and players
    network thread - reads the position, writes the server clock and players
*/
class ClientShared {
private:
    std::mutex lock;
    fpVec2 position;
    bool sendRequested = false;
    bool connected = true;
    double serverTime = 0.0;
    std::map< int, PlayerState > players;

public:
    void setPosition( const fpVec2& p ) { std::lock_guard< std::mutex > guard( lock ); position = p; }
    fpVec2 getPosition() { std::lock_guard< std::mutex > guard( lock ); return position; }

    void requestSend() { std::lock_guard< std::mutex > guard( lock ); sendRequested = true; }
    bool takeSendRequest() {
        std::lock_guard< std::mutex > guard( lock );
        bool requested = sendRequested;
        sendRequested = false;     // a late thread sends the newest position
        return requested;
    }

    void setConnected( bool c ) { std::lock_guard< std::mutex > guard( lock ); connected = c; }
    bool isConnected() { std::lock_guard< std::mutex > guard( lock ); return connected; }

    void setServerTime( double t ) { std::lock_guard< std::mutex > guard( lock ); serverTime = t; }
    double getServerTime() { std::lock_guard< std::mutex > guard( lock ); return serverTime; }

    void setPlayers( const std::map< int, PlayerState >& p ) { std::lock_guard< std::mutex > guard( lock ); players = p; }
    std::map< int, PlayerState > getPlayers() { std::lock_guard< std::mutex > guard( lock ); return players; }
};

/**
    The server sends its clock as a fake player with CLOCK_ID
    Takes it out of the player list and returns it, or -1 if this STATE had none
*/
double takeServerClock( std::map< int, PlayerState >& players ) {
    if ( players.count( CLOCK_ID ) == 0 ) {
        return -1.0;
    }
    double seconds = players[ CLOCK_ID ].x;
    players.erase( CLOCK_ID );
    return seconds;
}

/**
    Network thread sends MOVE, keeps the STATE
*/
void networkLoop( NetworkClient& network, ClientShared& shared, const std::atomic< bool >& running ) {
    double lastClock = -1.0;
    Uint64 lastClockArrivedMs = 0;

    while ( running ) {
        if ( shared.takeSendRequest() ) {
            network.sendPosition( shared.getPosition() );
            if ( !network.isConnected() ) {
                shared.setConnected( false );
                return;
            }
        }
        network.pollState();

        std::map< int, PlayerState > players = network.getPlayers();
        double clock = takeServerClock( players );
        if ( clock >= 0.0 && clock != lastClock ) {
            lastClock = clock;
            lastClockArrivedMs = SDL_GetTicks();
        }

        if ( lastClock >= 0.0 ) {
            double secondsSince = ( SDL_GetTicks() - lastClockArrivedMs ) / 1000.0;
            shared.setServerTime( lastClock + secondsSince );
        }
        shared.setPlayers( players );

        SDL_Delay( 1 );
    }
}

/**
    Makes the remote player entities match the STATE
    New ids get an entity, ids that are gone are removed
*/
void syncRemotePlayers( std::map< int, Entity >& remotePlayers, const std::map< int, PlayerState >& players, int myId ) {
    // Find the players not in STATE
    std::vector< int > gone;
    for ( const auto& [ id, entity ] : remotePlayers ) {
        if ( players.count( id ) == 0 ) {
            gone.push_back( id );
        }
    }

    // Remove them from the scene
    for ( int id : gone ) {
        Properties().removeObject( remotePlayers[ id ].id );
        remotePlayers.erase( id );
    }

    // Add new players
    for ( const auto& [ id, state ] : players ) {
        if ( id == myId ) {
            continue;
        }

        bool isNew = remotePlayers.count( id ) == 0;
        Entity& remote = remotePlayers[ id ];   // creates the entity
        if ( isNew ) {
            remote.setRectangleHeightAndWidth( fp( int64_t( PLAYER_SIZE ) ), fp( int64_t( PLAYER_SIZE ) ) );
            remote.setColor( 220, 60, 60, 255 );
        }
        remote.position() = fpVec2{ state.x, state.y };
    }
}

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

    bool isPeer2Peer = !( argc > 1 && std::string( argv[1] ) == "client" );


    if(!isPeer2Peer) {
        std::string host = argc > 2 ? argv[2] : "localhost";

        // server gives id and spawn point upon joining
        NetworkClient networkClient;
        if (!networkClient.connectToServer(host)) {
            SDL_Log("Failed to connect to server!");
            SDL_DestroyRenderer(renderer);
            SDL_DestroyWindow(window);
            SDL_Quit();
            return 1;
        }
        
        int myId = networkClient.getMyId();

        std::string title = "Client " + std::to_string( myId );
        SDL_SetWindowTitle( window, title.c_str() );
        SDL_SetWindowSize( window, SCENE_WIDTH, SCENE_HEIGHT );
        SDL_SetWindowPosition( window, 20 + ( ( myId - 1 ) % 4 ) * 120, 60 + ( ( myId - 1 ) % 4 ) * 60 );
        SDL_SetRenderVSync( renderer, 1 );

        Entity player;
        player.position() = networkClient.getSpawn();
        player.setRectangleHeightAndWidth( fp( int64_t( PLAYER_SIZE ) ), fp( int64_t( PLAYER_SIZE ) ) );
        player.setColor( 255, 165, 0, 255 );

        if ( !loadSprite( player.id, "test_sheet.png", renderer, 4, 40, 40, 200 ) ) {
            SDL_Log( "Could not load the test sprite" );
        }

        SetProperty( PROP_MOVEABLE, player.id, MoveHandler( [ & ]( const std::string& id ) {
            moveWithKeyboard( id, inputSystem, fp( timeline.getDeltaTime() ) );
        } ) );

        std::map< int, Entity > remotePlayers;

        // Network thread sends and reads, main thread runs
        ClientShared shared;
        shared.setPosition( player.position() );
        std::atomic< bool > networkRunning{ true };
        std::thread networkThread( networkLoop, std::ref( networkClient ), std::ref( shared ), std::cref( networkRunning ) );

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

            // pause control
            bool returnDown = inputSystem.isKeyPressed(SDL_SCANCODE_RETURN);
            if (returnDown && !sharedData.returnLastFrame) {
                timeline.setPause(!timeline.isPaused());
            }
            sharedData.returnLastFrame = returnDown;

            // Run every object's moveable handler
            for ( const std::string& id : Properties().objectsWith( PROP_MOVEABLE ) ) {
                GetProperty< MoveHandler >( PROP_MOVEABLE, id )( id );
            }

            updateSprites();

            // Send frame
            shared.setPosition( player.position() );
            shared.requestSend();

            if ( !shared.isConnected() ) {
                SDL_Log("[Client %d] Lost the server, closing", myId);
                running = false;
            }

            // Other players come from STATE
            syncRemotePlayers( remotePlayers, shared.getPlayers(), myId );

            // Clear screen
            SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
            SDL_RenderClear(renderer);

            for ( auto& entry : remotePlayers ) {
                entry.second.draw( renderer );
            }
            player.draw( renderer );

            char hud[ 96 ];
            std::snprintf( hud, sizeof( hud ), "%s   players %zu   server %.1fs%s", title.c_str(),
                           remotePlayers.size() + 1, shared.getServerTime(), timeline.isPaused() ? "   PAUSED" : "" );
            SDL_SetRenderDrawColor( renderer, 255, 255, 255, 255 );
            SDL_RenderDebugText( renderer, 10.0f, 10.0f, hud );

            SDL_RenderPresent(renderer);
        }

        networkRunning = false;
        networkThread.join();
        networkClient.disconnect();
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

                waitingForPeer = false;
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

    unloadSprites();
    SDL_DestroyRenderer( renderer );
    SDL_DestroyWindow( window );
    SDL_Quit();
    return 0;
}
