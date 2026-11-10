#ifndef REPLAY_SYSTEM_HPP
#define REPLAY_SYSTEM_HPP

#include "eventManager.hpp"
#include <map>
#include <string>
#include <utility>
#include <vector>

// Moved event arguments: the id of the object and its new position ( fpVec2 )
const std::string MOVED_ARG_OBJECT = "object";
const std::string MOVED_ARG_POSITION = "position";

// bool argument set on every event the replay system raises again during playback
const std::string REPLAY_ARG_REPLAYED = "replayed";

/**
    True for an event that came out of a recording instead of the live game
*/
bool isReplayed( const Event& event );

/**
    Records events and plays them back, built on the EventManager

    Controlled with events
        ReplayStartRecording    forget the old recording and start a new one
        ReplayStopRecording     finish the recording
        ReplayStartPlayback     play the recording from the start
        ReplayStopPlayback      end playback, also raised by the system when the recording runs out

    While recording, every Moved event and every event of the recorded types is copied
    into a log kept in memory, with its time since the recording started
    During playback the logged events are queued again when their time comes, marked with
    REPLAY_ARG_REPLAYED, so the game's handlers can show them without treating them as live

    Timestamps are Timeline time, so pausing or changing the speed of the timeline
    pauses or changes the speed of the playback

    Raise the control events once the frame's movement and collisions are finished
    Starting in the middle of a frame would capture objects before their collisions are resolved
*/
class ReplaySystem {
    public:
        enum class State { Idle, Recording, Playing };

        /**
            recordedTypes   event types to record besides Moved
        */
        ReplaySystem( EventManager& eventManager, const std::vector< EventType >& recordedTypes );
        ~ReplaySystem();

        ReplaySystem( const ReplaySystem& ) = delete;
        ReplaySystem& operator=( const ReplaySystem& ) = delete;

        /**
            Raises a Moved event for each of these objects whose position changed since the last call
            Call it once a frame while the live game is running, not during playback
        */
        void raiseMoved( double now, const std::vector< std::string >& ids );

        /**
            Call once a frame. During playback it queues the recorded events that are due
        */
        void update( double now );

        State state() const { return currentState; }
        bool isRecording() const { return currentState == State::Recording; }
        bool isPlaying() const { return currentState == State::Playing; }

        // Length of the recording so far, and how far playback has got, in seconds
        double recordedSeconds() const;
        double playbackSeconds() const;
        size_t eventCount() const { return log.size(); }

    private:
        struct Recorded {
            double time;    // seconds since the recording started
            Event event;
        };

        EventManager& eventManager;
        std::vector< std::pair< EventType, int > > handlerIds;

        State currentState = State::Idle;
        std::vector< Recorded > log;
        double recordStart = 0.0;
        double recordLength = 0.0;
        double playStart = 0.0;
        size_t nextToPlay = 0;
        double lastNow = 0.0;

        // Objects raiseMoved was last called with, and where they were
        std::vector< std::string > tracked;
        std::map< std::string, fpVec2 > lastPositions;

        // Where everything was when playback started, put back when it ends
        std::map< std::string, fpVec2 > livePositions;

        void listen( EventType type, EventHandler handler );
        void record( const Event& event );
        void startRecording( double now );
        void stopRecording( double now );
        void startPlayback( double now );
        void stopPlayback( double now );
        void queueReplayed( Event event, double now );
};

#endif
