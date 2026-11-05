#include "eventManager.hpp"

/**
    Earlier timestamps first
    then higher priority
*/
bool EventManager::HandledLater::operator()( const QueuedEvent& a, const QueuedEvent& b ) const {
    if ( a.event.timestamp != b.event.timestamp ) {
        return a.event.timestamp > b.event.timestamp;
    }
    if ( a.event.priority != b.event.priority ) {
        return a.event.priority < b.event.priority;
    }
    return a.order > b.order;
}

/**
    Registers handler for one event type
    return id for unregistering it
*/
int EventManager::registerHandler( EventType type, EventHandler handler ) {
    std::lock_guard< std::mutex > guard( lock );
    int id = nextHandlerId++;
    handlers[ type ][ id ] = handler;
    return id;
}

void EventManager::unregisterHandler( EventType type, int handlerId ) {
    std::lock_guard< std::mutex > guard( lock );
    handlers[ type ].erase( handlerId );
}

void EventManager::queueEvent( const Event& event ) {
    std::lock_guard< std::mutex > guard( lock );
    queue.push( QueuedEvent{ event, nextOrder++ } );
}

void EventManager::queueEvent( EventType type, double timestamp, int priority,
                               const std::map< std::string, PropertyValue >& args ) {
    Event event;
    event.type = type;
    event.timestamp = timestamp;
    event.priority = priority;
    event.args = args;
    queueEvent( event );
}

/**
 * Take event that is ready 
 * Pop from queue
 */
std::vector< EventManager::QueuedEvent > EventManager::takeReadyEvents( double now ) {
    std::lock_guard< std::mutex > guard( lock );
    std::vector< QueuedEvent > ready;
    while ( !queue.empty() && queue.top().event.timestamp <= now ) {
        ready.push_back( queue.top() );
        queue.pop();
    }
    return ready;
}

/**
 * Get all handlers for a specific event type
 */
std::vector< EventHandler > EventManager::handlersFor( EventType type ) {
    std::lock_guard< std::mutex > guard( lock );
    std::vector< EventHandler > list;
    for ( const auto& [ id, handler ] : handlers[ type ] ) {
        list.push_back( handler );
    }
    return list;
}

/**
    Process all events that are ready to be handled
    now     timeline time, events with timestamp <= now will be handled
*/
void EventManager::processEvents( double now ) {
    for ( const QueuedEvent& queued : takeReadyEvents( now ) ) {
        for ( const EventHandler& handler : handlersFor( queued.event.type ) ) {
            handler( queued.event );
        }
    }
}
