#ifndef NETWORK_EVENTS_HPP
#define NETWORK_EVENTS_HPP

#include "eventManager.hpp"
#include "evilNetworking.hpp"

/**
    Connects an EventManager to a NetworkClient so chosen events are shared with every client

    Server-centric: a client sends the event to the server, the server tags it with the sender
    and broadcasts it to everyone, and every other client queues it in its own EventManager
    The game registers handlers the usual way, a handler cannot tell a local event from one
    that came over the network except by the EVENT_ARG_SENDER argument

        NetworkedEvents shared( eventManager, client );
        shared.share( EventType::Spawn );
        ...
        each frame
            shared.update( timeline.getTime() );        receive other players' events
            eventManager.processEvents( timeline.getTime() );

    Events raised on this client travel as they are raised, no extra call is needed
    Only argument types the serializer supports are sent ( int, float, bool, string )

    Call update, and process events, from the thread that owns the NetworkClient
*/
class NetworkedEvents {
    public:
        NetworkedEvents( EventManager& eventManager, NetworkClient& client );
        ~NetworkedEvents();

        NetworkedEvents( const NetworkedEvents& ) = delete;
        NetworkedEvents& operator=( const NetworkedEvents& ) = delete;

        /**
            Events of this type raised on this client are also sent to the server
            Do not share Input events, a held key raises one every frame
        */
        void share( EventType type );

        /**
            Queues the events other clients have raised, stamped with this client's current time
            Our own events come back from the server too and are skipped, they were already handled
        */
        void update( double now );

        // For debugging and the write-up
        int sentCount() const { return sent; }
        int failedCount() const { return failed; }
        int receivedCount() const { return received; }

    private:
        EventManager& eventManager;
        NetworkClient& client;
        std::vector< std::pair< EventType, int > > handlerIds;

        int sent = 0;
        int failed = 0;
        int received = 0;

        void forward( const Event& event );
};

#endif