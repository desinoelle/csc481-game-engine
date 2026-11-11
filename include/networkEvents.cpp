#include "networkEvents.hpp"
#include "eventSerialization.hpp"
#include "replaySystem.hpp"

#include <algorithm>
#include <iostream>

namespace {
    // Same test as isReplayed, kept here so this file does not need replaySystem.cpp to link
    bool replayedEvent( const Event& event ) {
        auto it = event.args.find( REPLAY_ARG_REPLAYED );
        if ( it == event.args.end() ) {
            return false;
        }
        const bool* replayed = std::get_if< bool >( &it->second );
        return replayed && *replayed;
    }
}

NetworkedEvents::NetworkedEvents( EventManager& eventManager, NetworkClient& client )
    : eventManager( eventManager ), client( client ) {
}

NetworkedEvents::~NetworkedEvents() {
    for ( const auto& [ type, id ] : handlerIds ) {
        eventManager.unregisterHandler( type, id );
    }
}

void NetworkedEvents::share( EventType type ) {
    for ( const auto& entry : handlerIds ) {
        if ( entry.first == type ) {
            return;     // already shared
        }
    }
    int id = eventManager.registerHandler( type, [ this ]( const Event& event ) { forward( event ); } );
    handlerIds.push_back( { type, id } );
}

/**
    Runs when a shared event is handled on this client
    Only events that started here are sent, otherwise an event would bounce between clients forever
*/
void NetworkedEvents::forward( const Event& event ) {
    bool fromNetwork = event.args.count( EVENT_ARG_SENDER ) > 0;
    if ( fromNetwork || replayedEvent( event ) ) {
        return;
    }

    if ( client.sendEvent( event ) ) {
        sent++;
    }
    else {
        failed++;
        std::cerr << "Could not send a " << eventTypeName( event.type ) << " event to the server" << std::endl;
    }
}

void NetworkedEvents::update( double now ) {
    for ( Event& event : client.pollEvents() ) {
        auto sender = event.args.find( EVENT_ARG_SENDER );
        if ( sender != event.args.end() ) {
            const int* senderId = std::get_if< int >( &sender->second );
            if ( senderId && *senderId == client.getId() ) {
                continue;       // our own event coming back
            }
        }

        // The sender's timeline is not ours, so handle it as soon as it arrives
        event.timestamp = now;
        eventManager.queueEvent( event );
        received++;
    }
}