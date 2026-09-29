#include "saucyPeerToPeer.hpp"

#include <chrono>
#include <iostream>
#include <sstream>
#include <vector>

void KeyboardSnapshot::copyFrom( const InputSystem& input ) {
    for ( int i = 0; i < SDL_SCANCODE_COUNT; i++ ) {
        keys[ i ] = input.isKeyPressed( static_cast< SDL_Scancode >( i ) );
    }
}

void KeyboardSnapshot::copyFrom( const bool* source, int numKeys ) {
    for ( int i = 0; i < SDL_SCANCODE_COUNT; i++ ) {
        keys[ i ] = source && i < numKeys && source[ i ];
    }
}

void KeyboardSnapshot::copyFromBytes( const uint8_t* bytes, int numBytes ) {
    for ( int i = 0; i < SDL_SCANCODE_COUNT; i++ ) {
        keys[ i ] = i < numBytes && bytes[ i ] != 0;
    }
}

bool KeyboardSnapshot::isKeyPressed( SDL_Scancode scancode ) const {
    if ( scancode < 0 || scancode >= SDL_SCANCODE_COUNT ) {
        return false;
    }
    return keys[ scancode ];
}

LockstepPeer::LockstepPeer( int localPort, int peerPort ):
      context( 1 ),
      pushSocket( context, zmq::socket_type::push ),
      pullSocket( context, zmq::socket_type::pull ) {

    pullSocket.set( zmq::sockopt::linger, 0 );
    pullSocket.bind( "tcp://*:" + std::to_string( localPort ) );

    pushSocket.set( zmq::sockopt::linger, 200 );
    pushSocket.connect( "tcp://localhost:" + std::to_string( peerPort ) );
}

LockstepPeer::~LockstepPeer() {
    disconnect();
}

bool LockstepPeer::handshake( uint32_t localSeed, int timeoutMs ) {
    sendText( "HELLO " + std::to_string( localSeed ) );

    auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds( timeoutMs );

    while ( std::chrono::steady_clock::now() < deadline ) {
        zmq::pollitem_t items[] = { { pullSocket.handle(), 0, ZMQ_POLLIN, 0 } };
        zmq::poll( items, 1, std::chrono::milliseconds( 100 ) );

        if ( !( items[ 0 ].revents & ZMQ_POLLIN ) ) {
            continue;
        }

        zmq::message_t message;
        if ( !pullSocket.recv( message, zmq::recv_flags::dontwait ) ) {
            continue;
        }

        std::string text( static_cast< char* >( message.data() ), message.size() );
        std::istringstream in( text );
        std::string tag;
        uint32_t peerSeed = 0;
        in >> tag >> peerSeed;

        if ( tag == "HELLO" && !in.fail() ) {
            seed = localSeed ^ peerSeed;
            connected = true;
            return true;
        }
    }

    std::cerr << "Peer never said HELLO, is the other player running?" << std::endl;
    return false;
}

void LockstepPeer::sendInput( const KeyboardSnapshot& keys ) {
    if ( !connected ) {
        return;
    }

    std::vector< uint8_t > buffer;
    buffer.reserve( 2 + keys.size() );
    buffer.push_back( 'I' );
    buffer.push_back( 'N' );

    const bool* state = keys.data();
    for ( int i = 0; i < keys.size(); i++ ) {
        buffer.push_back( state[ i ] ? 1 : 0 );
    }

    sendBytes( buffer.data(), buffer.size() );
}

bool LockstepPeer::receiveInput( KeyboardSnapshot& keys ) {
    if ( !connected ) {
        return false;
    }

    zmq::message_t message;
    if ( !pullSocket.recv( message, zmq::recv_flags::dontwait ) ) {
        return false;
    }

    const uint8_t* data = static_cast< const uint8_t* >( message.data() );
    size_t size = message.size();

    if ( size >= 2 && data[ 0 ] == 'I' && data[ 1 ] == 'N' ) {
        int numBytes = static_cast< int >( size - 2 );
        if ( numBytes != SDL_SCANCODE_COUNT ) {
            std::cerr << "Keyboard state size mismatch: got " << numBytes
                      << ", expected " << SDL_SCANCODE_COUNT << std::endl;
        }
        keys.copyFromBytes( data + 2, numBytes );
        return true;
    }

    std::string text( reinterpret_cast< const char* >( data ), size );
    if ( text == "BYE" ) {
        peerLeft = true;
    }
    return false;
}

void LockstepPeer::disconnect() {
    if ( !connected ) {
        return;
    }
    connected = false;
    sendText( "BYE" );
}

void LockstepPeer::sendText( const std::string& text ) {
    sendBytes( text.data(), text.size() );
}

void LockstepPeer::sendBytes( const void* data, size_t size ) {
    auto sent = pushSocket.send( zmq::message_t( data, size ), zmq::send_flags::dontwait );
    if ( !sent ) {
        std::cerr << "Send queue full, peer may be gone" << std::endl;
    }
}
