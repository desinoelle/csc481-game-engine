#ifndef EVIL_NETWORKING
#define EVIL_NETWORKING

#include "coolMath.hpp"
#include <zmq.hpp>
#include <map>
#include <string>

const int REQ_PORT = 5555;
const int PUB_PORT = 5556;

/**
    Messages passed between the client and the server
        client to server        "JOIN"
                                "MOVE <id> <x> <y>"
                                "LEAVE <id>"
        server to one client    "ID <id> <x> <y>"
                                "OK"
        server to everyone      "STATE <count> <id> <x> <y> etc"
*/

/**
    player as it travels over the wire
*/
struct PlayerState {
    int id = 0;
    double x = 0.0;
    double y = 0.0;
};

/**
    Client side
    request socket and subscribe socket
*/
class NetworkClient {
    private:
        zmq::context_t context;
        zmq::socket_t requestSocket;
        zmq::socket_t subscribeSocket;

        int myId = -1;
        bool connected = false;
        fpVec2 spawn;
        std::map< int, PlayerState > players;

    public:
        NetworkClient();
        ~NetworkClient();

        bool connectToServer( const std::string& host );

        void sendPosition( const fpVec2& position );

        void pollState();

        void disconnect();

        int getId() const { return myId; }
        fpVec2 getSpawn() const { return spawn; }
        const std::map< int, PlayerState >& getPlayers() const { return players; }
};

#endif
