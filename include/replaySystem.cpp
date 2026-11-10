#include "replaySystem.hpp"

#include <algorithm>

namespace {
    // A recording stops by itself at this many events
    const size_t MAX_RECORDED_EVENTS = 200000;

    bool isReplayControl( EventType type ) {
        return type == EventType::ReplayStartRecording || type == EventType::ReplayStopRecording ||
               type == EventType::ReplayStartPlayback || type == EventType::ReplayStopPlayback;
    }

    Event movedEvent( const std::string& id, const fpVec2& position, double timestamp ) {
        Event event;
        event.type = EventType::Moved;
        event.timestamp = timestamp;
        event.args[ MOVED_ARG_OBJECT ] = id;
        event.args[ MOVED_ARG_POSITION ] = position;
        return event;
    }
}

bool isReplayed( const Event& event ) {
    auto it = event.args.find( REPLAY_ARG_REPLAYED );
    if ( it == event.args.end() ) {
        return false;
    }
    const bool* replayed = std::get_if< bool >( &it->second );
    return replayed && *replayed;
}

/**
    Registers the handlers that control the replay and the ones that record
*/
ReplaySystem::ReplaySystem( EventManager& eventManager, const std::vector< EventType >& recordedTypes )
    : eventManager( eventManager ) {

    listen( EventType::ReplayStartRecording, [ this ]( const Event& event ) { startRecording( event.timestamp ); } );
    listen( EventType::ReplayStopRecording, [ this ]( const Event& event ) { stopRecording( event.timestamp ); } );
    listen( EventType::ReplayStartPlayback, [ this ]( const Event& event ) { startPlayback( event.timestamp ); } );
    listen( EventType::ReplayStopPlayback, [ this ]( const Event& event ) { stopPlayback( event.timestamp ); } );

    // Moved is always recorded, the replay controls never are
    std::vector< EventType > types = { EventType::Moved };
    for ( EventType type : recordedTypes ) {
        if ( !isReplayControl( type ) && std::find( types.begin(), types.end(), type ) == types.end() ) {
            types.push_back( type );
        }
    }
    for ( EventType type : types ) {
        listen( type, [ this ]( const Event& event ) { record( event ); } );
    }
}

ReplaySystem::~ReplaySystem() {
    for ( const auto& [ type, id ] : handlerIds ) {
        eventManager.unregisterHandler( type, id );
    }
}

void ReplaySystem::listen( EventType type, EventHandler handler ) {
    handlerIds.push_back( { type, eventManager.registerHandler( type, handler ) } );
}

/**
    Copies a live event into the log while recording
*/
void ReplaySystem::record( const Event& event ) {
    if ( currentState != State::Recording || isReplayed( event ) ) {
        return;
    }

    log.push_back( Recorded{ std::max( 0.0, event.timestamp - recordStart ), event } );

    if ( log.size() >= MAX_RECORDED_EVENTS ) {
        eventManager.queueEvent( EventType::ReplayStopRecording, event.timestamp );
    }
}

/**
    Starts a new recording
    The log begins with where every tracked object is, so playback starts from the same scene
*/
void ReplaySystem::startRecording( double now ) {
    if ( currentState != State::Idle ) {
        return;
    }

    log.clear();
    recordStart = now;
    recordLength = 0.0;

    for ( const std::string& id : tracked ) {
        if ( Properties().has( PROP_POSITION, id ) ) {
            log.push_back( Recorded{ 0.0, movedEvent( id, GetProperty< fpVec2 >( PROP_POSITION, id ), now ) } );
        }
    }

    currentState = State::Recording;
}

void ReplaySystem::stopRecording( double now ) {
    if ( currentState != State::Recording ) {
        return;
    }
    recordLength = std::max( 0.0, now - recordStart );
    currentState = State::Idle;
}

/**
    Starts playing the recording, ending the recording first if it is still going
    Remembers where every recorded object is now so stopPlayback can put it back
*/
void ReplaySystem::startPlayback( double now ) {
    if ( currentState == State::Recording ) {
        stopRecording( now );
    }
    if ( currentState != State::Idle || log.empty() ) {
        return;
    }

    livePositions.clear();
    for ( const Recorded& recorded : log ) {
        if ( recorded.event.type != EventType::Moved ) {
            continue;
        }
        auto it = recorded.event.args.find( MOVED_ARG_OBJECT );
        if ( it == recorded.event.args.end() ) {
            continue;
        }
        const std::string* id = std::get_if< std::string >( &it->second );
        if ( id && livePositions.count( *id ) == 0 && Properties().has( PROP_POSITION, *id ) ) {
            livePositions[ *id ] = GetProperty< fpVec2 >( PROP_POSITION, *id );
        }
    }

    playStart = now;
    nextToPlay = 0;
    currentState = State::Playing;
}

/**
    Ends playback and moves every object back to where it was when playback started
*/
void ReplaySystem::stopPlayback( double now ) {
    if ( currentState != State::Playing ) {
        return;
    }

    for ( const auto& [ id, position ] : livePositions ) {
        queueReplayed( movedEvent( id, position, now ), now );
    }
    livePositions.clear();
    currentState = State::Idle;
}

void ReplaySystem::queueReplayed( Event event, double now ) {
    event.timestamp = now;
    event.args[ REPLAY_ARG_REPLAYED ] = true;
    eventManager.queueEvent( event );
}

void ReplaySystem::raiseMoved( double now, const std::vector< std::string >& ids ) {
    tracked = ids;

    for ( const std::string& id : ids ) {
        if ( !Properties().has( PROP_POSITION, id ) ) {
            continue;
        }
        fpVec2 position = GetProperty< fpVec2 >( PROP_POSITION, id );

        auto last = lastPositions.find( id );
        if ( last != lastPositions.end() && last->second.x == position.x && last->second.y == position.y ) {
            continue;
        }

        lastPositions[ id ] = position;
        eventManager.queueEvent( movedEvent( id, position, now ) );
    }
}

void ReplaySystem::update( double now ) {
    lastNow = now;
    if ( currentState != State::Playing ) {
        return;
    }

    double elapsed = now - playStart;
    while ( nextToPlay < log.size() && log[ nextToPlay ].time <= elapsed ) {
        queueReplayed( log[ nextToPlay ].event, now );
        nextToPlay++;
    }

    if ( nextToPlay >= log.size() && elapsed >= recordLength ) {
        eventManager.queueEvent( EventType::ReplayStopPlayback, now );
    }
}

double ReplaySystem::recordedSeconds() const {
    if ( currentState == State::Recording ) {
        return std::max( 0.0, lastNow - recordStart );
    }
    return recordLength;
}

double ReplaySystem::playbackSeconds() const {
    if ( currentState != State::Playing ) {
        return 0.0;
    }
    return std::min( recordLength, std::max( 0.0, lastNow - playStart ) );
}
