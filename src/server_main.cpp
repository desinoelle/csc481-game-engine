/**
    Asynchronous Server

    Lobby thread accepts multiple clients, every JOIN gets its own
    client thread on CLIENT_PORT_BASE + id
    One broadcast thread for publishing state at fixed ~60 Hz
    This allows clients to run at different speeds without affecting others.

    A client that sends LEAVE is removed
*/

#include "evilNetworking.hpp"
#include "sillyPropertyTypes.hpp"

#include <SDL3/SDL.h>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

std::mutex propertiesLock;
int nextId = 1;

const int CLOCK_ID = 0;
const int PLAYER_TIMEOUT_MS = 2000;
const fpVec2 SPAWN_POINT = fpVec2( 400.0, 300.0 );

std::string playerKey( int id ) {
    return "player_" + std::to_string( id );
}

/**
    Milliseconds since the program started
*/
int nowMs() {
    return static_cast< int >( SDL_GetTicks() );
}

/**
    zmq message
*/
std::string toText( const zmq::message_t& message ) {
    return std::string( static_cast< const char* >( message.data() ), message.size() );
}

void sendText( zmq::socket_t& socket, const std::string& text ) {
    socket.send( zmq::message_t( text.begin(), text.end() ), zmq::send_flags::none );
}

/**
    Checks if the player is still in the game
*/
bool playerExists( const std::string& key ) {
    std::lock_guard< std::mutex > lock( propertiesLock );
    return Properties().has( PROP_NET_ID, key );
}

/**
    Creates a new player at SPAWN_POINT and returns id
*/
int addPlayer( fpVec2& spawn ) {
    std::lock_guard< std::mutex > lock( propertiesLock );
    int id = nextId++;
    spawn = SPAWN_POINT;

    std::string key = playerKey( id );
    SetProperty( PROP_NET_ID, key, id );
    SetProperty( PROP_POSITION, key, spawn );
    SetProperty( PROP_LAST_SEEN, key, nowMs() );
    return id;
}

/**
    Stores a MOVE, returns false if the player is already gone
*/
bool movePlayer( const std::string& key, double x, double y ) {
    std::lock_guard< std::mutex > lock( propertiesLock );
    if ( !Properties().has( PROP_NET_ID, key ) ) {
        return false;
    }
    SetProperty( PROP_POSITION, key, fpVec2( x, y ) );
    SetProperty( PROP_LAST_SEEN, key, nowMs() );
    return true;
}

void removePlayer( const std::string& key ) {
    std::lock_guard< std::mutex > lock( propertiesLock );
    Properties().removeObject( key );
}

/**
    Removes every player that hasn't sent anything for PLAYER_TIMEOUT_MS
*/
void removeTimedOutPlayers() {
    std::lock_guard< std::mutex > lock( propertiesLock );

    // Copy the list first
    std::vector< std::string > keys = Properties().objectsWith( PROP_NET_ID );
    for ( const std::string& key : keys ) {
        int quietFor = nowMs() - GetProperty< int >( PROP_LAST_SEEN, key );
        if ( quietFor > PLAYER_TIMEOUT_MS ) {
            Properties().removeObject( key );
        }
    }
}

/**
    STATE <count> <id> <x> <y> ... <CLOCK_ID> <seconds> 0
    The server clock goes last with id CLOCK_ID
*/
std::string buildStateMessage() {
    std::lock_guard< std::mutex > lock( propertiesLock );
    const std::vector< std::string >& keys = Properties().objectsWith( PROP_NET_ID );

    std::ostringstream out;
    out << std::setprecision( 17 );
    out << "STATE " << keys.size() + 1;     // + 1 for the clock

    for ( const std::string& key : keys ) {
        int id = GetProperty< int >( PROP_NET_ID, key );
        fpVec2 position = GetProperty< fpVec2 >( PROP_POSITION, key );
        out << " " << id << " " << position.x.toDouble() << " " << position.y.toDouble();
    }

    double serverSeconds = nowMs() / 1000.0;
    out << " " << CLOCK_ID << " " << serverSeconds << " 0";
    return out.str();
}

/**
    Per-client thread
    MOVE <id> <x> <y>   
    LEAVE <id>          
    Ends once its player has left / timed out
*/
void clientThread( zmq::context_t& context, int id, int port ) {
    try {
        zmq::socket_t socket(context, zmq::socket_type::rep);
        socket.set( zmq::sockopt::linger, 0 );
        socket.set( zmq::sockopt::rcvtimeo, 1000 );     // wake up every second to check on the player
        socket.bind("tcp://*:" + std::to_string(port));

        std::string key = playerKey( id );

        while (true) {
            zmq::message_t request;
            if (!socket.recv(request, zmq::recv_flags::none)) {
                if ( !playerExists( key ) ) {
                    break;      
                }
                continue;
            }

            std::istringstream in( toText( request ) );
            std::string command;
            in >> command;

            if ( command == "MOVE" ) {
                int moveId = 0;
                double x = 0.0;
                double y = 0.0;
                in >> moveId >> x >> y;

                bool moved = !in.fail() && moveId == id && movePlayer( key, x, y );
                sendText( socket, moved ? "OK" : "ERR" );
            }
            else if ( command == "LEAVE" ) {
                removePlayer( key );
                sendText( socket, "OK" );
                break;
            }
            else {
                sendText( socket, "ERR" );
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "[server] Client thread error: " << e.what() << std::endl;
    }
}

/**
    Lobby thread
        JOIN    ->  ID <id> <x> <y> <port>
    Every JOIN creates a player and starts its own client thread
*/
void lobbyThread( zmq::context_t& context ) {
    zmq::socket_t socket( context, zmq::socket_type::rep );
    socket.bind( "tcp://*:" + std::to_string( REQ_PORT ) );

    while ( true ) {
        zmq::message_t request;
        if ( !socket.recv( request, zmq::recv_flags::none ) ) {
            continue;
        }

        if ( toText( request ) != "JOIN" ) {
            sendText( socket, "ERR" );
            continue;
        }

        fpVec2 spawn;
        int id = addPlayer( spawn );
        int port = CLIENT_PORT_BASE + id;
        std::thread( clientThread, std::ref( context ), id, port ).detach();

        std::ostringstream reply;
        reply << std::setprecision( 17 );
        reply << "ID " << id << " " << spawn.x.toDouble() << " " << spawn.y.toDouble() << " " << port;
        sendText( socket, reply.str() );
    }
}

/**
    Broadcast thread, publishes world state
*/
void publishThread(zmq::context_t& context) {
    zmq::socket_t socket(context, zmq::socket_type::pub);
    socket.bind("tcp://*:" + std::to_string(PUB_PORT));
    std::cout << "[server] broadcasting world state on " << PUB_PORT << std::endl;

    while (true) {
        removeTimedOutPlayers();
        sendText( socket, buildStateMessage() );

        // Broadcast at ~60 Hz (16 ms per frame)
        SDL_Delay( 16 );   
    }
}

int main() {
    zmq::context_t context(1);

    // Start broadcast thread (always running)
    std::thread publish(publishThread, std::ref(context));
    std::thread lobby( lobbyThread, std::ref( context ) );

    // Wait for threads (they run forever)
    publish.join();
    lobby.join();

    return 0;
}