#pragma once
#include <atomic>
#include <mutex>
#include "input.hpp"
#include "coolPhysics.hpp"
#include "timeMagic.hpp"

/**
 * Shared state for multithreaded engine.
 * All game objects and systems access this shared data safely.
 */
struct SharedData {
    // Game objects (pointers to be updated by threads)
    Dynamic* player = nullptr;
    Dynamic* enemy = nullptr;
    Physics* physics = nullptr;
    Timeline* timeline = nullptr;
    InputSystem* inputSystem = nullptr;

    // Control states
    bool paused = false;
    bool movingEnemyRight = true;
    bool spaceLastFrame = false;

    // Job distribution (lock-free)
    std::atomic<size_t> nextJobIndex{0};

    // Pause toggle protection
    std::mutex pauseMutex;
    bool returnLastFrame = false;
};