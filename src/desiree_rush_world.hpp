#pragma once

/**
    Coin Rush - the game simulation.

    Built only from engine pieces: fp / fpVec2 fixed-point math, Physics + Dynamic bodies,
    Entity + collision boxes. There are no SDL calls in here except key names and drawing,
    so the same code runs in both launch modes:

        Lockstep   (peer-to-peer)  both players are simulated on both machines from exchanged keyboard state
        Networked  (client-server) only the local player is simulated, the server owns platform/hazard/coins

    Determinism rules this file follows (needed for lockstep):
      * every number in the simulation is fp, never float or double
      * the step is a constant dt, never wall-clock time
      * anything that varies (platform, hazard) is a pure function of the tick counter and the shared seed
      * order of updates is fixed (actor 0 before actor 1)

    One tick is three phases so the middle one can be split across worker threads:
        beginTick()   main thread   read inputs, move platform + hazard
        stepActor(i)  any thread    physics + landing for actor i (touches only actor i)
        endTick()     main thread   hazard hits, coin pickup, scoring (in fixed order)
*/

#include "desiree_rush_level.hpp"
#include "coolMath.hpp"
#include "coolPhysics.hpp"
#include "entity.hpp"

#include <SDL3/SDL.h>

#include <array>
#include <cstdint>
#include <cstring>

namespace rush {

struct PadInput {
    bool left = false;
    bool right = false;
    bool jump = false;
    bool pause = false;
    bool faster = false;
    bool slower = false;
};

// works for both InputSystem and KeyboardSnapshot (anything with isKeyPressed)
template < class Keys >
PadInput padFrom( const Keys& keys ) {
    PadInput pad;
    pad.left = keys.isKeyPressed( SDL_SCANCODE_A );
    pad.right = keys.isKeyPressed( SDL_SCANCODE_D );
    pad.jump = keys.isKeyPressed( SDL_SCANCODE_SPACE ) || keys.isKeyPressed( SDL_SCANCODE_W );
    pad.pause = keys.isKeyPressed( SDL_SCANCODE_RETURN );
    pad.faster = keys.isKeyPressed( SDL_SCANCODE_UP );
    pad.slower = keys.isKeyPressed( SDL_SCANCODE_DOWN );
    return pad;
}

inline fp fpi( int value ) {
    return fp( static_cast< int64_t >( value ) );
}

/**
    One player: an Entity (position + collision box), a Dynamic body, and its own Physics.
    One Physics per actor so two actors can be stepped on two threads without sharing anything.
*/
struct Actor {
    Entity entity;
    Dynamic body;
    Physics physics;
    fpVec2 spawn;
    PadInput input;
    bool onGround = false;
    bool onMover = false;
    int score = 0;

    Actor() {
        body.setMass( fp( 1.0f ) );
        body.setGravity( fpVec2( fp( 0.0f ), fp( 1500.0f ) ) );
        body.setFriction( fpVec2( fp( 1.0f ), fp( 1.0f ) ) );
        body.entity = &entity;
        physics.addBody( &body );
    }
};

class Sim {
    public:
        enum class Mode { Lockstep, Networked };

        Sim( Mode mode, uint32_t seed ): mode_( mode ) {
            dt_ = fp( 1.0f ) / fp( static_cast< float >( TICKS_PER_SECOND ) );
            moveSpeed_ = fp( 240.0f );
            jumpSpeed_ = fp( -640.0f );

            // the shared seed decides where the platform and hazard start in their cycles
            phaseMover_ = static_cast< int64_t >( seed % ( 2u * MOVER_ONE_WAY_TICKS ) );
            phaseHazard_ = static_cast< int64_t >( ( seed / 7u ) % ( 2u * HAZARD_ONE_WAY_TICKS ) );

            for ( int i = 0; i < STATIC_COUNT; i++ ) {
                const Rect& r = STATIC_SOLIDS[ i ];
                place( statics_[ i ], r.cx, r.cy, r.hw, r.hh );
                statics_[ i ].setColor( 96, 74, 58, 255 );
            }

            place( mover_, moverX( phaseMover_ ), MOVER_Y, MOVER_HW, MOVER_HH );
            mover_.setColor( 214, 150, 52, 255 );

            place( hazard_, hazardX( phaseHazard_ ), HAZARD_Y, HAZARD_HALF, HAZARD_HALF );
            hazard_.setColor( 210, 54, 54, 255 );

            for ( int c = 0; c < COIN_COUNT; c++ ) {
                place( coins_[ c ], COIN_POS[ c ][ 0 ], COIN_POS[ c ][ 1 ], COIN_HALF, COIN_HALF );
                coins_[ c ].setColor( 250, 214, 48, 255 );
            }

            actors_[ 0 ].entity.setColor( 66, 140, 232, 255 );
            actors_[ 1 ].entity.setColor( 70, 200, 120, 255 );
            setSpawn( 0, 150 );
            setSpawn( 1, 810 );
        }

        Sim( const Sim& ) = delete;
        Sim& operator=( const Sim& ) = delete;

        // ---------- one tick ----------

        void beginTick( const PadInput& in0, const PadInput& in1 ) {
            running_ = true;

            if ( mode_ == Mode::Lockstep ) {
                // Pause and speed live INSIDE the simulation. They come from the shared keyboard
                // state, so both peers make the same change on the same tick. (A paused clock
                // would stop the input exchange and deadlock the peers, so pause can't be local.)
                bool pausePressed = ( in0.pause && !prev_[ 0 ].pause ) || ( in1.pause && !prev_[ 1 ].pause );
                bool fasterPressed = ( in0.faster && !prev_[ 0 ].faster ) || ( in1.faster && !prev_[ 1 ].faster );
                bool slowerPressed = ( in0.slower && !prev_[ 0 ].slower ) || ( in1.slower && !prev_[ 1 ].slower );
                prev_[ 0 ] = in0;
                prev_[ 1 ] = in1;

                if ( pausePressed ) {
                    paused_ = !paused_;
                }
                if ( fasterPressed && speedSteps_ < 4 ) {
                    speedSteps_++;
                }
                if ( slowerPressed && speedSteps_ > -2 ) {
                    speedSteps_--;
                }

                if ( paused_ ) {
                    running_ = false;
                    return;
                }

                actors_[ 0 ].input = in0;
                actors_[ 1 ].input = in1;

                // platform and hazard are pure functions of the tick counter
                moveKinematics( fpi( moverX( tick_ + phaseMover_ ) ), fpi( hazardX( tick_ + phaseHazard_ ) ) );
            } else {
                actors_[ 0 ].input = in0;
            }

            moverDx_ = pendingMoverDx_;
            pendingMoverDx_ = fp( 0.0f );
        }

        // Touches only actor i (plus read-only platform data), so different i can run on different threads
        void stepActor( int i ) {
            if ( !running_ ) {
                return;
            }
            if ( mode_ == Mode::Networked && i != 0 ) {
                return;
            }

            Actor& a = actors_[ i ];

            // riding the moving platform carries you with it
            if ( a.onMover ) {
                a.entity.position().x += moverDx_;
            }

            fp vx = fp( 0.0f );
            if ( a.input.left ) {
                vx = vx - moveSpeed_;
            }
            if ( a.input.right ) {
                vx = vx + moveSpeed_;
            }
            a.body.velocity.x = vx;

            if ( a.input.jump && a.onGround ) {
                a.body.velocity.y = jumpSpeed_;
            }

            fp prevBottom = a.entity.position().y + fpi( PLAYER_HALF );

            a.physics.step( dt_ );

            fp minX = fpi( PLAYER_HALF );
            fp maxX = fpi( WORLD_W - PLAYER_HALF );
            if ( a.entity.position().x < minX ) {
                a.entity.position().x = minX;
            }
            if ( a.entity.position().x > maxX ) {
                a.entity.position().x = maxX;
            }
            refreshActorBox( a );

            // The engine's physics only integrates motion, landing is game logic:
            // land on a platform if we are falling and our feet were above its top last tick
            a.onGround = false;
            a.onMover = false;
            if ( a.body.velocity.y >= fp( 0.0f ) ) {
                for ( int s = 0; s < STATIC_COUNT; s++ ) {
                    tryLand( a, statics_[ s ], STATIC_SOLIDS[ s ].hh, prevBottom, false );
                }
                tryLand( a, mover_, MOVER_HH, prevBottom, true );
            }
        }

        void endTick() {
            if ( !running_ ) {
                return;
            }

            int count = ( mode_ == Mode::Lockstep ) ? 2 : 1;

            for ( int i = 0; i < count; i++ ) {
                if ( actors_[ i ].entity.box().checkOverlap( &hazard_.box() ) ) {
                    respawn( i );
                }
            }

            // In networked mode the server owns the coins, so only lockstep decides pickups here.
            // Fixed order (coin by coin, actor 0 before actor 1) keeps both peers identical.
            if ( mode_ == Mode::Lockstep ) {
                for ( int c = 0; c < COIN_COUNT; c++ ) {
                    if ( coinCollected_[ c ] ) {
                        continue;
                    }
                    for ( int i = 0; i < 2; i++ ) {
                        if ( actors_[ i ].entity.box().checkOverlap( &coins_[ c ].box() ) ) {
                            coinCollected_[ c ] = true;
                            actors_[ i ].score++;
                            break;
                        }
                    }
                }

                bool all = true;
                for ( int c = 0; c < COIN_COUNT; c++ ) {
                    all = all && coinCollected_[ c ];
                }
                if ( all ) {
                    for ( int c = 0; c < COIN_COUNT; c++ ) {
                        coinCollected_[ c ] = false;
                    }
                }
            }

            tick_++;
        }

        // convenience for tests and for running without threads
        void runTickSerial( const PadInput& in0, const PadInput& in1 ) {
            beginTick( in0, in1 );
            stepActor( 0 );
            stepActor( 1 );
            endTick();
        }

        // ---------- networked mode: the server tells us where its objects are ----------

        void setExternal( double moverCenterX, double hazardCenterX ) {
            moveKinematics( fp( moverCenterX ), fp( hazardCenterX ) );
        }

        void setCoinCollected( int c, bool collected ) {
            coinCollected_[ c ] = collected;
        }

        void setSpawn( int i, int x ) {
            actors_[ i ].spawn = fpVec2( fpi( x ), fpi( SPAWN_Y ) );
            respawn( i );
        }

        // ---------- read access for drawing and tests ----------

        Actor& actor( int i ) { return actors_[ i ]; }
        Entity& solid( int i ) { return statics_[ i ]; }
        Entity& mover() { return mover_; }
        Entity& hazard() { return hazard_; }
        Entity& coin( int c ) { return coins_[ c ]; }
        bool coinCollected( int c ) const { return coinCollected_[ c ]; }
        bool paused() const { return paused_; }
        int speedSteps() const { return speedSteps_; }
        double timeScale() const { return 1.0 + 0.25 * speedSteps_; }
        int64_t tick() const { return tick_; }

        // fingerprint of the whole simulation, used to compare two machines/runs
        uint64_t stateHash() const {
            uint64_t h = 1469598103934665603ull;
            auto mix = [ & ]( double value ) {
                uint64_t bits = 0;
                std::memcpy( &bits, &value, sizeof( bits ) );
                h ^= bits;
                h *= 1099511628211ull;
            };
            for ( int i = 0; i < 2; i++ ) {
                const Actor& a = actors_[ i ];
                mix( a.entity.position().x.toDouble() );
                mix( a.entity.position().y.toDouble() );
                mix( a.body.velocity.x.toDouble() );
                mix( a.body.velocity.y.toDouble() );
                mix( static_cast< double >( a.score ) );
            }
            mix( mover_.position().x.toDouble() );
            mix( hazard_.position().x.toDouble() );
            mix( static_cast< double >( tick_ ) );
            mix( paused_ ? 1.0 : 0.0 );
            mix( static_cast< double >( speedSteps_ ) );
            for ( int c = 0; c < COIN_COUNT; c++ ) {
                mix( coinCollected_[ c ] ? 1.0 : 0.0 );
            }
            return h;
        }

    private:
        Mode mode_;
        fp dt_;
        fp moveSpeed_;
        fp jumpSpeed_;

        std::array< Actor, 2 > actors_;
        std::array< Entity, STATIC_COUNT > statics_;
        std::array< Entity, COIN_COUNT > coins_;
        Entity mover_;
        Entity hazard_;
        std::array< bool, COIN_COUNT > coinCollected_{};

        PadInput prev_[ 2 ];
        bool paused_ = false;
        bool running_ = true;
        int speedSteps_ = 0;
        int64_t tick_ = 0;
        int64_t phaseMover_ = 0;
        int64_t phaseHazard_ = 0;

        fp moverDx_;
        fp pendingMoverDx_;

        static void place( Entity& e, int cx, int cy, int hw, int hh ) {
            e.position() = fpVec2( fpi( cx ), fpi( cy ) );
            e.setCollisionBox( fpi( hw ), fpi( hh ) );
        }

        static void refreshActorBox( Actor& a ) {
            a.entity.setCollisionBox( fpi( PLAYER_HALF ), fpi( PLAYER_HALF ) );
        }

        void moveKinematics( fp newMoverX, fp newHazardX ) {
            pendingMoverDx_ += newMoverX - mover_.position().x;
            mover_.position().x = newMoverX;
            mover_.setCollisionBox( fpi( MOVER_HW ), fpi( MOVER_HH ) );

            hazard_.position().x = newHazardX;
            hazard_.setCollisionBox( fpi( HAZARD_HALF ), fpi( HAZARD_HALF ) );
        }

        void respawn( int i ) {
            Actor& a = actors_[ i ];
            a.entity.position() = a.spawn;
            a.body.velocity = fpVec2();
            a.body.force = fpVec2();
            a.onGround = false;
            a.onMover = false;
            refreshActorBox( a );
        }

        void tryLand( Actor& a, Entity& solid, int solidHalfHeight, fp prevBottom, bool isMover ) {
            if ( a.onGround ) {
                return;
            }
            fp top = solid.position().y - fpi( solidHalfHeight );
            if ( prevBottom <= top && a.entity.box().checkOverlap( &solid.box() ) ) {
                a.entity.position().y = top - fpi( PLAYER_HALF );
                a.body.velocity.y = fp( 0.0f );
                refreshActorBox( a );
                a.onGround = true;
                a.onMover = isMover;
            }
        }
};

}