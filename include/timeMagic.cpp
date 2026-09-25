#include "timeMagic.hpp"

#include <SDL3/SDL.h>
#include <cmath>

void Timeline::init() {
    lastTicks = SDL_GetTicks();
    lastDelta = 0.0f;
};

float Timeline::update() {
    Uint64 now = SDL_GetTicks();
    float delta = (now - lastTicks) / 1000.0f;
    lastTicks = now;

    if ( paused ) {
        lastDelta = 0.0f;
        return 0.0f;
    } else {
        lastDelta = delta * (float)timeScale;
        return lastDelta;
    }
};

void Timeline::setScale(double s) {
    timeScale = std::max(MIN_SPEED, std::min(MAX_SPEED, s));
};