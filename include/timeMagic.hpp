#ifndef TIME_MAGIC
#define TIME_MAGIC

#include <SDL3/SDL.h>
#include <algorithm>

struct Timeline {
protected:
    double timeScale = 1.0;
    Uint64 lastTicks = 0;
    float lastDelta = 0.0f;

    const double MAX_SPEED = 2.0;
    const double MIN_SPEED = 0.5;

    bool paused = false;

public:
    void init();

    float update();

    void setScale(double s);

    double getScale() const { return timeScale;};

    void setPause(bool pause) {paused = pause;};

    bool isPaused() const {return paused;};

    float getDeltaTime() const { return lastDelta; };
};

#endif