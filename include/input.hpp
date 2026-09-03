#ifndef INPUT_HPP
#define INPUT_HPP

#include <SDL3/SDL.h>

class InputSystem {
private:
    const bool* keyboardState;
    int numKeys;

public:
    InputSystem();
    ~InputSystem();

    void update();
    bool isKeyPressed(SDL_Scancode scancode) const;
};

#endif