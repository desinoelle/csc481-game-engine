#ifndef TIME_MAGIC
#define TIME_MAGIC

struct Timeline {
protected:
    double timeScale = 1.0;
    Uint64 lastTicks = 0;
    float accumulated = 0.0;

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
};

#endif