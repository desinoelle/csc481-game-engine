#include "timeMagic.hpp"

#include <SDL3/SDL.h>
#include <cmath>

void Timeline::init() {
    lastTicks = SDL_GetTicks();
};

double Timeline::update() {
    Uint64 now = SDL_GetTicks();
    double delta = (now - lastTicks) / 1000.0;
    lastTicks = now;

    if (isPaused()) {
        return 0.0;
    } else {
        accumulated += delta * timeScale;
        return accumulated;
    }
};

void Timeline::setScale(double s) {
    timeScale = std::max(MIN_SPEED, std::min(MAX_SPEED, s));
};