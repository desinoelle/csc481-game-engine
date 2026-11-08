#include "evilNetworking.hpp"
#include "eventSerialization.hpp"

#include <iomanip>
#include <iostream>
#include <sstream>

/**
    Creates both sockets and connects to the server on the client's unique port
*/
NetworkClient::NetworkClient( int clientId ): 
      context( 1 ),
      requestSocket( context, zmq::socket_type::req ),
      subscribeSocket( context, zmq::socket_type::sub ) {

        // Calculate unique port for this client (6000+)
        int myPort = CLIENT_PORT_BASE + clientId;
        std::string connectAddr = "tcp://localhost:" + std::to_string( myPort );
        
        requestSocket.set( zmq::sockopt::linger, 0 );
        requestSocket.set( zmq::sockopt::rcvtimeo, 2000 );
        requestSocket.connect( connectAddr );
        
        subscribeSocket.set( zmq::sockopt::linger, 0 );
        subscribeSocket.connect( "tcp://localhost:" + std::to_string( PUB_PORT ) );
        subscribeSocket.set( zmq::sockopt::subscribe, "STATE" );
        subscribeSocket.set( zmq::sockopt::subscribe, "EVENT" );
}

/**
    Creates both sockets, connectToServer connects them through the lobby
*/
NetworkClient::NetworkClient()
    : context(1),
      requestSocket(context, zmq::socket_type::req),
      subscribeSocket(context, zmq::socket_type::sub),
      usesLobby(true) {
}

NetworkClient::~NetworkClient() {
    disconnect();
}

/**
    Sends JOIN and stores the id and spawn point the server sends back
    The reply also names this client's server port through the lobby
    Returns false when the server is not up
*/
bool NetworkClient::connectToServer( const std::string& host ) {
    if ( usesLobby ) {
        requestSocket.set( zmq::sockopt::linger, 0 );
        requestSocket.set( zmq::sockopt::rcvtimeo, 2000 );
        requestSocket.connect( "tcp://" + host + ":" + std::to_string( REQ_PORT ) );

        subscribeSocket.set( zmq::sockopt::linger, 0 );
        subscribeSocket.connect( "tcp://" + host + ":" + std::to_string( PUB_PORT ) );
        subscribeSocket.set( zmq::sockopt::subscribe, "STATE" );
        subscribeSocket.set( zmq::sockopt::subscribe, "EVENT" );
    }

    std::string join = "JOIN";
    requestSocket.send( zmq::message_t( join.begin(), join.end() ), zmq::send_flags::none );

    zmq::message_t reply;
    if ( !requestSocket.recv( reply, zmq::recv_flags::none ) ) {
        std::cerr << "No reply from the server, is server running?" << std::endl;
        return false;
    }

    // ID <id> <x> <y> <port>
    std::string replyText( static_cast< char* >( reply.data() ), reply.size() );
    std::istringstream in( replyText );
    std::string tag;
    double x = 0.0;
    double y = 0.0;
    int port = 0;
    in >> tag >> myId >> x >> y;
    if ( usesLobby ) {
        in >> port;
    }

    if ( tag != "ID" || in.fail() ) {
        std::cerr << "Could not read the server reply: " << replyText << std::endl;
        return false;
    }

    if ( usesLobby ) {
        requestSocket.close();
        requestSocket = zmq::socket_t( context, zmq::socket_type::req );
        requestSocket.set( zmq::sockopt::linger, 0 );
        requestSocket.set( zmq::sockopt::rcvtimeo, 2000 );
        requestSocket.connect( "tcp://" + host + ":" + std::to_string( port ) );
    }

    spawn = fpVec2{ fp( x ), fp( y ) };
    connected = true;
    return true;
}

/**
    Sends where this client's character is
*/
void NetworkClient::sendPosition( const fpVec2& position ) {
    if ( !connected ) {
        return;
    }

    std::ostringstream out;
    out << std::setprecision( 17 );
    out << "MOVE " << myId << " " << position.x.toDouble() << " " << position.y.toDouble();
    std::string message = out.str();

    requestSocket.send( zmq::message_t( message.begin(), message.end() ), zmq::send_flags::none );

    zmq::message_t reply;
    if ( !requestSocket.recv( reply, zmq::recv_flags::none ) ) {
        std::cerr << "Lost contact with the server" << std::endl;
        connected = false;
    }
}

/**
    Reads every message waiting on the subscribe socket and sorts it
        STATE   only the newest is kept
        EVENT   every one is kept, in the order they arrived
*/
void NetworkClient::drainSubscription() {
    zmq::message_t message;
    while ( subscribeSocket.recv( message, zmq::recv_flags::dontwait ) ) {
        std::string text( static_cast< char* >( message.data() ), message.size() );

        if ( text.compare( 0, 5, "STATE" ) == 0 ) {
            latestState = text;
        }
        else if ( text.compare( 0, 5, "EVENT" ) == 0 ) {
            Event event;
            if ( parseEvent( text, event ) ) {
                pendingEvents.push_back( event );
            }
            else {
                std::cerr << "Ignoring a malformed event: " << text << std::endl;
            }
        }
    }
}

/**
    Reads the newest state message waiting in the queue
    Access information about others 
*/
void NetworkClient::pollState() {
    if ( !connected ) {
        return;
    }

    drainSubscription();

    std::string latest;
    latest.swap( latestState );

    if ( latest.empty() ) {
        return;
    }

    // STATE <count> <id> <x> <y> ...
    std::istringstream in( latest );
    std::string tag;
    int count = 0;
    in >> tag >> count;

    if ( tag != "STATE" || in.fail() ) {
        return;
    }

    players.clear();
    for ( int i = 0; i < count; i++ ) {
        PlayerState player;
        in >> player.id >> player.x >> player.y;
        if ( in.fail() ) {
            return;
        }
        players[ player.id ] = player;
    }
}

/**
    Sends an event to the server, which decides what it means
*/
bool NetworkClient::sendEvent( const Event& event ) {
    if ( !connected ) {
        return false;
    }

    std::string message = serializeEvent( event );
    requestSocket.send( zmq::message_t( message.begin(), message.end() ), zmq::send_flags::none );

    zmq::message_t reply;
    if ( !requestSocket.recv( reply, zmq::recv_flags::none ) ) {
        std::cerr << "Lost contact with the server" << std::endl;
        connected = false;
        return false;
    }

    std::string replyText( static_cast< char* >( reply.data() ), reply.size() );
    return replyText == "OK";
}

/**
    Hands over the events received so far and forgets them
*/
std::vector< Event > NetworkClient::pollEvents() {
    if ( !connected ) {
        return {};
    }

    drainSubscription();

    std::vector< Event > events;
    events.swap( pendingEvents );
    return events;
}

/**
    Tells the server this client is leaving
*/
void NetworkClient::disconnect() {
    if ( !connected ) {
        return;
    }
    connected = false;

    std::ostringstream out;
    out << "LEAVE " << myId;
    std::string message = out.str();

    requestSocket.send( zmq::message_t( message.begin(), message.end() ), zmq::send_flags::none );

    zmq::message_t reply;
    static_cast< void >( requestSocket.recv( reply, zmq::recv_flags::none ) );
}