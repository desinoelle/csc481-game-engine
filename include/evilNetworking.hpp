#ifndef EVIL_NETWORKING
#define EVIL_NETWORKING

#include "coolMath.hpp"
#include "eventManager.hpp"
#include <zmq.hpp>
#include <map>
#include <string>
#include <vector>

const int REQ_PORT = 5555;            
const int PUB_PORT = 5556;
const int CLIENT_PORT_BASE = 6000;    // per client server threads

const std::string EVENT_ARG_SENDER = "sender";

/**
    Messages passed between the client and the server
        client to lobby         "JOIN"
        lobby to one client     "ID <id> <x> <y> <port>"
        client to server        "MOVE <id> <x> <y>"
                                "LEAVE <id>"
                                "EVENT <type> <timestamp> <priority> <count> <args>"  (see eventSerialization.hpp)
        server to one client    "OK"
        server to everyone      "STATE <count> <id> <x> <y> etc"
                                "EVENT <type> <timestamp> <priority> <count> <args>"
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

        // Filled by drainSubscription, which sorts what arrives on the subscribe socket
        std::string latestState;
        std::vector< Event > pendingEvents;

        void drainSubscription();

    public:
        NetworkClient();             
        NetworkClient(int clientId);  // Fixed slot on CLIENT_PORT_BASE + id
        ~NetworkClient();

        bool connectToServer( const std::string& host );

        void sendPosition( const fpVec2& position );

        void pollState();

        /**
            Sends one event to the server and waits for its reply
            Uses the request socket, so call it from the same thread as sendPosition
            Returns false if the server did not answer or the event could not be sent
        */
        bool sendEvent( const Event& event );

        /**
            Every event the server has broadcast since the last call, oldest first
            Unlike STATE, none are skipped because each event only happens once
        */
        std::vector< Event > pollEvents();

        void disconnect();

        int getId() const { return myId; }
        int getMyId() const { return myId; }  // Alias for getId()
        bool isConnected() const { return connected; }
        fpVec2 getSpawn() const { return spawn; }
        const std::map< int, PlayerState >& getPlayers() const { return players; }
};

#endif
