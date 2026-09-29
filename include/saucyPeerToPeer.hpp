#pragma once

#include "input.hpp"

#include <SDL3/SDL.h>
#include <zmq.hpp>

#include <array>
#include <cstdint>
#include <string>

class KeyboardSnapshot {
public:
    void copyFrom( const InputSystem& input );
    void copyFrom( const bool* keys, int numKeys );
    void copyFromBytes( const uint8_t* bytes, int numBytes );

    bool isKeyPressed( SDL_Scancode scancode ) const;

    const bool* data() const { return keys.data(); }
    int size() const { return SDL_SCANCODE_COUNT; }

private:
    std::array< bool, SDL_SCANCODE_COUNT > keys{};
};

class LockstepPeer {
public:
    LockstepPeer( int localPort, int peerPort );
    ~LockstepPeer();

    LockstepPeer( const LockstepPeer& ) = delete;
    LockstepPeer& operator=( const LockstepPeer& ) = delete;

    bool handshake( uint32_t localSeed, int timeoutMs );

    void sendInput( const KeyboardSnapshot& keys );
    bool receiveInput( KeyboardSnapshot& keys );

    uint32_t sharedSeed() const { return seed; }
    bool isConnected() const { return connected; }
    bool peerHasLeft() const { return peerLeft; }

    void disconnect();

private:
    zmq::context_t context;
    zmq::socket_t pushSocket;
    zmq::socket_t pullSocket;

    uint32_t seed = 0;
    bool connected = false;
    bool peerLeft = false;

    void sendBytes( const void* data, size_t size );
    void sendText( const std::string& text );
};
