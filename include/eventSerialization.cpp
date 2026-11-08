#include "eventSerialization.hpp"

#include <cstdio>
#include <cstdlib>
#include <iomanip>
#include <sstream>

namespace {

    // Keep this list in step with enum class EventType in eventManager.hpp
    const char* const EVENT_TYPE_NAMES[] = { "Collision", "Death", "Spawn", "Input" };
    const int EVENT_TYPE_COUNT = sizeof( EVENT_TYPE_NAMES ) / sizeof( EVENT_TYPE_NAMES[ 0 ] );

    /**
        Percent-encodes anything that is not a letter, digit or _ or .
    */
    std::string encode( const std::string& text ) {
        std::string out;
        for( unsigned char c : text ) {
            bool safe = ( c >= 'a' && c <= 'z' ) || ( c >= 'A' && c <= 'Z' ) ||
                        ( c >= '0' && c <= '9' ) || c == '_' || c == '.';
            if ( safe ) {
                out += static_cast< char >( c );
            } else {
                char buffer[ 4 ];
                std::snprintf( buffer, sizeof( buffer ), "%%%02X", c );
                out += buffer;
            }
        }
        return out;
    }

    int hexValue( char c ) {
        if ( c >= '0' && c <= '9' ) return c - '0';
        if ( c >= 'a' && c <= 'f' ) return c - 'a' + 10;
        if ( c >= 'A' && c <= 'F' ) return c - 'A' + 10;
        return -1;
    }

    /**
        Reverse of encode, false when a % is not followed by two hex digits
    */
    bool decode( const std::string& text, std::string& out ) {
        out.clear();
        for ( size_t i = 0; i < text.size(); i++ ) {
            if ( text[ i ] != '%' ) {
                out += text[ i ];
                continue;
            }
            if ( i + 2 >= text.size() ) {
                return false;       // not enough characters left for two hex digits
            }
            int high = hexValue( text[ i + 1 ] );
            int low = hexValue( text[ i + 2 ] );
            if ( high < 0 || low < 0 ) {
                return false;
            }
            out += static_cast< char >( high * 16 + low );
            i += 2;
        }
        return true;
    }
}

const char* eventTypeName( EventType type ) {
    int index = static_cast< int >( type );
    if ( index < 0 || index >= EVENT_TYPE_COUNT ) {
        return "Unknown";
    }
    return EVENT_TYPE_NAMES[ index ];
}

bool parseEventType( const std::string& name, EventType& out ) {
    for ( int i = 0; i < EVENT_TYPE_COUNT; i++ ) {
        if ( name == EVENT_TYPE_NAMES[ i ] ) {
            out = static_cast< EventType >( i );
            return true;
        }
    }
    return false;
}

std::string serializeEvent( const Event& event ) {
    // Work out the sendable args first so the count in the message is right
    std::ostringstream args;
    args << std::setprecision( 9 );
    int count = 0;

    for ( const auto& [ key, value ] : event.args ) {
        std::ostringstream one;
        one << std::setprecision( 9 );

        if ( const int* v = std::get_if< int >( &value ) ) {
            one << "i:" << *v;
        } else if ( const float* v = std::get_if< float >( &value ) ) {
            one << "f:" << *v;
        } else if ( const bool* v = std::get_if< bool >( & value ) ) {
            one << "b:" << ( *v ? 1 : 0 );
        } else if ( const std::string* v = std::get_if< std::string >( &value ) ) {
            one << "s:" << encode( *v );
        } else {
            continue;   //type that does not go over the wire
        }

        args << " " << encode( key ) << "=" << one.str();
        count++;
    }

    std::ostringstream out;
    out << std::setprecision( 17 );
    out << "EVENT " << eventTypeName( event.type ) << " " << event.timestamp << " "
        << event.priority << " " << count << args.str();
    return out.str();
}

bool parseEvent( const std::string& text, Event& out ) {
    std::istringstream in( text );

    std::string tag;
    std::string typeName;
    Event event;
    int count = 0;

    in >> tag >> typeName >> event.timestamp >> event.priority >> count;
    if ( in.fail() || tag != "EVENT" || count < 0 ) {
        return false;
    }
    if ( !parseEventType( typeName, event.type ) ) {
        return false;
    }

    for ( int i = 0; i < count; i++ ) {
        std::string token;
        if ( !( in >> token ) ) {
            return false;
        }

        // <key>=<kind>:<value>
        size_t equals = token.find( '=' );
        if ( equals == std::string::npos || equals + 2 >= token.size() || token[ equals + 2 ] != ':' ) {
            return false;
        }

        std::string key;
        if ( !decode( token.substr( 0, equals ), key ) || key.empty() ) {
            return false;
        }

        char kind = token[ equals + 1 ];
        std::string raw = token.substr( equals + 3 );

        if ( kind == 'i' ) {
            char* end = nullptr;
            long value = std::strtol( raw.c_str(), &end, 10 );
            if ( raw.empty() || *end != '\0' ) return false;
            event.args[ key ] = static_cast< int >( value );
        } else if ( kind == 'f' ) {
            char* end = nullptr;
            float value = std::strtof( raw.c_str(), &end );
            if ( raw.empty() || *end != '\0' ) return false;
            event.args[ key ] = value;
        } else if ( kind == 'b' ) {
            if ( raw != "0" && raw != "1" ) return false;
            event.args[ key ] = ( raw == "1" );
        } else if ( kind == 's' ) {
            std::string value;
            if ( !decode( raw, value ) ) return false;
            event.args[ key ] = value;
        } else {
            return false;
        }
    }

    out = event;
    return true;
}