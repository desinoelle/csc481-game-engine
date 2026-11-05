#ifndef EVENT_MANAGER_HPP
#define EVENT_MANAGER_HPP

#include "sillyPropertyTypes.hpp"
#include <functional>
#include <map>
#include <mutex>
#include <queue>
#include <string>
#include <vector>

enum class EventType {
    Collision,
    Death,
    Spawn,
    Input
};

/**
    type        type of enum
    timestamp   timeline time it should be handled at, earlier first
    priority    breaks ties between events with the same timestamp, higher first
    args        extra data
*/
struct Event {
    EventType type;
    double timestamp = 0.0;
    int priority = 0;
    std::map< std::string, PropertyValue > args;
};

using EventHandler = std::function< void( const Event& ) >;

/**
    Keeps track of events and queues events until their timestamp is reached, then dispatches them to handlers
*/
class EventManager {
    private:
        // ordered events
        struct QueuedEvent {
            Event event;
            long long order;
        };

        struct HandledLater {
            bool operator()( const QueuedEvent& a, const QueuedEvent& b ) const;
        };

        std::priority_queue< QueuedEvent, std::vector< QueuedEvent >, HandledLater > queue;
        std::map< EventType, std::map< int, EventHandler > > handlers;
        int nextHandlerId = 1;
        long long nextOrder = 0;
        std::mutex lock;

        std::vector< QueuedEvent > takeReadyEvents( double now );
        std::vector< EventHandler > handlersFor( EventType type );

    public:
        int registerHandler( EventType type, EventHandler handler );
        void unregisterHandler( EventType type, int handlerId );

        void queueEvent( const Event& event );
        void queueEvent( EventType type, double timestamp, int priority = 0,
                         const std::map< std::string, PropertyValue >& args = {} );

        void processEvents( double now );
};

#endif
