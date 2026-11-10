#ifndef INPUT_HPP
#define INPUT_HPP

#include <SDL3/SDL.h>
#include <unordered_map>
#include "eventManager.hpp"

class InputSystem {
private:
    EventManager& eventManager;
    const bool* keyboardState = nullptr;
    int numKeys = 0;

    std::unordered_map< SDL_Scancode, std::string > keyBindings;

public:
    InputSystem( EventManager& eventManager );
    ~InputSystem();

    void bindKey( SDL_Scancode key, const std::string& action );
    void update( double now );
    bool isKeyPressed(SDL_Scancode scancode) const;
};

#endif
