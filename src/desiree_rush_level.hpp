#pragma once

/**
    Coin Rush - level layout shared by the game and its server.
    Plain integers only, no engine or SDL includes, so the headless server can use it too.
    y grows downward (like the screen), so "top" of a platform is cy - hh.
*/

#include <cmath>
#include <cstdint>

namespace rush {

constexpr int WORLD_W = 960;
constexpr int WORLD_H = 540;
constexpr int TICKS_PER_SECOND = 60;

constexpr int PLAYER_HALF = 14;
constexpr int COIN_HALF = 9;
constexpr int HAZARD_HALF = 16;

struct Rect {
    int cx, cy, hw, hh;
};

// ground + three floating platforms (one-way: you can jump up through them)
constexpr int STATIC_COUNT = 4;
constexpr Rect STATIC_SOLIDS[ STATIC_COUNT ] = {
    { 480, 525, 480, 15 },   // ground, top edge at y = 510
    { 190, 405,  90,  8 },   // left platform
    { 770, 405,  90,  8 },   // right platform
    { 480, 300,  80,  8 },   // high center platform
};

constexpr int GROUND_TOP = 510;
constexpr int SPAWN_Y = GROUND_TOP - PLAYER_HALF;

// moving platform: slides left and right, owned by the SERVER in client-server mode
constexpr int MOVER_Y = 455;
constexpr int MOVER_HW = 60;
constexpr int MOVER_HH = 8;
constexpr int MOVER_MIN_X = 330;
constexpr int MOVER_RANGE = 300;
constexpr int MOVER_ONE_WAY_TICKS = 300;      // 5 seconds each direction

// hazard: patrols the ground, touching it sends you back to your spawn
constexpr int HAZARD_Y = GROUND_TOP - HAZARD_HALF;
constexpr int HAZARD_MIN_X = 200;
constexpr int HAZARD_RANGE = 560;
constexpr int HAZARD_ONE_WAY_TICKS = 240;     // 4 seconds each direction

constexpr int COIN_COUNT = 8;
constexpr int COIN_POS[ COIN_COUNT ][ 2 ] = {
    { 110, 490 }, { 850, 490 },     // ground level
    { 190, 384 }, { 770, 384 },     // on the side platforms
    { 480, 279 },                   // on the high platform
    { 480, 425 },                   // above the moving platform
    { 330, 350 }, { 630, 350 },     // mid-air
};

// Reserved ids for the existing "STATE <count> <id> <x> <y> ..." message, so the
// engine's wire format does not have to change. Real players use ids 1..999.
constexpr int PLATFORM_ID = 0;        // x = moving platform centre x
constexpr int COIN_ID_BASE = 1000;    // 1000 + n exists only while coin n is uncollected
constexpr int SCORE_ID_BASE = 2000;   // 2000 + playerId, x = that player's score
constexpr int HAZARD_ID = 3000;       // x = hazard centre x

inline int spawnX( int slot ) {
    return 100 + 130 * ( slot % 6 );
}

/**
    Triangle wave built from integers only, so it gives identical results on every machine.
    Goes 0 -> range over oneWay ticks, then back down.
*/
inline int triangle( int64_t tick, int oneWay, int range ) {
    int64_t period = 2LL * oneWay;
    int64_t phase = tick % period;
    if ( phase < 0 ) {
        phase += period;
    }
    int64_t t = phase < oneWay ? phase : period - phase;
    return static_cast< int >( ( t * range ) / oneWay );
}

inline int moverX( int64_t tick ) {
    return MOVER_MIN_X + triangle( tick, MOVER_ONE_WAY_TICKS, MOVER_RANGE );
}

inline int hazardX( int64_t tick ) {
    return HAZARD_MIN_X + triangle( tick, HAZARD_ONE_WAY_TICKS, HAZARD_RANGE );
}

// axis-aligned box test with doubles, used by the server for coin pickup
inline bool overlapsCoin( double px, double py, int coin ) {
    return std::fabs( px - COIN_POS[ coin ][ 0 ] ) <= PLAYER_HALF + COIN_HALF &&
           std::fabs( py - COIN_POS[ coin ][ 1 ] ) <= PLAYER_HALF + COIN_HALF;
}

}