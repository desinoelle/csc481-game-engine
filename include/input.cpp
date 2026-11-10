#include "input.hpp"

InputSystem::InputSystem( EventManager& eventManager ) : eventManager( eventManager ) {
    keyboardState = SDL_GetKeyboardState(&numKeys);
}

InputSystem::~InputSystem() {
}

void InputSystem::bindKey( SDL_Scancode key, const std::string& action ) {
    keyBindings[ key ] = action;
}

void InputSystem::update( double now ) {
    keyboardState = SDL_GetKeyboardState(&numKeys);

    for ( const auto& [ scancode, action ] : keyBindings ) {
        if ( isKeyPressed( scancode ) ) {
            eventManager.queueEvent( EventType::Input, now, 0, { { "action, action } } );
        }
    }
}

bool InputSystem::isKeyPressed(SDL_Scancode scancode) const {
    if (!keyboardState) {
        return false;
    }
    if (scancode >= numKeys) {
        return false;
    }
    return keyboardState[scancode];
}
