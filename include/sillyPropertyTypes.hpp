#pragma once
#include <SDL3/SDL.h>
#include <algorithm>
#include <functional>
#include <map>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>
#include "coolMath.hpp"
#include "coolPhysics.hpp"

// Property names
const std::string PROP_POSITION = "position";     
const std::string PROP_NET_ID = "netId";          
const std::string PROP_LAST_SEEN = "lastSeen";    
const std::string PROP_MOVEABLE = "moveable";
const std::string PROP_SPRITE = "sprite";     

struct SpriteData {
    SDL_Texture* texture = nullptr;
    int frameCount = 0;
    int frameWidth = 0;
    int frameHeight = 0;
    Uint32 animationDelay = 0;
    int currentFrame = 0;
    Uint32 lastFrameTime = 0;
};

using PropertyValue = std::variant<
    int,
    float,
    bool,
    fp,
    fpVec2,
    collision,
    SDL_FRect,
    SDL_Color,
    std::function<void(const std::string&)>,
    std::string,
    SpriteData,
    PhysicsData
>;

/* PropertyManager
 * Stores which game objects have which properties, and their values.
 *
 * registry_ : property name to get ids of the objects that have it
 * values_ : property name and id to get value
 *
 * Errors:
 * empty property name or id has std::invalid_argument
 * get() on a missing property/object has std::out_of_range
 * get<T>() with the wrong type has std::bad_variant_access
 */
class PropertyManager {
public:
    void registerProperty( const std::string& propertyName ) {
        checkNotEmpty( propertyName );
        registry_.try_emplace( propertyName );
    }

    template < typename T >
    void set( const std::string& propertyName, const std::string& gameId, const T& value ) {
        checkNotEmpty( propertyName );
        checkNotEmpty( gameId );

        values_[ propertyName ][ gameId ] = value;

        auto& owners = registry_[ propertyName ];
        if ( std::find( owners.begin(), owners.end(), gameId ) == owners.end() ) {
            owners.push_back( gameId );
        }
    }

    template < typename T >
    T& get( const std::string& propertyName, const std::string& gameId ) {
        auto propertyIt = values_.find( propertyName );
        if ( propertyIt == values_.end() ) {
            throw std::out_of_range( "IDK this property :( " + propertyName );
        }
        auto objectIt = propertyIt->second.find( gameId );
        if ( objectIt == propertyIt->second.end() ) {
            throw std::out_of_range( "Object '" + gameId + "' has no property :( '" + propertyName + "'" );
        }
        return std::get<T>( objectIt->second );
    }

    bool has( const std::string& propertyName, const std::string& gameId ) const {
        auto propertyIt = values_.find( propertyName );
        return propertyIt != values_.end() && propertyIt->second.count( gameId ) > 0;
    }

    const std::vector<std::string>& objectsWith( const std::string& propertyName ) const {
        static const std::vector<std::string> none;
        auto it = registry_.find( propertyName );
        return it != registry_.end() ? it->second : none;
    }

    void remove( const std::string& propertyName, const std::string& gameId ) {
        auto propertyIt = values_.find( propertyName );
        if ( propertyIt == values_.end() ) return;
        propertyIt->second.erase( gameId );

        auto& owners = registry_[ propertyName ];
        owners.erase( std::remove( owners.begin(), owners.end(), gameId ), owners.end() );
    }

    void removeObject( const std::string& gameId ) {
        for ( auto& [ propertyName, objects ] : values_ ) {
            objects.erase( gameId );
            auto& owners = registry_[ propertyName ];
            owners.erase( std::remove( owners.begin(), owners.end(), gameId ), owners.end() );
        }
    }

    void clear() {
        registry_.clear();
        values_.clear();
    }

private:
    static void checkNotEmpty( const std::string& name ) {
        if ( name.empty() ) {
            throw std::invalid_argument( "Property name and object id must not be empty cause yeah :( " );
        }
    }

    std::map<std::string, std::vector<std::string>> registry_;
    std::map<std::string, std::map<std::string, PropertyValue>> values_;
};


inline PropertyManager& Properties() {
    static PropertyManager manager;
    return manager;
}

template <typename T>
T& GetProperty( const std::string& propertyName, const std::string& gameId ) {
    return Properties().get<T>( propertyName, gameId );
}

template <typename T>
void SetProperty( const std::string& propertyName, const std::string& gameId, const T& value ) {
    Properties().set( propertyName, gameId, value );
}
