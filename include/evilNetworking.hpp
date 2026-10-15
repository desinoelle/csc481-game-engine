#ifndef EVIL_NETWORKING
#define EVIL_NETWORKING

#include "coolMath.hpp"
#include <zmq.hpp>
#include <map>
#include <string>

const int REQ_PORT = 5555;            
const int PUB_PORT = 5556;
const int CLIENT_PORT_BASE = 6000;    // per client server threads

/**
    Messages passed between the client and the server
        client to lobby         "JOIN"
        lobby to one client     "ID <id> <x> <y> <port>"
        client to server        "MOVE <id> <x> <y>"
                                "LEAVE <id>"
        server to one client    "OK"
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
        bool usesLobby = false;
        fpVec2 spawn;
        std::map< int, PlayerState > players;

    public:
        NetworkClient();             
        NetworkClient(int clientId);  // Fixed slot on CLIENT_PORT_BASE + id
        ~NetworkClient();

        bool connectToServer( const std::string& host );

        void sendPosition( const fpVec2& position );

        void pollState();

        void disconnect();

        int getId() const { return myId; }
        int getMyId() const { return myId; }  // Alias for getId()
        bool isConnected() const { return connected; }
        fpVec2 getSpawn() const { return spawn; }
        const std::map< int, PlayerState >& getPlayers() const { return players; }
};

#endif