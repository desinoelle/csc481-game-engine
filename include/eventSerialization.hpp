#ifndef EVENT_SERIALIZATION_HPP
#define EVENT_SERIALIZATION_HPP

#include "eventManager.hpp"
#include <string>

/**
    Turns an event into one line of text and back,
    so events can be sent over the network
    
    Wire format (one line, tokens separated by single spaces)
        EVENT <type> <timestamp> <priority> <argCount> <key>=<kind>:<value> ...
        
    kind    i   int
            f   float
            b   bool    0 or 1
            s   string
    Any other kind of PropertyValue cannot be sent andis skipped when serializing
    
    Keys and string values are percent-encoded ( space becomes %20, % becomes %25 )
    so they can never break the tokens apart
*/

/**
    Event to text, no trailing newline
*/
std::string serializeEvent( const Event& event );

/**
    Text to Event
    Returns false and leaves out untouched when the text is not a well formed EVENT message
*/
bool parseEvent( const std::string& text, Event& out );

/**
    Name of an event type as it appears on the wire
    parseEventType returns false for a name it does not know
*/
const char* eventTypeName( EventType type );
bool parseEventType( const std::string& name, EventType& out );

#endif