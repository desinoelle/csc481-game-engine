#include "input.hpp"

InputSystem::InputSystem() {
    keyboardState = SDL_GetKeyboardState(&numKeys);
}

InputSystem::~InputSystem() {
}

void InputSystem::update() {
    keyboardState = SDL_GetKeyboardState(&numKeys);
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