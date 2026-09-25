#ifndef TIME_MAGIC
#define TIME_MAGIC

struct Timeline {
private:
    double timeScale = 1.0;
    Uint64 lastTicks = 0;
    float accumulated = 0.0;

    const double MAX_SPEED = 2.0;
    const double MIN_SPEED = 0.5;

    boolean paused = false;

public:
    void init();

    float update();

    void setScale(double s);

    double getScale() const { return timeScale;};

    void setPause(boolean pause) {paused = pause;};

    boolean isPaused() const {return paused;};
};

#endif